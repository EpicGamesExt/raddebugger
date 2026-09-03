// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// --- Build Options -----------------------------------------------------------

#define BUILD_CONSOLE_INTERFACE 1
#define BUILD_TITLE "Epic Games Tools (R) RAD PE/COFF Linker"

#define ARENA_FREE_LIST 1
#define NO_ASYNC 1
#define NO_WIN32_RIO 1

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

#if OS_WINDOWS
# include <psapi.h> // GetProcessMemoryInfo for the end-of-link summary line
#endif

// --- Third Party -------------------------------------------------------------

#include "base_ext/base_blake3.h"
#include "base_ext/base_blake3.c"

// --- Code Base Extensions ----------------------------------------------------

#include "base_ext/base_inc.h"
#include "thread_pool/thread_pool.h"
#include "base_ext/base_radix_sort.h"
#include "codeview_ext/codeview.h"
#include "pdb_ext/msf_builder.h"

#include "base_ext/base_inc.c"
#include "thread_pool/thread_pool.c"
#include "base_ext/base_radix_sort.c"
#include "codeview_ext/codeview.c"

// --- PDB Extensions- ---------------------------------------------------------

#include "pdb_ext/pdb.h"
#include "pdb_ext/pdb_helpers.h"
#include "pdb_ext/pdb_builder.h"

#include "pdb_ext/msf_builder.c"
#include "pdb_ext/pdb.c"
#include "pdb_ext/pdb_helpers.c"
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
#include "lnk_hasher.h"
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
#include "lnk_compressed_obj.h"
#include "lnk.h"

#include "lnk_log.c"
#include "lnk_timer.c"
#include "lnk_hasher.c"
#include "lnk_io.c"
#include "lnk_compressed_obj.c"
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
                                                "<?xml version='1.0' encoding='UTF-8' standalone='yes'?>\r\n"
                                                "<assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>\r\n"));
  if (manifest_uac) {
    String8 uac = push_str8f(scratch.arena,
                             "  <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">\r\n"
                             "    <security>\r\n"
                             "      <requestedPrivileges>\r\n"
                             "        <requestedExecutionLevel level=%S uiAccess=%S />\r\n"
                             "      </requestedPrivileges>\r\n"
                             "    </security>\r\n"
                             "  </trustInfo>\r\n",
                             manifest_level,
                             manifest_ui_access);
    str8_serial_push_string(scratch.arena, &srl, uac);
  }
  for (String8Node *node = manifest_dependency_list.first; node != 0; node = node->next) {
    String8 dep = push_str8f(scratch.arena, 
                             "  <dependency>\r\n"
                             "    <dependentAssembly>\r\n"
                             "      <assemblyIdentity %S />\r\n"
                             "    </dependentAssembly>\r\n"
                             "  </dependency>\r\n",
                             node->string);
    str8_serial_push_string(scratch.arena, &srl, dep);
  }
  str8_serial_push_string(scratch.arena, &srl, str8_lit("</assembly>\r\n"));

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
    B8 was_manifest_read;
    manifest_data = lnk_read_data_from_file_path(arena, io_flags, merged_manifest_path, &was_manifest_read);
    if (was_manifest_read == 0) {
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
  
  struct QueueNode {
    struct QueueNode      *next;
    PE_Resource           *res;
    COFF_ResourceDirEntry *coff_entry;
  };
  struct QueueNode *queue_first = 0;
  struct QueueNode *queue_last  = 0;

  PE_Resource *root_wrapper = push_array(scratch.arena, PE_Resource, 1);
  root_wrapper->id.type     = COFF_ResourceIDType_Number;
  root_wrapper->id.u.number = 0;
  root_wrapper->kind        = PE_ResDataKind_DIR;
  root_wrapper->u.dir       = root_dir;

  COFF_ResourceDirEntry *root_entry = push_array(scratch.arena, COFF_ResourceDirEntry, 1);
  struct QueueNode *root_node       = push_array(scratch.arena, struct QueueNode, 1);
  root_node->res                     = root_wrapper;
  root_node->coff_entry              = root_entry;
  SLLQueuePush(queue_first, queue_last, root_node);

  COFF_ObjSection *rsrc1 = coff_obj_writer_push_section(obj_writer, str8_lit(".rsrc$01"), PE_RSRC1_SECTION_FLAGS, str8_zero());
  COFF_ObjSection *rsrc2 = coff_obj_writer_push_section(obj_writer, str8_lit(".rsrc$02"), PE_RSRC2_SECTION_FLAGS, str8_zero());
  
  for (struct QueueNode *node = queue_first; node != 0; node = queue_first) {
    SLLQueuePop(queue_first, queue_last);
    PE_Resource *res = node->res;

    node->coff_entry->id.data_entry_offset = safe_cast_u32(rsrc1->data.total_size);
    if (res->kind == PE_ResDataKind_DIR) {
      node->coff_entry->id.data_entry_offset |= COFF_Resource_SubDirFlag;
    }

    switch (res->kind) {
    case PE_ResDataKind_DIR: {
      COFF_ResourceDirTable *dir_header = push_array(obj_writer->arena, COFF_ResourceDirTable, 1);
      dir_header->characteristics       = res->u.dir->characteristics;
      dir_header->time_stamp            = res->u.dir->time_stamp;
      dir_header->major_version         = res->u.dir->major_version;
      dir_header->minor_version         = res->u.dir->minor_version;
      dir_header->name_entry_count      = res->u.dir->named_list.count;
      dir_header->id_entry_count        = res->u.dir->id_list.count;

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

      COFF_ResourceDirEntry *named_entries = push_array(obj_writer->arena, COFF_ResourceDirEntry, named_array.count);
      COFF_ResourceDirEntry *id_entries    = push_array(obj_writer->arena, COFF_ResourceDirEntry, id_array.count);
      str8_list_push(obj_writer->arena, &rsrc1->data, str8_struct(dir_header));
      str8_list_push(obj_writer->arena, &rsrc1->data, str8_array(named_entries, named_array.count));
      str8_list_push(obj_writer->arena, &rsrc1->data, str8_array(id_entries, id_array.count));

      for (U64 i = 0; i < named_array.count; i += 1) {
        PE_Resource            *child = &named_array.v[i];
        COFF_ResourceDirEntry *entry = &named_entries[i];
        U32                     name_off = safe_cast_u32(rsrc1->data.total_size);
        String8                 name = coff_resource_string_from_str8(obj_writer->arena, child->id.u.string);
        str8_list_push(obj_writer->arena, &rsrc1->data, name);
        entry->name.offset = COFF_Resource_SubDirFlag | name_off;

        struct QueueNode *child_node = push_array(scratch.arena, struct QueueNode, 1);
        child_node->res              = child;
        child_node->coff_entry       = entry;
        SLLQueuePush(queue_first, queue_last, child_node);
      }
      for (U64 i = 0; i < id_array.count; i += 1) {
        PE_Resource            *child = &id_array.v[i];
        COFF_ResourceDirEntry *entry = &id_entries[i];
        entry->name.id = child->id.u.number;

        struct QueueNode *child_node = push_array(scratch.arena, struct QueueNode, 1);
        child_node->res              = child;
        child_node->coff_entry       = entry;
        SLLQueuePush(queue_first, queue_last, child_node);
      }
    } break;

    case PE_ResDataKind_COFF_RESOURCE: {
      COFF_ResourceDataEntry *coff_res = push_array(obj_writer->arena, COFF_ResourceDataEntry, 1);
      coff_res->data_size = res->u.coff_res.data.size;
      coff_res->data_voff = 0;
      coff_res->code_page = 0;

      if (res->u.coff_res.data.size >= sizeof(U32)) {
        U32             resdat_off = safe_cast_u32(rsrc2->data.total_size);
        COFF_ObjSymbol *resdat     = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("resdat"), resdat_off, rsrc2);
        U64             apply_off  = rsrc1->data.total_size + OffsetOf(COFF_ResourceDataEntry, data_voff);
        coff_obj_writer_section_push_reloc_voff(obj_writer, rsrc1, safe_cast_u32(apply_off), resdat);
      }

      str8_list_push(obj_writer->arena, &rsrc1->data, str8_struct(coff_res));
      str8_list_push(obj_writer->arena, &rsrc2->data, res->u.coff_res.data);
      str8_list_push_aligner(obj_writer->arena, &rsrc2->data, 0, 8);
    } break;

    case PE_ResDataKind_NULL: break;
    case PE_ResDataKind_COFF_LEAF: InvalidPath;
    }
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
lnk_make_linker_obj(Arena *arena, LNK_Config *config, MSCRT_FeatFlags feat_flags)
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
    
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit(MSCRT_GUARD_FLAGS_SYMBOL_NAME)        , feat_flags & MSCRT_FeatFlag_GUARD_STACK, COFF_SymStorageClass_External);
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

typedef struct LNK_InputOpenTask
{
  LNK_Input **inputs;
  String8    *datas;
} LNK_InputOpenTask;

internal
THREAD_POOL_TASK_FUNC(lnk_input_open_task)
{
  LNK_InputOpenTask *task = raw_task;
  LNK_Input *input = task->inputs[task_id];
  String8 data = task->datas[task_id];
  if (!input->has_disk_read_failed) {
    lnk_compressed_obj_open(input, data);
  }
}

typedef struct LNK_InputClassifyTask
{
  String8    *datas;
  U8         *tags;
} LNK_InputClassifyTask;

enum
{
  LNK_InputClassify_PortableCompressed = (1 << 0),
};

internal
THREAD_POOL_TASK_FUNC(lnk_input_classify_task)
{
  LNK_InputClassifyTask *task = raw_task;
  String8 data = task->datas[task_id];

  U8 tag = 0;
  if (data.size >= sizeof(LNK_CObjHeader) && ((LNK_CObjHeader *)data.str)->magic == LNK_COBJ_MAGIC) {
    if (((LNK_CObjHeader *)data.str)->flags & LNK_COBJ_FLAG_PORTABLE_RAW_MAP) {
      tag |= LNK_InputClassify_PortableCompressed;
    }
  }
  task->tags[task_id] = tag;
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
  B8           *thin_input_was_read = push_array(scratch.arena, B8, thin_input_paths.count);
  String8Array  thin_input_datas    = lnk_read_data_from_file_path_parallel(tp, inputer->arena, io_flags, thin_input_paths, thin_input_was_read);
  B32           is_mapped           = !!(io_flags & (LNK_IO_Flags_MemoryMapFilesReadWrite|LNK_IO_Flags_MemoryMapFilesReadOnly));

  for EachIndex(thin_input_idx, thin_inputs_count) {
    thin_inputs[thin_input_idx]->has_disk_read_failed = !thin_input_was_read[thin_input_idx];
    thin_inputs[thin_input_idx]->owns_file_map        = is_mapped && !thin_inputs[thin_input_idx]->has_disk_read_failed;
    thin_inputs[thin_input_idx]->data                 = thin_input_datas.v[thin_input_idx];
  }

  // Keep the ordinary-object path as direct mmap followed by cheap first-page classification.
  // Portable compressed objects have independent metadata and can be opened in parallel.
  B32 has_portable_cobj = 0;
  U8 *classify_tags = push_array_no_zero(scratch.arena, U8, thin_inputs_count);
  LNK_InputClassifyTask classify_task = {thin_input_datas.v, classify_tags};
  if (thin_inputs_count >= 64) {
    lnk_tp_for_parallel_capped(tp, 0, 32, thin_inputs_count, lnk_input_classify_task, &classify_task);
  } else {
    for EachIndex(i, thin_inputs_count) { lnk_input_classify_task(0, 0, i, &classify_task, tp); }
  }
  for EachIndex(thin_input_idx, thin_inputs_count) {
    has_portable_cobj |= !!(classify_tags[thin_input_idx] & LNK_InputClassify_PortableCompressed);
  }
  if (has_portable_cobj) {
    lnk_compressed_obj_prepare_cache(thin_input_datas.v, thin_input_datas.count);
    LNK_InputOpenTask open_task = {thin_inputs, thin_input_datas.v};
    // Reserving/splitting thousands of logical OBJ views contends on the Windows process VAD
    // lock. Four lanes measured substantially faster than 8-64 on the UEFN corpus.
    lnk_tp_for_parallel_capped(tp, 0, 4, thin_inputs_count, lnk_input_open_task, &open_task);
  }
  // Compressed inputs register their reserved address regions independently while the open tasks
  // run. Sort once after all tasks have joined; sorting after every insertion serialized the
  // parallel open and performed thousands of increasingly large qsorts.
  lnk_compressed_obj_finalize_open();
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

internal
THREAD_POOL_TASK_FUNC(lnk_release_file_map_task)
{
  LNK_Input **mapped_inputs = raw_task;
  LNK_Input  *input         = mapped_inputs[task_id];
  if (input->compressed_obj) {
    lnk_compressed_obj_close(input);
  } else {
    file_map_view_close((FileMap){0}, input->data.str, r1u64(0, input->data.size));
    input->data          = str8_zero();
    input->owns_file_map = 0;
  }
}

internal void
lnk_inputer_release_file_maps(TP_Context *tp, U64 worker_cap, LNK_Inputer *inputer)
{
  Temp scratch = scratch_begin(0, 0);

  U64 mapped_input_count = 0;
  for EachNode(input, LNK_Input, inputer->objs.first) {
    mapped_input_count += input->owns_file_map;
  }
  for EachNode(input, LNK_Input, inputer->libs.first) {
    mapped_input_count += input->owns_file_map;
  }

  LNK_Input **mapped_inputs = push_array_no_zero(scratch.arena, LNK_Input *, mapped_input_count);
  U64 mapped_input_idx = 0;
  for EachNode(input, LNK_Input, inputer->objs.first) {
    if (input->owns_file_map) {
      mapped_inputs[mapped_input_idx++] = input;
    }
  }
  for EachNode(input, LNK_Input, inputer->libs.first) {
    if (input->owns_file_map) {
      mapped_inputs[mapped_input_idx++] = input;
    }
  }
  Assert(mapped_input_idx == mapped_input_count);

  // UnmapViewOfFile serializes on the process address-space lock: with the
  // full pool every worker camps in the kernel for the whole pass (measured
  // ~257s of kernel CPU for a ~4.1s wall pass at 64 workers on the FN editor
  // DLL). Cap the lanes like the other fault/VAD-bound stages -- same wall,
  // a fraction of the spin.
  lnk_tp_for_parallel_capped(tp, 0, worker_cap, mapped_input_count, lnk_release_file_map_task, mapped_inputs);

  scratch_end(scratch);
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
      if (new_objs[obj_idx].data.coff.header.machine != COFF_MachineType_Unknown) {
        config->machine = new_objs[obj_idx].data.coff.header.machine;
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
        lnk_apply_cmd_option_to_config(config, dir->id, dir->value, obj);
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
  lnk_summary_phase_begin(LNK_SummaryPhase_Input);
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

  // TODO: This works around function override metadata pulled in from archives.
  // We need a BDD merger to implement the overrides. For now, create a weak
  // symbol for the function override to prevent all arch specific functions
  // from being pulled in from archives.
  {
    B32 has_function_overrides = 0;

    // Ordinary alternate names can number in the millions. Classify overrides
    // once when parsing directives, but revisit the override subset here: a
    // later archive member may introduce a reference to an earlier directive.
    for EachNode(alt_name_n, LNK_AltNameNode, config->function_override_list.first) {
      LNK_AltName alt_name = alt_name_n->v;

      LNK_Symbol *symbol = lnk_symbol_table_search(symtab, alt_name.from);
      if (symbol && lnk_interp_from_symbol(symbol) == COFF_SymbolValueInterp_Undefined) {
        COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_Unknown);
        COFF_ObjSymbol *to_symbol  = coff_obj_writer_push_symbol_undef(obj_writer, alt_name.to);
        coff_obj_writer_push_symbol_weak(obj_writer, alt_name.from, COFF_WeakExt_SearchAlias, to_symbol);
        String8 obj_data = coff_obj_writer_serialize(arena->v[0], obj_writer);
        coff_obj_writer_release(&obj_writer);

        LNK_Obj *obj = alt_name.obj;
        lnk_inputer_push_obj_linkgen(inputer, obj ? obj->link_member : 0,
                                              obj ? obj->path : str8_lit("RADLINK"), obj_data);

        has_function_overrides = 1;
      }
    }

    if (has_function_overrides) {
      lnk_load_inputs(tp, arena, config, inputer, symtab, link);
    }
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
        if (~config->flags & LNK_ConfigFlag_NoTsAware && ~config->file_characteristics & PE_ImageFileCharacteristic_DLL) {
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
  lnk_summary_phase_end(LNK_SummaryPhase_Input);
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
  U32                member_offset = memory_read32(lib->member_offsets + member_idx);
  COFF_ArchiveMember member_info   = coff_archive_member_from_offset(lib->data, member_offset);
  COFF_DataType      member_type   = coff_data_type_from_data(member_info.data);
  B32                is_import_member = (member_type == COFF_DataType_Import);

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

  if (is_import_member && is_queued_import) {
    // do not queue second import member link -> flag member and continue
    U8                 flag                = str8_starts_with(link_symbol->name, str8_lit("__imp_")) ? LNK_LibMemberFlag_LinkedImp : LNK_LibMemberFlag_LinkedRegular;
    LNK_LibMemberInfo *import_member_infos = hash_map_search_raw_raw(&lib_member_info_hm, is_queued_import->lib);
    ins_atomic_u8_or(&import_member_infos[is_queued_import->member_idx].flags, flag);
  } else {
    B32 do_queue;
    if (is_import_member && str8_starts_with(link_symbol->name, str8_lit("__imp_"))) {
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

  for EachNode(c, LNK_SymbolHashTrieChunk, symtab->search_chunks[task_id].first) {
    for EachIndex(i, c->count) {
      LNK_Symbol           *symbol      = c->v[i].symbol;
      LNK_SymbolSearchType  search_type = lnk_search_type_from_symbol(symbol);

      if (search_type == LNK_SymbolSearch_Undefined || search_type == LNK_SymbolSearch_WeakLibrary) {
        U32 member_idx;
        if (lnk_search_lib(lib, symbol->name, &member_idx)) {
          lnk_queue_lib_member(arena, task->imports_hm, task->link->lib_member_infos_hm, member_ref_list, symbol, lib, lib_member_infos, member_idx);
        }
      } else if (search_type == LNK_SymbolSearch_WeakAntiDependency && search_anti_deps) {
        LNK_ObjSymbolRef symbol_ref = lnk_ref_from_symbol(symbol);
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

internal U64
lnk_search_lib_task_work_count(LNK_SearchLibTask *task, U64 task_id)
{
  LNK_SymbolTable         *symtab      = task->symtab;
  U64 work_count = 0;
  for EachNode(c, LNK_SymbolHashTrieChunk, symtab->search_chunks[task_id].first) {
    work_count += c->count;
  }
  return work_count;
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

  // summary: this function is Input (lnk_load_inputs rounds, accumulated on its
  // own bucket) + Resolve (lib search / member resolution / directives, i.e.
  // everything else); attribute the remainder to Resolve at the bottom. The
  // same subtraction works for every counter: they are monotonic and the Input
  // brackets nest strictly inside this window
  LNK_SummaryCounters summary_begin    = lnk_summary_counters_now();
  LNK_SummaryCounters summary_input_at = g_summary_phase[LNK_SummaryPhase_Input];

  HashMap imports_hm = {0};

  LNK_LibMemberRefList *member_ref_lists = push_array(scratch.arena, LNK_LibMemberRefList, tp->worker_count);
  B32                   search_anti_deps = 0;
  for (U64 resolved_members_count = 0; ; resolved_members_count = 0) {
    ProfBegin("Search Pass");

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
        } else { // search symbols in lib
          MemoryZeroTyped(member_ref_lists, tp->worker_count);

          LNK_SearchLibTask search_task = {
            .search_anti_deps    = search_anti_deps,
            .link                = link,
            .imports_hm          = &imports_hm,
            .lib                 = lib,
            .symtab              = symtab,
            .lib_member_infos    = lib_member_infos,
            .member_ref_lists    = member_ref_lists
          };
          enum { serial_work_limit = 16384 };
          U64 search_work_count = 0;
          for EachIndex(task_id, tp->worker_count) {
            search_work_count += lnk_search_lib_task_work_count(&search_task, task_id);
            if (search_work_count > serial_work_limit) {
              break;
            }
          }
          if (search_work_count <= serial_work_limit) {
            // thread pool barrier waits dominate small library searches,
            // search small tasks on the main thread
            for EachIndex(task_id, tp->worker_count) {
              lnk_search_lib_task(arena->v[0], 0, task_id, &search_task, tp);
            }
          } else {
            tp_for_parallel(tp, arena, tp->worker_count, lnk_search_lib_task, &search_task);
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
            member_ref->link_symbol->last_ref_and_search_type = import_stub->last_ref_and_search_type;

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

    ProfEnd();
    if (resolved_members_count == 0) { break; }
  }

  {
    LNK_SummaryCounters now         = lnk_summary_counters_now();
    LNK_SummaryCounters window      = lnk_summary_counters_sub_sat(now, summary_begin);
    LNK_SummaryCounters input_delta = lnk_summary_counters_sub_sat(g_summary_phase[LNK_SummaryPhase_Input], summary_input_at);
    LNK_SummaryCounters resolve     = lnk_summary_counters_sub_sat(window, input_delta);
    g_summary_phase[LNK_SummaryPhase_Resolve].wall_us += resolve.wall_us;
    g_summary_phase[LNK_SummaryPhase_Resolve].user_us += resolve.user_us;
    g_summary_phase[LNK_SummaryPhase_Resolve].kern_us += resolve.kern_us;
    g_summary_phase[LNK_SummaryPhase_Resolve].faults  += resolve.faults;
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

  if (config->guard_flags != LNK_Guard_None) {
    lnk_include_symbol(config, str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME), 0);
  }

  // link inputer
  lnk_link_inputs(tp, arena, config, inputer, symtab, link);

  MSCRT_FeatFlags image_feat_flags = 0;
  for EachNode(obj_n, LNK_ObjNode, link->objs.first) {
    image_feat_flags |= lnk_obj_get_features(&obj_n->data);
  }
  if (image_feat_flags & MSCRT_FeatFlag_GUARD_STACK) {
    lnk_include_symbol(config, str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME), 0);
  }

  {
    ProfBegin("Push Linker Symbols");
    String8 linker_symbols_obj = lnk_make_linker_obj(arena->v[0], config, image_feat_flags);
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
        String8            import_obj           = pe_make_import_dll_obj_static(arena->v[0], time_stamp, config->machine, dll_name_n->string, import_debug_symbols, *imports, dll_name_n->next != 0);
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
    for EachNode(node, String8Node, config->input_list[LNK_Input_Res].first) {
      B8      was_read = 0;
      String8 res_data = lnk_read_data_from_file_path(scratch.arena, config->io_flags, node->string, &was_read);

      if (was_read == 0) {
        lnk_error(LNK_Error_LoadRes, "unable to open res file: %S", node->string);
        continue;
      }

      if (pe_is_res(res_data)) {
        str8_list_push(scratch.arena, &res_data_list, res_data);
        String8 stable_res_path = lnk_make_full_path(scratch.arena, config->path_style, config->work_dir, node->string);
        str8_list_push(scratch.arena, &res_path_list, stable_res_path);
      } else {
        lnk_error(LNK_Error_LoadRes, "file is not of RES format: %S", node->string);
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
      String8 obj_data     = lnk_make_linker_coff_obj(arena->v[0], config->time_stamp, config->machine, config->work_dir, lnk_get_image_name(config), config->pdb_name, config->raw_cmd_line, obj_name);
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
  // assign COMDAT leaders
  //
  {
    LNK_Obj **objs = lnk_array_from_obj_list(scratch.arena, link->objs);
    lnk_assign_comdat_symlinks(tp, arena, symtab, link->objs.count, objs);
  }

  //
  // was entry point resolved?
  //
  if (config->entry_point_name.size == 0 || link->try_to_resolve_entry_point) {
    String8      machine_str   = coff_string_from_machine_type(config->machine);
    String8      subsystem_str = pe_string_from_subsystem(config->subsystem);
    String8Array entry_points  = pe_get_entry_point_names(config->machine, config->subsystem, config->file_characteristics);

    String8List list = {0};
    for EachIndex(i, entry_points.count) { str8_list_push(scratch.arena, &list, entry_points.v[i]); }
    String8 default_entries = str8_list_join(scratch.arena, &list, &(StringJoin){.sep = str8_lit(", ")});

    StringJoin comma_join = {.sep = str8_lit(", ")};
    String8 obj_default_libs = str8_list_join(scratch.arena, &config->input_obj_lib_list, &comma_join);
    String8 cmd_default_libs = str8_list_join(scratch.arena, &config->input_default_lib_list, &comma_join);
    String8List loaded_lib_list = {0};
    for EachNode(lib_n, LNK_LibNode, link->libs.first) {
      str8_list_push(scratch.arena, &loaded_lib_list, str8_skip_last_slash(lib_n->data.path));
    }
    String8 loaded_libs = str8_list_join(scratch.arena, &loaded_lib_list, &comma_join);

    lnk_error(LNK_Error_EntryPoint,
              "failed to infer entry point symbol from the inputs\n"
              "  Machine:         %S\n"
              "  Subsystem:       %S\n"
              "  Subsystem Ver:   %llu.%llu\n"
              "  File Chars:      %S\n"
              "  DLL Chars:       %S\n"
              "  Default Entries: %S\n"
              "  User Entry:      \"%S\"\n"
              "  Input Obj Count: %S\n"
              "  Input Lib Count: %S\n"
              "  Obj Default Libs: %S\n"
              "  Cmd Default Libs: %S\n"
              "  Loaded Libs:      %S",
              machine_str.size   ? machine_str   : str8_lit("Unknown"),                         // Machine
              subsystem_str.size ? subsystem_str : str8_lit("Unknown"),                         // Version
              config->subsystem_ver.major, config->subsystem_ver.minor,                         // Subsystem
              pe_string_from_file_characteristics(scratch.arena, config->file_characteristics), // File Chars
              pe_string_from_dll_characteristics(scratch.arena, config->dll_characteristics),   // DLL Chars
              default_entries.size ? default_entries : str8_lit("None"),                        // Default Entry Points
              config->entry_point_name,                                                         // /ENTRY
              str8_from_count(scratch.arena, config->input_list[LNK_Input_Obj].node_count),     // Input Objects Count
              str8_from_count(scratch.arena, config->input_list[LNK_Input_Lib].node_count),     // Input Libs Count
              obj_default_libs.size ? obj_default_libs : str8_lit("None"),
              cmd_default_libs.size ? cmd_default_libs : str8_lit("None"),
              loaded_libs.size ? loaded_libs : str8_lit("None")
              );
  }

  //
  // report unresolved symbols
  //
  {
    ProfBegin("Report Unresolved Symbols");

    // collect unresolved symbols
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
      Temp debug_scratch = temp_begin(scratch.arena);

      LNK_ObjSymbolMap **func_symbol_maps = push_array(debug_scratch.arena, LNK_ObjSymbolMap *, link->objs.count);
      LNK_ObjLineMap   **line_maps        = push_array(debug_scratch.arena, LNK_ObjLineMap *,   link->objs.count);
      HashMap            message_hm       = {0};
      for EachIndex(i, unresolved_symbols_count) {
        LNK_Symbol *symbol = unresolved_symbols[i];

        if (i > config->unresolved_symbol_limit) {
          lnk_error(LNK_Error_UnresolvedSymbol, "too many unresolved symbol errors, stopping now");
          break;
        }

        String8List        ref_messages = {0};
        U64                refs_count   = 0;
        LNK_ObjSymbolRef **refs         = lnk_ref_from_symbol_many(scratch.arena, symbol, &refs_count);
        for EachIndex(ref_idx, refs_count) {
          LNK_ObjSymbolRef   *ref           = refs[ref_idx];
          LNK_Obj            *obj           = ref->obj;
          String8             string_table  = lnk_coff_string_table_from_obj(obj);

          if (func_symbol_maps[obj->input_idx] == 0) {
            func_symbol_maps[obj->input_idx] = lnk_symbol_map_from_obj(debug_scratch.arena, obj);
          }
          if (config->map_lines_for_unresolved_symbols == LNK_SwitchState_Yes) {
            if (line_maps[obj->input_idx] == 0) {
              line_maps[obj->input_idx] = lnk_line_map_from_obj(debug_scratch.arena, obj);
            }
          }

          COFF_SectionFlags sect_filter = 0;
          sect_filter |= COFF_SectionFlag_LnkRemove; // skip sections that are discarded from the output
          if ( ! lnk_do_debug_info(config)) { sect_filter |= LNK_SECTION_FLAG_DEBUG; } // on /DEBUG:NONE linker never invokes the debug info machinery

          for LNK_EachCoffSection(it, obj) {
            if (*it.v.flags & sect_filter) { continue; }

            // unpack section and relocations
            COFF_SectionHeader *section_header = it.v.header;
            String8             section_name   = coff_name_from_section_header(string_table, section_header);
            U64                 section_number = it.v.section_number;
            COFF_RelocArray     relocs         = lnk_coff_relocs_from_section_header(obj, section_header);

            // scan for undefined target symbols in relocations
            for EachIndex(reloc_idx, relocs.count) {
              // cap number of the diagnostic messages per symbol
              if (ref_messages.node_count > config->unresolved_symbol_ref_limit) {
                str8_list_pushf(scratch.arena, &ref_messages, "too many unresolved symbol references reported, stopping now");
                goto next_undefined_symbol;
              }

              // skip relocations that do not reference the unresolved symbol
              COFF_Reloc *reloc = &relocs.v[reloc_idx];
              if (reloc->isymbol != ref->symbol_idx) { continue; }

              String8  ref_name    = {0};
              String8  source_file = {0};
              CV_Line *source_line = 0;
              if (config->map_lines_for_unresolved_symbols == LNK_SwitchState_Yes) {
                LNK_ObjLineMap *line_map = line_maps[obj->input_idx];

                // is reference inline site?
                LNK_InlineSite *inline_site = lnk_inline_site_from_section_offset(line_map, section_number, reloc->apply_off, symbol);
                if (inline_site && inline_site->name.size) {
                  CV_Line *line_inline = lnk_line_from_inline_site(inline_site, reloc->apply_off);
                  if (line_inline) {
                    ref_name    = inline_site->name;
                    source_line = line_inline;
                  }
                }

                // default to regular line table
                if (source_line == 0) {
                  U64      line_matches_count = 0;
                  CV_Line *line_matches       = lnk_lines_from_section_offset(line_map, section_number, reloc->apply_off, &line_matches_count);
                  source_line = line_matches_count ? &line_matches[0] : 0;
                }

                if (source_line) {
                  // read source file name
                  CV_C13Checksum checksum = {0};
                  B32 is_invalid = source_line->line_num == 0 ||
                                   str8_deserial_read_struct(line_map->debug_checksums, source_line->file_off, &checksum) != sizeof(checksum) ||
                                   str8_deserial_read_cstr(line_map->debug_strings, checksum.name_off, &source_file) == 0;

                  // no file name? -> invalidate source info
                  if (is_invalid) {
                    ref_name    = str8_zero();
                    source_file = str8_zero();
                    source_line = 0;
                  }
                }
              }

              // no inline site? -> lookup function in the COFF symbol table
              if (ref_name.size == 0) {
                U32 symbol_idx = lnk_symbol_from_section_offset(func_symbol_maps[obj->input_idx], section_number, reloc->apply_off);
                ref_name = symbol_idx != max_U32 ? lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx) : str8_lit("(N/A)");
              }

              // pick location format
              String8 ref_location = source_line ?
                                     push_str8f(scratch.arena, "%S(%u)", source_file, source_line->line_num) :
                                     push_str8f(scratch.arena, "%S[%llx]+%x", section_name, section_number, reloc->apply_off);

              // push unique reference error
              String8 message = push_str8f(scratch.arena, "%S: referenced by '%S' in %S", ref_location, ref_name, lnk_loc_from_obj(scratch.arena, obj));
              if (hash_map_search_string_u64(&message_hm, message) == 0) {
                str8_list_push(scratch.arena, &ref_messages, message);
                hash_map_push_string_u64(scratch.arena, &message_hm, message, 1);
              }
            }
          }
        }
        next_undefined_symbol:;

        lnk_error(LNK_Error_UnresolvedSymbol, "unresolved symbol '%S'", symbol->name);
        if (str8_match(str8_prefix(symbol->name, 6), str8_lit("__imp_"), 0)) {
          lnk_supplement_error("this is a DLL import, but no linked object or import library defines it");
          lnk_supplement_error("verify that the response file includes the import library for this module dependency");
        }
        lnk_supplement_error_list(ref_messages);
      }

      if (!config->force) {
        lnk_exit(LNK_Error_UnresolvedSymbol);
      }

      temp_end(debug_scratch);
    }

    ProfEnd();
  }

  {
    LNK_Obj **objs = 0;

    //
    // discard COMDAT sections that are not referenced
    //
    if (config->opt_ref == LNK_SwitchState_Yes) {
      if (objs == 0) { objs = lnk_array_from_obj_list(scratch.arena, link->objs); }
      lnk_summary_phase_begin(LNK_SummaryPhase_Ref);
      lnk_opt_ref(tp, symtab, config, objs, link->objs.count);
      lnk_summary_phase_end(LNK_SummaryPhase_Ref);
    }

    //
    // fold duplicate sections
    //
    if (config->opt_icf == LNK_SwitchState_Yes) {
      if (objs == 0) { objs = lnk_array_from_obj_list(scratch.arena, link->objs); }
      lnk_summary_phase_begin(LNK_SummaryPhase_Icf);
      lnk_opt_icf(tp, arena->v[0], symtab, config, objs, link->objs.count);
      lnk_summary_phase_end(LNK_SummaryPhase_Icf);
    }

    //
    // keep line info for ICF-folded functions (bound to the leader RVA) -- see task comment
    //
    if (config->opt_icf == LNK_SwitchState_Yes && config->opt_ref == LNK_SwitchState_Yes) {
      if (objs == 0) { objs = lnk_array_from_obj_list(scratch.arena, link->objs); }
      lnk_icf_mark_folded_lines(tp, arena, objs, link->objs.count);
    }
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
    for (LNK_ObjNode *obj_n = link->objs.first; obj_n != 0; obj_n = obj_n->next) { total_input_size += obj_n->data.coff.data.size; }
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

internal U32Array *
lnk_obj_indices_from_section_counts(Arena *arena, U64 worker_count, LNK_Obj **objs, U64 objs_count)
{
  Temp scratch = scratch_begin(&arena, 1);

  U64      *worker_section_counts = push_array(scratch.arena, U64, worker_count);
  U64      *worker_obj_counts     = push_array(scratch.arena, U64, worker_count);
  U32Array *obj_indices           = push_array(arena, U32Array, worker_count);

  for EachIndex(obj_idx, objs_count) {
    U64 min_worker_idx = 0;
    for (U64 worker_idx = 1; worker_idx < worker_count; worker_idx += 1) {
      if (worker_section_counts[worker_idx] < worker_section_counts[min_worker_idx]) {
        min_worker_idx = worker_idx;
      }
    }

    worker_section_counts[min_worker_idx] += objs[obj_idx]->coff.sections.count_no_null;
    worker_obj_counts[min_worker_idx]     += 1;
  }

  for EachIndex(worker_idx, worker_count) {
    obj_indices[worker_idx].v = push_array_no_zero(arena, U32, worker_obj_counts[worker_idx]);
  }

  MemoryZero(worker_section_counts, sizeof(worker_section_counts[0])*worker_count);
  MemoryZero(worker_obj_counts,     sizeof(worker_obj_counts[0])*worker_count);

  for EachIndex(obj_idx, objs_count) {
    U64 min_worker_idx = 0;
    for (U64 worker_idx = 1; worker_idx < worker_count; worker_idx += 1) {
      if (worker_section_counts[worker_idx] < worker_section_counts[min_worker_idx]) {
        min_worker_idx = worker_idx;
      }
    }

    U32Array *worker_obj_indices = &obj_indices[min_worker_idx];
    worker_obj_indices->v[worker_obj_counts[min_worker_idx]++] = (U32)obj_idx;
    worker_obj_indices->count += 1;
    worker_section_counts[min_worker_idx] += objs[obj_idx]->coff.sections.count_no_null;
  }

  scratch_end(scratch);
  return obj_indices;
}

internal B32
lnk_resolve_reloc_target_symbol(Arena *arena, LNK_SymbolTable *symtab, LNK_ObjSymbolRef symbol, String8 pass_name, LNK_ObjSymbolRef *resolved_symbol_out)
{
  Temp             temp        = temp_begin(arena);
  B32              is_resolved = 1;
  HashMap          seen_hm     = {0};
  LNK_ObjSymbolRef result      = symbol;
  for (;;) {
    // unpack symbol
    COFF_ParsedSymbol          result_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(result.obj, result.symbol_idx);
    COFF_SymbolValueInterpType result_interp = coff_interp_from_parsed_symbol(result_parsed);

    // resolve symbol
    LNK_ObjSymbolRef next_ref = {0};
    if (!lnk_resolve_symbol(symtab, result, &next_ref)) {
      break;
    }
    if (result_interp != COFF_SymbolValueInterp_Weak && result_interp != COFF_SymbolValueInterp_Undefined) {
      result = next_ref;
      break;
    }

    // most relocations resolve in one step; only allocate cycle tracking for chains
    U64 symbol_key = ((U64)result.obj->input_idx << 32ull) | (U64)result.symbol_idx;
    if (hash_map_search_u64_u64(&seen_hm, symbol_key) != 0) {
      COFF_ParsedSymbol symbol_parsed = lnk_parsed_symbol_from_coff_symbol_idx(symbol.obj, symbol.symbol_idx);
      lnk_error_obj(LNK_Warning_CyclicSymbol, symbol.obj, "symbol %S forms a cyclic chain (%S)", symbol_parsed.name, pass_name);
      MemoryZeroStruct(&result);
      is_resolved = 0;
      break;
    }
    hash_map_push_u64_u64(temp.arena, &seen_hm, symbol_key, 1);
    result = next_ref;
  }

  if (resolved_symbol_out) {
    *resolved_symbol_out = result;
  }

  temp_end(temp);
  return is_resolved;
}

internal
THREAD_POOL_TASK_FUNC(lnk_opt_ref_task)
{
  ProfBeginFunction();

  Temp scratch  = scratch_begin(0,0);
  Temp scratch2 = scratch_begin(&scratch.arena, 1);

  LNK_OptTask     *task       = raw_task;
  LNK_SymbolTable *symtab     = task->symtab;
  LNK_Config      *config     = task->config;
  LNK_Obj        **objs       = task->objs;
  U64              objs_count = task->objs_count;

  // "Remove Unreachable Sections" per-task stat accumulators (reduced on task 0 for the log)
  typedef struct { U64 vsize; U64 fsize; U64 section_count; } LNK_OptRefStat;
  enum { LNK_OptRefStat_Null, LNK_OptRefStat_Code, LNK_OptRefStat_Data, LNK_OptRefStat_Debug, LNK_OptRefStat_Count };

  U8                    **is_live             = 0;
  LNK_Obj               **objs_by_idx         = 0; // input_idx -> obj, for the strided removal pass
  U64                    *active_thread_count = 0;
  LNK_RelocRefsBatchList  *global_batch_list   = 0;
  LNK_OptRefStat          *remove_stats        = 0;
  if (task_id == 0) {
    remove_stats = push_array(scratch.arena, LNK_OptRefStat, LNK_OptRefStat_Count * tp->worker_count);
    active_thread_count = push_array(scratch.arena, U64,                   1);
    global_batch_list   = push_array(scratch.arena, LNK_RelocRefsBatchList, 1);

    // alloc live flags and set live status on every non-COMDAT section
    is_live     = push_array_no_zero(scratch.arena, U8 *,      objs_count);
    objs_by_idx = push_array_no_zero(scratch.arena, LNK_Obj *, objs_count ? objs_count : 1);
    {
      for EachIndex(obj_idx, objs_count) {
        LNK_Obj *obj = objs[obj_idx];

        is_live[obj_idx]     = push_array(scratch.arena, U8, obj->coff.sections.count_no_null + 1);
        objs_by_idx[obj_idx] = obj;

        for LNK_EachCoffSection(it, obj) {
          is_live[obj_idx][it.v.section_number] = !(*it.v.flags & COFF_SectionFlag_LnkCOMDAT);
        }
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
        if (root == 0) { continue; }
        LNK_ObjSymbolRef  root_ref = lnk_ref_from_symbol(root);

        LNK_RelocRefs r = {0};
        r.obj                 = root_ref.obj;
        r.relocs.count        = 1;
        r.relocs.v            = push_array(scratch.arena, COFF_Reloc, 1);
        r.relocs.v[0].isymbol = root_ref.symbol_idx;

        lnk_reloc_ref_batch_list_push(scratch.arena, global_batch_list, r);
      }

      // push task for every non-COMDAT section
      for EachIndex(obj_idx, objs_count) {
        LNK_Obj *obj = objs[obj_idx];

        for LNK_EachCoffSection(it, obj) {
          COFF_SectionFlags section_flags = *it.v.flags;

          // is section eligible for walking?
          if (section_flags & COFF_SectionFlag_LnkRemove)  { continue; }
          if (section_flags & COFF_SectionFlag_LnkCOMDAT)  { continue; }
          if (section_flags & COFF_SectionFlag_LnkInfo)    { continue; }
          if (section_flags & LNK_SECTION_FLAG_DEBUG)      { continue; }

          // divide relocs and push task for each reloc block
          COFF_RelocArray  relocs           = lnk_coff_reloc_info_from_section_number(obj, it.v.section_number);
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
  tp_broadcast(&remove_stats);

  // Per-worker memo: (obj input idx << 32 | coff symbol idx) -> final ref of the reloc-symbol
  // resolve chain below. A symbol is referenced by one reloc per call site, so the chain
  // (interp parse + symbol-table trie search per hop) otherwise repeats for identical inputs
  // millions of times. Open-addressing and lossy (a collision past the probe window evicts);
  // a miss only costs the recompute -- the cached value is a pure function of the key because
  // the symbol table and parsed_symbols are read-only during /OPT:REF.
  // Keep the table small enough that initializing every worker does not become a page-fault
  // amplifier. Pack the resolved object input index + symbol index into one U64 and recover the
  // object through objs_by_idx: 32K slots is 512KiB per worker (versus 768KiB with a pointer and
  // widened symbol index, or 24MiB at the old 1M-slot size).
  typedef struct { U64 key; U64 ref; } LNK_RefResolveSlot;
  U64                 resolve_cache_mask = (1ull << 15) - 1;
  LNK_RefResolveSlot *resolve_cache      = push_array_no_zero(scratch.arena, LNK_RefResolveSlot, resolve_cache_mask + 1);
  MemorySet(resolve_cache, 0xff, sizeof(resolve_cache[0]) * (resolve_cache_mask + 1));

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
            // resolve-cache lookup
            U64 cache_key  = ((U64)ref_symbol.obj->input_idx << 32ull) | (U64)ref_symbol.symbol_idx;
            U64 cache_hash = cache_key * 0x9E3779B97F4A7C15ull; cache_hash ^= cache_hash >> 32;
            U64 cache_slot = max_U64;
            B32 cache_hit  = 0;
            for (U64 probe_idx = 0; probe_idx < 8; probe_idx += 1) {
              U64 slot = (cache_hash + probe_idx) & resolve_cache_mask;
              if (resolve_cache[slot].key == cache_key) {
                U64 packed_ref = resolve_cache[slot].ref;
                ref_symbol = packed_ref != max_U64 ? (LNK_ObjSymbolRef){ .obj = objs_by_idx[packed_ref >> 32], .symbol_idx = (U32)packed_ref }
                                                   : (LNK_ObjSymbolRef){0};
                cache_hit  = 1;
                break;
              }
              if (resolve_cache[slot].key == max_U64) { cache_slot = slot; break; }
            }

            if (!cache_hit) {
              // cycle detection via linear scan of the visited chain: chains are 1-3 hops in
              // practice, and this keeps the tree HashMap + per-hop arena pushes off the hot path
              // (exact same first-revisit semantics)
              U64  chain_fixed[64];
              U64 *chain       = chain_fixed;
              U64  chain_count = 0;
              U64  chain_cap   = ArrayCount(chain_fixed);
              B32  was_cyclic  = 0;

              Temp temp         = temp_begin(scratch2.arena);
              B32  keep_walking = 1;
              do {
                // detect cyclic chains
                U64 symbol_key = ((U64)ref_symbol.obj->input_idx << 32ull) | (U64)ref_symbol.symbol_idx;
                B32 was_seen   = 0;
                for EachIndex(chain_idx, chain_count) {
                  if (chain[chain_idx] == symbol_key) { was_seen = 1; break; }
                }
                if (!was_seen) {
                  if (chain_count == chain_cap) {
                    U64 *new_chain = push_array_no_zero(temp.arena, U64, chain_cap * 2);
                    MemoryCopy(new_chain, chain, sizeof(chain[0]) * chain_count);
                    chain = new_chain; chain_cap *= 2;
                  }
                  chain[chain_count++] = symbol_key;
                } else {
                  COFF_ParsedSymbol reloc_parsed = lnk_parsed_symbol_from_coff_symbol_idx(batch->v[i].obj, reloc->isymbol);
                  lnk_error_obj(LNK_Warning_CyclicSymbol, batch->v[i].obj, "symbol %S forms a cyclic chain (/OPT:REF)", reloc_parsed.name);
                  MemoryZeroStruct(&ref_symbol);
                  was_cyclic = 1;
                  break;
                }

                // unpack symbol (interp needs no name decode)
                COFF_ParsedSymbol          ref_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(ref_symbol.obj, ref_symbol.symbol_idx);
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

              // memoize (skip the cyclic-warning path so the warning replays per reloc as before)
              if (!was_cyclic) {
                if (cache_slot == max_U64) { cache_slot = cache_hash & resolve_cache_mask; }
                U64 packed_ref = ref_symbol.obj ? Compose64Bit(ref_symbol.obj->input_idx, ref_symbol.symbol_idx) : max_U64;
                resolve_cache[cache_slot] = (LNK_RefResolveSlot){ .key = cache_key, .ref = packed_ref };
              }
            }
          }

          // skip unresolved symbol
          if (ref_symbol.obj == 0) { continue; }

          // unpack resolved symbol (only interp + section_number are used -- skip the name decode)
          COFF_ParsedSymbol           ref_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(ref_symbol.obj, ref_symbol.symbol_idx);
          COFF_SymbolValueInterpType  ref_interp = coff_interp_from_parsed_symbol(ref_parsed);

          if (ref_interp == COFF_SymbolValueInterp_Regular) {
            Temp temp = temp_begin(scratch2.arena);

            LNK_Obj *walk_obj      = ref_symbol.obj;
            U32      seed_sn       = ref_parsed.section_number;

            // per-walk visited set + walk stack: flat arrays with linear-scan membership --
            // associative groups are a handful of sections, and the tree HashMap + per-node
            // arena pushes dominated this walk
            U32  visited_fixed[64];
            U32 *visited       = visited_fixed;
            U64  visited_count = 0;
            U64  visited_cap   = ArrayCount(visited_fixed);
            U32  stack_fixed[64];
            U32 *stack       = stack_fixed;
            U64  stack_count = 0;
            U64  stack_cap   = ArrayCount(stack_fixed);
            stack[stack_count++] = seed_sn;
            do {
              U32 section_number = stack[--stack_count];

              // detect cyclic associative sections
              {
                B32 was_seen = 0;
                for EachIndex(visited_idx, visited_count) {
                  if (visited[visited_idx] == section_number) { was_seen = 1; break; }
                }
                if (was_seen) { continue; }
                if (visited_count == visited_cap) {
                  U32 *new_visited = push_array_no_zero(temp.arena, U32, visited_cap * 2);
                  MemoryCopy(new_visited, visited, sizeof(visited[0]) * visited_count);
                  visited = new_visited; visited_cap *= 2;
                }
                visited[visited_count++] = section_number;
              }

              // push associated section
              U32Array associated_sections = lnk_obj_associated_sections_from_section_number(walk_obj, section_number);
              for EachIndex(associated_idx, associated_sections.count) {
                U32 assoc_sn = associated_sections.v[associated_idx];


                {
                  B32 assoc_seen = 0;
                  for EachIndex(visited_idx, visited_count) {
                    if (visited[visited_idx] == assoc_sn) { assoc_seen = 1; break; }
                  }
                  if (assoc_seen) { continue; }
                }
                if (stack_count == stack_cap) {
                  U32 *new_stack = push_array_no_zero(temp.arena, U32, stack_cap * 2);
                  MemoryCopy(new_stack, stack, sizeof(stack[0]) * stack_count);
                  stack = new_stack; stack_cap *= 2;
                }
                stack[stack_count++] = assoc_sn;
              }

              COFF_SectionFlags section_flags = ref_symbol.obj->coff.sections.headers[section_number].flags;

              // on first section visit, set live flag and enqueue section
              // (plain read first -- most targets are already live; the read keeps the flag
              //  cacheline shared instead of dirtying it with an unconditional exchange)
              U8 was_visited = *(volatile U8 *)&is_live[walk_obj->input_idx][section_number];
              if (!was_visited) { was_visited = ins_atomic_u8_eval_assign(&is_live[walk_obj->input_idx][section_number], 1); }
              if (was_visited) { continue; }

              // is section eligible for walking?
              if (section_flags & COFF_SectionFlag_LnkRemove) { continue; }
              if (section_flags & COFF_SectionFlag_LnkInfo)   { continue; }
              if (section_flags & LNK_SECTION_FLAG_DEBUG)     { continue; }

              LNK_RelocRefs refs = {0};
              refs.obj    = ref_symbol.obj;
              refs.relocs = lnk_coff_reloc_info_from_section_number(ref_symbol.obj, section_number);

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

            } while (stack_count);

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

  // Remove unreachable sections. Section flags are per-obj (disjoint writes), so the obj list is
  // strided across tasks via objs_by_idx; stats accumulate per task and are reduced on task 0, so
  // the debug log totals are identical regardless of cohort width or schedule.
  {
    ProfBegin("Remove Unreachable Sections");
    LNK_OptRefStat *stats = remove_stats + task_id * LNK_OptRefStat_Count;
    for (U64 obj_idx = task_id; obj_idx < objs_count; obj_idx += tp->worker_count) {
      LNK_Obj *obj = objs_by_idx[obj_idx];

      for LNK_EachCoffSection(it, obj) {
        U32 section_number = it.v.section_number;
        if (is_live[obj->input_idx][section_number]) { continue; }

        COFF_SectionHeader *section_header = it.v.header;
        *it.v.flags |= COFF_SectionFlag_LnkRemove;
        COFF_SectionFlags section_flags = *it.v.flags;

        U64 stat_kind = LNK_OptRefStat_Null;
        if      (section_flags & LNK_SECTION_FLAG_DEBUG)   { stat_kind = LNK_OptRefStat_Debug; }
        else if (section_flags & COFF_SectionFlag_CntCode) { stat_kind = LNK_OptRefStat_Code;  }
        else                                                { stat_kind = LNK_OptRefStat_Data;  }

        if (section_flags & COFF_SectionFlag_CntUninitializedData) {
          stats[stat_kind].vsize += section_header->vsize;
        } else {
          stats[stat_kind].fsize += section_header->fsize;
        }
        stats[stat_kind].section_count += 1;
      }
    }
    ProfEnd();
  }
  barrier_wait(tp->barrier);

  if (task_id == 0 && lnk_get_log_status(LNK_Log_Debug)) {
    LNK_OptRefStat stats[LNK_OptRefStat_Count] = {0};
    for EachIndex(reduce_task_idx, tp->worker_count) {
      for EachIndex(stat_idx, (U64)LNK_OptRefStat_Count) {
        stats[stat_idx].vsize         += remove_stats[reduce_task_idx * LNK_OptRefStat_Count + stat_idx].vsize;
        stats[stat_idx].fsize         += remove_stats[reduce_task_idx * LNK_OptRefStat_Count + stat_idx].fsize;
        stats[stat_idx].section_count += remove_stats[reduce_task_idx * LNK_OptRefStat_Count + stat_idx].section_count;
      }
    }

    U64 total_fsize = 0, total_section_count = 0;
    for EachElement(i, stats) {
      total_fsize         += stats[i].fsize;
      total_section_count += stats[i].section_count;
    }
    String8List stat_list = {0};
    str8_list_pushf(scratch.arena, &stat_list, "Code : %M, %S sections", stats[LNK_OptRefStat_Code].fsize,  str8_from_count(scratch.arena, stats[LNK_OptRefStat_Code].section_count ));
    str8_list_pushf(scratch.arena, &stat_list, "Data : %M, %S sections", stats[LNK_OptRefStat_Data].fsize,  str8_from_count(scratch.arena, stats[LNK_OptRefStat_Data].section_count ));
    str8_list_pushf(scratch.arena, &stat_list, "Debug: %M, %S sections", stats[LNK_OptRefStat_Debug].fsize, str8_from_count(scratch.arena, stats[LNK_OptRefStat_Debug].section_count));
    str8_list_pushf(scratch.arena, &stat_list, "Total: %M, %S sections", total_fsize,             str8_from_count(scratch.arena, total_section_count));
    String8 stat_str = str8_list_join(scratch.arena, &stat_list, &(StringJoin){.pre = str8_lit("  "), .sep = str8_lit("\n  ")});
    lnk_log(LNK_Log_Debug, "/OPT:REF Stats:\n%S", stat_str);
  }

  scratch_end(scratch2);
  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_opt_ref(TP_Context *tp, LNK_SymbolTable *symtab, LNK_Config *config, LNK_Obj **objs, U64 objs_count)
{
  ProfBegin("/OPT:REF");
  Temp scratch = scratch_begin(0,0);
  // BARRIER pass (path B): the task synchronizes with barrier_wait(tp->barrier)/tp_broadcast,
  // so under /RAD_SHARED_THREAD_POOL it must run at a pinned cohort via the reserve path --
  // a plain tp_for_parallel admits workers incrementally and the barrier never fills (deadlock).
  // Pin the cohort BEFORE sizing the per-lane obj distribution so both agree on the width.
  U32 C = tp_barrier_begin(tp);
  U32Array *obj_indices = lnk_obj_indices_from_section_counts(scratch.arena, C, objs, objs_count);
  LNK_OptTask task = { .symtab = symtab, .config = config, .objs = objs, .objs_count = objs_count, .obj_indices = obj_indices };
  tp_for_parallel_reserve(tp, 0, C, lnk_opt_ref_task, &task); // BARRIER pass (path B)
  tp_barrier_end(tp);
  scratch_end(scratch);
  ProfEnd();
}

#define LNK_ICF_Scope_XList \
  X(Null)                   \
  X(Code)                   \
  X(Unwind)                 \
  X(VFTable)                \
  X(ConstData) // string literals, float consts, const tables (/Gw and /GF)

typedef enum LNK_ICF_Scope
{
#define X(ID) LNK_ICF_Scope_##ID,
  LNK_ICF_Scope_XList
  LNK_ICF_Scope_COUNT
#undef X
} LNK_ICF_Scope;

internal LNK_ICF_Scope
lnk_icf_scope_from_section_number(LNK_Obj *obj, U32 section_number)
{
  LNK_ICF_Scope result = LNK_ICF_Scope_Null;

  //
  // * section flags filter *
  //
  COFF_SectionFlags expected_flags = COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_MemRead;
  COFF_SectionFlags exclude_flags  = COFF_SectionFlag_LnkRemove | COFF_SectionFlag_MemWrite | LNK_SECTION_FLAG_NOICF;
  if ((obj->coff.sections.headers[section_number].flags & expected_flags) != expected_flags || (obj->coff.sections.headers[section_number].flags & exclude_flags) != 0) {
    goto exit;
  }

  if (obj->coff.sections.headers[section_number].flags & COFF_SectionFlag_CntCode) {
    result = LNK_ICF_Scope_Code;
    goto exit;
  }

  if (obj->coff.sections.headers[section_number].flags & COFF_SectionFlag_CntInitializedData) {
    COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(obj, section_number);
    String8             section_name   = str8_cstring_capped(section_header->name, section_header->name + sizeof(section_header->name));

    //
    // * include unwind info metadata *
    //
    if (str8_match(section_name, str8_lit(".xdata"), 0) || str8_match(section_name, str8_lit(".pdata"), 0)) {
      result = LNK_ICF_Scope_Unwind;
      goto exit;
    }

    // query COMDAT symlink that is associated with the section number
    // because the properties are stored in the symbol table
    LNK_ObjSymbolRef comdat_ref = {0};
    if (lnk_obj_get_comdat_symlink_from_section_number(obj, section_number, &comdat_ref)) {

      // load COMDAT symbol name
      String8 comdat_name = lnk_symbol_name_from_coff_symbol_idx(comdat_ref.obj, comdat_ref.symbol_idx);

      //
      // * include MSVC C++ EH tables *
      //
      if (str8_starts_with(comdat_name, str8_lit("$cppxdata$")) ||
          str8_starts_with(comdat_name, str8_lit("$stateUnwindMap$")) ||
          str8_starts_with(comdat_name, str8_lit("$ip2state$"))) {
        result = LNK_ICF_Scope_Unwind;
        goto exit;
      }

      //
      // * include virtual function tables *
      //
      if (str8_starts_with(comdat_name, str8_lit(MSCRT_VFTABLE_SYMBOL_PREFIX))) {
        result = LNK_ICF_Scope_VFTable;
        goto exit;
      } 

      // exclude static COMDAT data
      COFF_ParsedSymbol comdat_symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(comdat_ref.obj, comdat_ref.symbol_idx);
      if (comdat_symbol.storage_class != COFF_SymStorageClass_External) {
        goto exit;
      }

      // exclude general read-only data records with fixups
      COFF_SectionHeader *header = lnk_coff_section_header_from_section_number(obj, section_number);
      if (lnk_coff_relocs_from_section_header(obj, header).count > 0) {
        goto exit;
      }

      //
      // * include COMDATs *
      //
      COFF_ComdatSelectType select = COFF_ComdatSelect_Null;
      if (lnk_try_comdat_props_from_section_number(obj, section_number, &select, 0, 0, 0)) {
        // TODO: ref linkers exclude C++ virtual base tables, not sure why,
        // are there tools that assume vbptr is unique for _some_reason_?
        if (str8_starts_with(comdat_name, str8_lit(MSCRT_VBTABLE_SYMBOL_PREFIX))) {
          goto exit;
        }

        // following selections are not included in ICF
        //  1. NoDuplicate: selection requires a unique address
        //  2. Associative: breaks COMDAT ownership model
        if (select == COFF_ComdatSelect_Any        ||
            select == COFF_ComdatSelect_SameSize   ||
            select == COFF_ComdatSelect_ExactMatch ||
            select == COFF_ComdatSelect_Largest) {
          result = LNK_ICF_Scope_ConstData;
          goto exit;
        }
      }
    }
  }

  exit:;
  Assert(result == LNK_ICF_Scope_Null || obj->coff.sections.comdats[section_number] != max_U32); // TODO: are symlinks on COMDATs optional?
  return result;
}

internal void
lnk_icf_atomic_min_u32(U32 *dst, U32 value)
{
  // preserve stable leaders despite concurrent insertion
  for (U32 old_value = ins_atomic_u32_eval(dst); value < old_value;) {
    U32 observed = ins_atomic_u32_eval_cond_assign(dst, value, old_value);
    if (observed == old_value) { break; }
    old_value = observed;
  }
}

// NOTE: OPT uses a color-refinement algorithm for folding duplicate sections.
// If a color group contains multiple distinct hashes, the group is split
// and a new refinement round is run. By default, the algorithm loops until
// partitions stabilize. Equivalence is established by comparing cryptographic
// 128-bit hashes; in theory, the chance of collisions are near the birthday
// paradox with BLAKE3
THREAD_POOL_TASK_FUNC(lnk_opt_icf_task)
{
  ProfBeginFunction();

  typedef struct { U128 hash; U64 old_color; } ColorKey;

  // only target colors vary between rounds, so cache non-recursive relocation data
  // and rehash target colors each round
  typedef struct {
    union {
      U64 *color;
      U64  static_id;
    };
    U64 association_id;
    U32 value;
    COFF_SymbolValueInterpType interp;
  } RelocTarget;

  // Contribution and table indices are bounded to U32 below. Keeping the hot contribution
  // record at 64 bytes cuts both its footprint and the demand-zero work during ICF.
  typedef struct {
    ColorKey            key;
    U128                static_hash;
    RelocTarget        **reloc_targets;
    U32                 color_slot_idx;
    U32                 reloc_count;
    U32                 obj_idx;
    U32                 section_number;
  } Contrib;

  // Reuse both tables without clearing them between refinement rounds. U32 generations and
  // indices keep these records at 32 and 16 bytes respectively. The 128-bit group hash is dead
  // before old-color indexing, so that phase stores its U32 slot index in the hash bytes. The
  // old-color key is then dead after split counting, so assignment overwrites it with the output
  // color. These lifetime reuses avoid carrying either result through the 16M-slot table.
  typedef struct {
    ColorKey key;
    U32      state; // generation << 2 | (0 = empty, 1 = initializing, 2 = ready)
    U32      first_contrib_idx;
  } ColorHashSlot;

  typedef struct { ColorHashSlot *slots; U64 slots_count; } ColorHashTable;

  typedef struct {
    U64 old_color;
    U32 state; // generation << 2 | (0 = empty, 1 = initializing, 2 = ready)
    U32 first_contrib_idx;
  } OldColorHashSlot;

  typedef struct { OldColorHashSlot *slots; U64 slots_count; } OldColorHashTable;

  Temp scratch  = scratch_begin(&arena,1);
  Temp scratch2 = scratch_begin(&scratch.arena,1); // retain relocation metadata through temporary associated-section traversals

  LNK_OptTask   *task          = raw_task;
  LNK_Obj      **objs          = task->objs;
  LNK_HashKind   icf_hash_kind = task->config->icf_hash_kind;

  if (task->config->llvm_addrsig == LNK_SwitchState_Yes) {
    ProfBegin("Flag significant sections");

    // .llvm_addrsig is an array of ULEB128 symbol indices, which mark sections
    // whose addresses are significant
    for EachIndex(i, task->obj_indices[task_id].count) {
      U64      obj_idx = task->obj_indices[task_id].v[i];
      LNK_Obj *obj     = task->objs[obj_idx];

      if (obj->coff.llvm_addrsig_section_number == 0) { continue; }

      // parse symbol indices and mark selected sections with NOICF flag,
      // the selected symbols maybe undefined/weak which need to be resolved
      String8 section_data = lnk_obj_section_data_from_number(obj, obj->coff.llvm_addrsig_section_number);
      for (U64 off = 0; off < section_data.size;) {
        U64 symbol_off = off;
        U64 symbol_idx = 0;
        off += str8_deserial_read_uleb128(section_data, off, &symbol_idx);
        if (symbol_off == off) { break; }

        if (symbol_idx < obj->coff.header.symbol_count) {
          LNK_ObjSymbolRef target_ref      = { .obj = obj, .symbol_idx = symbol_idx };
          B32              is_symbol_found = lnk_resolve_reloc_target_symbol(scratch.arena, task->symtab, target_ref, str8_lit("/OPT:ICF"), &target_ref);
          if (is_symbol_found) {
            COFF_ParsedSymbol symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(target_ref.obj, target_ref.symbol_idx);
            if (coff_interp_from_parsed_symbol(symbol) == COFF_SymbolValueInterp_Regular) {
              target_ref.obj->coff.sections.headers[symbol.section_number].flags |= LNK_SECTION_FLAG_NOICF;
            }
          } else {
            COFF_ParsedSymbol original_symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(obj, symbol_idx);
            if (coff_interp_from_parsed_symbol(original_symbol) == COFF_SymbolValueInterp_Regular) {
              obj->coff.sections.headers[original_symbol.section_number].flags |= LNK_SECTION_FLAG_NOICF;
            } else {
              lnk_log(LNK_Log_Debug, "%S: .llvm_addrsig: contains an unresolved symbol index 0x%x at offset 0x%x", lnk_loc_from_obj(scratch.arena, obj), symbol_idx, symbol_off);
            }
          }
        } else {
          lnk_error_obj(LNK_Error_IllData, obj, ".llvm_addrsig: contains out of bounds symbol index 0x%x at offset 0x%x\n", symbol_idx, symbol_off);
        }
      }
    }
    ProfEnd();
    barrier_wait(tp->barrier);
  }

  //
  // step 1: fill out color map and contributions
  //

  COFF_SectionFlags associated_filter = COFF_SectionFlag_LnkRemove | COFF_SectionFlag_LnkInfo | COFF_SectionFlag_MemDiscardable | LNK_SECTION_FLAG_DEBUG;

  ProfBegin("Flag Unsupported Associative Sections");
  for EachIndex(i, task->obj_indices[task_id].count) {
    U64      obj_idx = task->obj_indices[task_id].v[i];
    LNK_Obj *obj     = objs[obj_idx];

    for LNK_EachCoffSection(it, obj) {
      COFF_SectionHeader *header = lnk_coff_section_header_from_section_number(obj, it.v.section_number);

      if ((header->flags & (COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_CntCode)) != (COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_CntCode)) {
        continue;
      }

      Temp temp = temp_begin(scratch.arena);
      U32List associated_sections = lnk_obj_collect_associated_section_numbers(temp.arena, obj, it.v.section_number, associated_filter);
      for EachNode(section_n, U32Node, associated_sections.first) {
        String8 section_name = lnk_obj_section_name_from_section_number(obj, section_n->data);
        if (str8_starts_with(section_name, str8_lit(".gsspr$"))) {
          header->flags |= LNK_SECTION_FLAG_NOICF;
          break;
        }
      }
      temp_end(temp);
    }
  }
  ProfEnd();

  // alloc total section counter
  U64 *contrib_counts = 0;
  if (task_id == 0) {
    contrib_counts = push_array(scratch.arena, U64, task->objs_count);
  }
  tp_broadcast(&contrib_counts);

  ProfBegin("Count Contributions");
  for EachIndex(i, task->obj_indices[task_id].count) {
    U64      obj_idx = task->obj_indices[task_id].v[i];
    LNK_Obj *obj     = objs[obj_idx];
    for LNK_EachCoffSection(it, obj) {
      if (lnk_icf_scope_from_section_number(obj, it.v.section_number)) {
        contrib_counts[obj_idx] += 1;
      }
    }
  }
  ProfEnd();
  barrier_wait(tp->barrier);

  struct Shared {
    U64       contrib_count;
    U64      *noncontrib_offsets;
    U64      *contrib_offsets;
    U64     **color_map;
    Contrib  *contribs;
    Rng1U64  *contrib_ranges;
    U64      *split_counts;
    U64      *split_offsets;
    U32      *is_part_stable;
    U64      *next_color;
  } shared = {
    .contrib_count = sum_array_u64(task->objs_count, contrib_counts)
  };
  Assert(shared.contrib_count <= max_U32);
  if (task_id == 0) {
    ProfBegin("Init");

    shared.noncontrib_offsets = push_array(scratch.arena, U64, task->objs_count);
    U64 noncontrib_count = 0;
    for EachIndex(obj_idx, task->objs_count) {
      shared.noncontrib_offsets[obj_idx] = noncontrib_count;
      noncontrib_count += objs[obj_idx]->coff.sections.count_no_null - contrib_counts[obj_idx];
    }

    shared.color_map = push_array(scratch.arena, U64 *, task->objs_count);
    for EachIndex(obj_idx, task->objs_count) {
      LNK_Obj *obj = objs[obj_idx];
      shared.color_map[obj_idx] = push_array(scratch.arena, U64, obj->coff.sections.count_no_null + 1);
    }

    shared.contrib_offsets = offsets_from_counts_array_u64(scratch.arena, contrib_counts, task->objs_count);
    shared.contribs        = push_array(scratch.arena, Contrib, shared.contrib_count);
    shared.contrib_ranges  = tp_divide_work(scratch.arena, shared.contrib_count, tp->worker_count);
    shared.split_counts    = push_array(scratch.arena, U64, tp->worker_count);
    shared.split_offsets   = push_array(scratch.arena, U64, tp->worker_count + 1);
    shared.is_part_stable  = push_array(scratch.arena, U32, 1);
    shared.next_color      = push_array(scratch.arena, U64, 1);
    *shared.next_color     = LNK_ICF_Scope_COUNT + noncontrib_count;

    lnk_log(LNK_Log_Debug, "  Contrib count: %S", str8_from_count(scratch.arena, shared.contrib_count));

    ProfEnd();
  }
  tp_broadcast(&shared);

  ProfBegin("Compute Hashes");
  HashMap reloc_target_hm = {0}; // cache source-symbol resolution so refinement only reads colors and hashes
  for EachIndex(i, task->obj_indices[task_id].count) {
    U64      obj_idx           = task->obj_indices[task_id].v[i];
    LNK_Obj *obj               = objs[obj_idx];
    U64      cursor            = 0;
    U64      noncontrib_cursor = 0;
    for LNK_EachCoffSection(it, obj) {
      LNK_ICF_Scope scope = lnk_icf_scope_from_section_number(obj, it.v.section_number);
      if (scope == LNK_ICF_Scope_Null) {
        // assign colors in object order to avoid a contended atomic allocator
        shared.color_map[obj_idx][it.v.section_number] = LNK_ICF_Scope_COUNT + shared.noncontrib_offsets[obj_idx] + noncontrib_cursor++;
        continue;
      }

      // compute contribution index
      U64      contrib_idx = shared.contrib_offsets[obj_idx] + cursor++;
      Contrib *contrib     = &shared.contribs[contrib_idx];
      *contrib = (Contrib){
        .obj_idx        = safe_cast_u32(obj->input_idx),
        .section_number = safe_cast_u32(it.v.section_number),
      };

      Temp temp = temp_begin(scratch.arena);
      // include associative children in their parent COMDAT's identity
      U32List           associated_sections = lnk_obj_collect_associated_section_numbers(temp.arena, obj, it.v.section_number, associated_filter);
      u32_list_push(temp.arena, &associated_sections, it.v.section_number);

      U64 reloc_count = 0;
      for EachNode(associated_n, U32Node, associated_sections.first) {
        COFF_SectionHeader *associated_header = lnk_coff_section_header_from_section_number(obj, associated_n->data);
        COFF_RelocArray     associated_relocs = lnk_coff_relocs_from_section_header(obj, associated_header);
        reloc_count += associated_relocs.count;
      }
      contrib->reloc_count = safe_cast_u32(reloc_count);
      if (contrib->reloc_count) {
        contrib->reloc_targets = push_array(scratch2.arena, RelocTarget *, contrib->reloc_count);
      }

      LNK_Hasher hasher; lnk_hasher_init(&hasher, icf_hash_kind);
      lnk_hasher_update_struct(&hasher, &scope);

      U64 reloc_cursor = 0;
      for EachNode(associated_n, U32Node, associated_sections.first) {
        COFF_SectionHeader *associated_header = lnk_coff_section_header_from_section_number(obj, associated_n->data);
        String8             associated_data   = lnk_obj_section_data_from_number(obj, associated_n->data);
        COFF_RelocArray     associated_relocs = lnk_coff_relocs_from_section_header(obj, associated_header);

        lnk_hasher_update_string(&hasher, associated_data);
        lnk_hasher_update_struct(&hasher, &associated_relocs.count);

        for EachIndex(reloc_idx, associated_relocs.count) {
          COFF_Reloc *r = &associated_relocs.v[reloc_idx];

          U64          reloc_key = Compose64Bit(obj->input_idx, r->isymbol);
          RelocTarget *target    = hash_map_search_u64_raw(&reloc_target_hm, reloc_key);
          if (target == 0) {
            target = push_array(scratch2.arena, RelocTarget, 1);
            *target = (RelocTarget){0};

            LNK_ObjSymbolRef target_ref      = { .obj = obj, .symbol_idx = r->isymbol };
            B32              is_symbol_found = lnk_resolve_reloc_target_symbol(scratch2.arena, task->symtab, target_ref, str8_lit("/OPT:ICF"), &target_ref);
            if (is_symbol_found) {
              COFF_ParsedSymbol target_symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(target_ref.obj, target_ref.symbol_idx);
              target->interp = coff_interp_from_parsed_symbol(target_symbol);
              target->value  = target_symbol.value;

              switch (target->interp) {
              case COFF_SymbolValueInterp_Regular: {
                LNK_Obj *target_obj  = target_ref.obj;
                U32      target_sect = target_symbol.section_number;

                // references between corresponding associative children compare
                // through the owning COMDAT rather than each child's unique identity
                while (lnk_icf_scope_from_section_number(target_obj, target_sect) == LNK_ICF_Scope_Null) {
                  COFF_ComdatSelectType select       = COFF_ComdatSelect_Null;
                  U32                  parent_sect  = 0;
                  if (!lnk_try_comdat_props_from_section_number(target_obj, target_sect, &select, &parent_sect, 0, 0)) {
                    break;
                  }
                  if (select != COFF_ComdatSelect_Associative) {
                    break;
                  }

                  U64 child_pos = 0;
                  U32Array associated_sections = lnk_obj_associated_sections_from_section_number(target_obj, parent_sect);
                  for EachIndex(child_idx, associated_sections.count) {
                    if (associated_sections.v[child_idx] == target_sect) { break; }
                    child_pos += 1;
                  }

                  String8 child_name        = lnk_obj_section_name_from_section_number(target_obj, target_sect);
                  U64     association_key[] = { target->association_id, hash_map_hasher(child_name), child_pos };

                  target->association_id = hash_map_hasher(str8_array_fixed(association_key));
                  target_sect = parent_sect;
                }

                // use the selected COMDAT leader so equivalent targets hash alike
                if (target_obj->coff.sections.headers[target_sect].flags & COFF_SectionFlag_LnkCOMDAT) {
                  LNK_ObjSymbolRef leader_ref = {0};
                  if (lnk_obj_get_comdat_symlink_from_section_number(target_obj, target_sect, &leader_ref)) {
                    COFF_ParsedSymbol leader_symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(leader_ref.obj, leader_ref.symbol_idx);
                    if (leader_symbol.section_number != 0 && leader_symbol.section_number <= leader_ref.obj->coff.sections.count_no_null) {
                      target_obj  = leader_ref.obj;
                      target_sect = leader_symbol.section_number;
                    }
                  }
                }

                target->color = &shared.color_map[target_obj->input_idx][target_sect];
              } break;
              default: {
                target->static_id = Compose64Bit(target_ref.obj->input_idx, target_ref.symbol_idx);
              } break;
              }
            } else {
              target->interp    = max_U32;
              target->static_id = Compose64Bit(obj_idx, r->isymbol);
            }

            hash_map_push_u64_raw(scratch2.arena, &reloc_target_hm, reloc_key, target);
          }

          contrib->reloc_targets[reloc_cursor++] = target;
          lnk_hasher_update_struct(&hasher, &r->apply_off);
          lnk_hasher_update_struct(&hasher, &r->type);
          lnk_hasher_update_struct(&hasher, &target->interp);
          lnk_hasher_update_struct(&hasher, &target->value);
        }
      }
      Assert(reloc_cursor == contrib->reloc_count);
      contrib->static_hash = lnk_hasher_digest128(&hasher);

      // seed foldable sections with their immutable content and relocation shape
      shared.color_map[obj_idx][it.v.section_number] = hash_map_hasher(str8_struct(&contrib->static_hash)) | (1ull << 63);
      temp_end(temp);
    }
  }
  ProfEnd();
  barrier_wait(tp->barrier);

  ColorHashTable    color_table      = {0};
  OldColorHashTable old_color_table  = {0};
  U32              *table_generation = 0;
  if (task_id == 0) {
    ProfBegin("Alloc hash tables");
    color_table.slots_count     = u64_up_to_pow2(Max(2, shared.contrib_count*2));
    Assert(color_table.slots_count <= (U64)max_U32 + 1);
    color_table.slots           = push_array(scratch.arena, ColorHashSlot, color_table.slots_count);
    old_color_table.slots_count = color_table.slots_count;
    old_color_table.slots       = push_array(scratch.arena, OldColorHashSlot, old_color_table.slots_count);
    table_generation            = push_array_no_zero(scratch.arena, U32, 1);
    *table_generation           = 0;
    ProfEnd();
  }
  tp_broadcast(&color_table);
  tp_broadcast(&old_color_table);
  tp_broadcast(&table_generation);

  //
  // step 2: refine equivalence classes
  //

  U64 iter_count = 0;
  for (;; iter_count += 1) {
    ProfBegin("Round #%llu", iter_count);

    barrier_wait(tp->barrier);

    if (task_id == 0) {
      // reset color status tracker
      *shared.is_part_stable = 1;

      // update hash tables generations
      Assert(*table_generation < (max_U32 >> 2));
      *table_generation += 1;
    }
    barrier_wait(tp->barrier);

    // unpack the table generation
    U32 table_generation_value = *table_generation;
    U32 initializing_state     = (table_generation_value << 2) | 1;
    U32 ready_state            = (table_generation_value << 2) | 2;

    ProfBegin("Compute colored hashes");
    for EachInRange(contrib_idx, shared.contrib_ranges[task_id]) {
      Contrib *contrib = &shared.contribs[contrib_idx];
      contrib->key.old_color = shared.color_map[contrib->obj_idx][contrib->section_number];

      // apply colors from section references
      LNK_Hasher hasher; lnk_hasher_init(&hasher, icf_hash_kind);
      lnk_hasher_update_struct(&hasher, &contrib->static_hash);
      for EachIndex(reloc_idx, contrib->reloc_count) {
        RelocTarget *target    = contrib->reloc_targets[reloc_idx];
        U64          target_id = target->interp == COFF_SymbolValueInterp_Regular ? *target->color : target->static_id;
        lnk_hasher_update_struct(&hasher, &target_id);
        lnk_hasher_update_struct(&hasher, &target->association_id);
      }
      U128 hash = lnk_hasher_digest128(&hasher);

      // insert the colored hash into the concurrent table
      contrib->key.hash = hash;

      Assert(color_table.slots_count > 0 && (color_table.slots_count & (color_table.slots_count - 1)) == 0);
      U64 table_hash     = hash_map_hasher(str8_struct(&contrib->key));
      U64 color_slot_idx = table_hash & (color_table.slots_count - 1);
      for (;;) {
        ColorHashSlot *color_slot = &color_table.slots[color_slot_idx];
        U32            state      = ins_atomic_u32_eval(&color_slot->state);

        if ((state >> 2) != table_generation_value) {
          if (ins_atomic_u32_eval_cond_assign(&color_slot->state, initializing_state, state) == state) {
            color_slot->key               = contrib->key;
            color_slot->first_contrib_idx = safe_cast_u32(contrib_idx);
            contrib->color_slot_idx       = safe_cast_u32(color_slot_idx);
            ins_atomic_u32_eval_assign(&color_slot->state, ready_state);
            break;
          }
          continue;
        }

        if (state == initializing_state) {
          do { state = ins_atomic_u32_eval(&color_slot->state); } while (state == initializing_state);
          continue;
        }

        Assert(state == ready_state);

        if (color_slot->key.old_color == contrib->key.old_color && u128_match(color_slot->key.hash, contrib->key.hash)) {
          lnk_icf_atomic_min_u32(&color_slot->first_contrib_idx, safe_cast_u32(contrib_idx));
          contrib->color_slot_idx = safe_cast_u32(color_slot_idx);
          break;
        }

        color_slot_idx = (color_slot_idx + 1) & (color_table.slots_count - 1);
      }
    }
    ProfEnd();
    barrier_wait(tp->barrier);

    // publish one old-color record per color group
    ProfBegin("Index color groups");
    Rng1U64 contrib_range = shared.contrib_ranges[task_id];
    for EachInRange(contrib_idx, contrib_range) {
      Contrib       *contrib = &shared.contribs[contrib_idx];
      ColorHashSlot *slot    = &color_table.slots[contrib->color_slot_idx];
      if (ins_atomic_u32_eval(&slot->first_contrib_idx) == contrib_idx) {
        Assert(old_color_table.slots_count > 0 && (old_color_table.slots_count & (old_color_table.slots_count - 1)) == 0);
        U64 old_color    = slot->key.old_color;
        U64 table_hash   = hash_map_hasher(str8_struct(&old_color));
        U64 old_slot_idx = table_hash & (old_color_table.slots_count - 1);
        for (;;) {
          OldColorHashSlot *old_color_slot = &old_color_table.slots[old_slot_idx];
          U32               state          = ins_atomic_u32_eval(&old_color_slot->state);

          if ((state >> 2) != table_generation_value) {
            if (ins_atomic_u32_eval_cond_assign(&old_color_slot->state, initializing_state, state) == state) {
              old_color_slot->old_color         = old_color;
              old_color_slot->first_contrib_idx = safe_cast_u32(contrib_idx);
              ins_atomic_u32_eval_assign(&old_color_slot->state, ready_state);
              break;
            }
            continue;
          }

          if (state == initializing_state) {
            do { state = ins_atomic_u32_eval(&old_color_slot->state); } while (state == initializing_state);
            continue;
          }

          Assert(state == ready_state);

          if (old_color_slot->old_color == old_color) {
            lnk_icf_atomic_min_u32(&old_color_slot->first_contrib_idx, safe_cast_u32(contrib_idx));
            break;
          }

          old_slot_idx = (old_slot_idx + 1) & (old_color_table.slots_count - 1);
        }
        memory_write32(&slot->key.hash, safe_cast_u32(old_slot_idx));
      }
    }
    ProfEnd();
    barrier_wait(tp->barrier);

    // count split groups in deterministic contribution order
    ProfBegin("Count color splits");
    U64 split_count = 0;
    for EachInRange(contrib_idx, contrib_range) {
      Contrib       *contrib    = &shared.contribs[contrib_idx];
      ColorHashSlot *color_slot = &color_table.slots[contrib->color_slot_idx];
      if (ins_atomic_u32_eval(&color_slot->first_contrib_idx) != contrib_idx) { continue; }

      U32 old_color_slot_idx = memory_read32(&color_slot->key.hash);
      OldColorHashSlot *old_color_slot = &old_color_table.slots[old_color_slot_idx];
      if (ins_atomic_u32_eval(&old_color_slot->first_contrib_idx) != contrib_idx) {
        split_count += 1;
      }
    }
    shared.split_counts[task_id] = split_count;
    ProfEnd();
    barrier_wait(tp->barrier);

    // assign deterministic color ranges with a small serial prefix sum
    if (task_id == 0) {
      ProfBegin("Prefix color splits");

      U64 start_next_color  = *shared.next_color;
      U64 total_split_count = 0;
      for EachIndex(worker_id, tp->worker_count) {
        shared.split_offsets[worker_id] = total_split_count;
        total_split_count += shared.split_counts[worker_id];
      }
      shared.split_offsets[tp->worker_count] = start_next_color;
      
      *shared.next_color += total_split_count;
      *shared.is_part_stable = (total_split_count == 0);

      lnk_log(LNK_Log_Debug, "  Round %llu found %S splits", iter_count, str8_from_count(scratch.arena, total_split_count));

      ProfEnd();
    }
    barrier_wait(tp->barrier);

    // assign old and split colors in deterministic contribution order
    ProfBegin("Assign colors");
    U64 next_split_color = shared.split_offsets[tp->worker_count] + shared.split_offsets[task_id];
    for EachInRange(contrib_idx, contrib_range) {
      Contrib       *contrib    = &shared.contribs[contrib_idx];
      ColorHashSlot *color_slot = &color_table.slots[contrib->color_slot_idx];
      if (ins_atomic_u32_eval(&color_slot->first_contrib_idx) != contrib_idx) { continue; }

      U32 old_color_slot_idx = memory_read32(&color_slot->key.hash);
      OldColorHashSlot *old_color_slot = &old_color_table.slots[old_color_slot_idx];
      if (ins_atomic_u32_eval(&old_color_slot->first_contrib_idx) == contrib_idx) {
        // key.old_color has completed its lookup lifetime; reuse it as the output color
      } else {
        color_slot->key.old_color = ++next_split_color;
      }
    }
    ProfEnd();
    barrier_wait(tp->barrier);

    // update colors for this worker's contributions
    ProfBegin("Update color map");
    for EachIndex(i, task->obj_indices[task_id].count) {
      U64     obj_idx           = task->obj_indices[task_id].v[i];
      Rng1U64 obj_contrib_range = r1u64(shared.contrib_offsets[obj_idx], shared.contrib_offsets[obj_idx] + contrib_counts[obj_idx]);
      for EachInRange(contrib_idx, obj_contrib_range) {
        Contrib *contrib = &shared.contribs[contrib_idx];
        shared.color_map[contrib->obj_idx][contrib->section_number] = color_table.slots[contrib->color_slot_idx].key.old_color;
      }
    }
    ProfEnd();
    barrier_wait(tp->barrier);

    ProfEnd(); // round prof

    // stop iterating when partitions stabilize
    if (*shared.is_part_stable) { break; }
  }
  barrier_wait(tp->barrier);

  //
  // step 3: flag folded sections for removal
  //

  typedef struct { U64 count; U64 size; U64 live_count; U64 live_size; } FoldStats;
  FoldStats *fold_stats = 0;
  if (task_id == 0 && lnk_get_log_status(LNK_Log_Debug)) {
    fold_stats = push_array(scratch.arena, FoldStats, tp->worker_count * LNK_ICF_Scope_COUNT);
  }
  tp_broadcast(&fold_stats);

  ProfBegin("Flag Folds");
  for EachInRange(contrib_idx, shared.contrib_ranges[task_id]) {
    Contrib       *contrib    = &shared.contribs[contrib_idx];
    ColorHashSlot *color_slot = &color_table.slots[contrib->color_slot_idx];
    Contrib       *leader     = &shared.contribs[ins_atomic_u32_eval(&color_slot->first_contrib_idx)];
    
    LNK_Obj *contrib_obj = objs[contrib->obj_idx];
    LNK_Obj *leader_obj  = objs[leader->obj_idx];

    if (fold_stats) {
      FoldStats    *st    = fold_stats + (task_id * LNK_ICF_Scope_COUNT);
      U64           fsize = lnk_coff_section_header_from_section_number(contrib_obj, contrib->section_number)->fsize;
      LNK_ICF_Scope scope = lnk_icf_scope_from_section_number(contrib_obj, contrib->section_number);
      if (leader == contrib) {
        st[scope].live_count += 1;
        st[scope].live_size  += fsize;
      } else {
        st[scope].count += 1;
        st[scope].size  += fsize;
      }
    }

    if (leader == contrib) {
      continue;
    }

    // TODO: double-check if discarded section headers should be updated with leaders' alignment,
    // leaders at this point are resolved
    U64                contrib_align = coff_align_size_from_section_flags(contrib_obj->coff.sections.headers[contrib->section_number].flags);
    COFF_SectionFlags *leader_flags  = &leader_obj->coff.sections.headers[leader->section_number].flags;
    for (COFF_SectionFlags old_flags = ins_atomic_u32_eval((U32 *)leader_flags);;) {
      U64 leader_align = coff_align_size_from_section_flags(old_flags);
      if (leader_align >= contrib_align) { break; }

      COFF_SectionFlags new_flags = old_flags;
      new_flags &= ~(COFF_SectionFlag_AlignMask << COFF_SectionFlag_AlignShift);
      new_flags |= coff_section_flag_from_align_size(contrib_align);

      COFF_SectionFlags observed = ins_atomic_u32_eval_cond_assign((U32 *)leader_flags, new_flags, old_flags);
      if (observed == old_flags) { break; }
      old_flags = observed;
    }

    // remove folded section from the image
    contrib_obj->coff.sections.headers[contrib->section_number].flags |= COFF_SectionFlag_LnkRemove;

    // update folded section COMDAT symlink to point to the leader section
    contrib_obj->symlinks[contrib->section_number] = (LNK_ObjSymbolRef){ leader_obj, leader_obj->coff.sections.comdats[leader->section_number] };

    // remove discarded associated sections
    {
      Temp assoc_temp = temp_begin(scratch.arena);

      U32List  contrib_children = lnk_obj_collect_associated_section_numbers(assoc_temp.arena, contrib_obj, contrib->section_number, associated_filter);
      U32List  leader_children  = lnk_obj_collect_associated_section_numbers(assoc_temp.arena, leader_obj,  leader->section_number,  associated_filter);
      U32Node *contrib_child_n  = contrib_children.first;
      U32Node *leader_child_n   = leader_children.first;

      for (; contrib_child_n && leader_child_n; contrib_child_n = contrib_child_n->next, leader_child_n = leader_child_n->next) {
        U32     contrib_child = contrib_child_n->data;
        U32     leader_child  = leader_child_n->data;
        String8 contrib_name  = lnk_obj_section_name_from_section_number(contrib_obj, contrib_child);
        String8 leader_name   = lnk_obj_section_name_from_section_number(leader_obj, leader_child);

        if ( ! str8_match(contrib_name, leader_name, 0))                                         { continue; }
        if (lnk_icf_scope_from_section_number(contrib_obj, contrib_child) != LNK_ICF_Scope_Null) { continue; }

        contrib_obj->coff.sections.headers[contrib_child].flags |= COFF_SectionFlag_LnkRemove;
        contrib_obj->symlinks[contrib_child] = (LNK_ObjSymbolRef){ leader_obj, leader_obj->coff.sections.comdats[leader_child] };
      }

      temp_end(assoc_temp);
    }

    // record the fold for debug aliasing (lnk_icf_mark_folded_lines): unlike the symlink
    // redirect, this distinguishes an ICF fold (different-named section joined to a leader)
    // from same-name COMDAT selection and /OPT:REF removal
    if (contrib_obj->icf_fold) {
      contrib_obj->icf_fold[contrib->section_number] = (LNK_ICFFold){ .leader_obj_idx = (U32)leader->obj_idx, .leader_sn = leader->section_number, .set = 1 };
    }

    #if LNK_PARANOID
    String8 section_name = lnk_obj_section_name_from_section_number(contrib_obj, contrib->section_number);
    String8 leader_name  = lnk_obj_section_name_from_section_number(leader_obj, leader->section_number);
    lnk_log(LNK_Log_Debug, "fold %.*s[SECT%X \"%.*s\"] ==> %.*s[SECT%X \"%.*s\"]", str8_varg(lnk_loc_from_obj(scratch.arena, contrib_obj)), contrib->section_number, str8_varg(section_name), str8_varg(lnk_loc_from_obj(scratch.arena, leader_obj)), leader->section_number, str8_varg(leader_name));
    #endif
  }
  ProfEnd();
  barrier_wait(tp->barrier);

  if (task_id == 0 && fold_stats) {
    FoldStats stats[LNK_ICF_Scope_COUNT] = {0};
    for EachIndex(worker_id, tp->worker_count) {
      FoldStats *w = fold_stats + (worker_id * LNK_ICF_Scope_COUNT);
      for EachEnumVal(LNK_ICF_Scope, scope) {
        stats[scope].count      += w[scope].count;
        stats[scope].size       += w[scope].size;
        stats[scope].live_size  += w[scope].live_size;
        stats[scope].live_count += w[scope].live_count;
      }
    }

    FoldStats total = {0};
    for EachEnumVal(LNK_ICF_Scope, scope) {
      if (scope == LNK_ICF_Scope_Null) { continue; }

      // scope -> string
      String8 scope_str = str8_lit("Unknown");
      switch (scope) {
#define X(ID) case LNK_ICF_Scope_##ID: scope_str = str8_lit(Stringify(ID)); break;
        LNK_ICF_Scope_XList
#undef X
        default: InvalidPath;
      }

      lnk_log(LNK_Log_Debug, "  %-9S: removed %M, %.*s sections; live %M, %.*s sections",
              scope_str,
              stats[scope].size,
              str8_varg(str8_from_count(scratch.arena, stats[scope].count)),
              stats[scope].live_size,
              str8_varg(str8_from_count(scratch.arena, stats[scope].live_count)));

      total.count      += stats[scope].count;
      total.size       += stats[scope].size;
      total.live_count += stats[scope].live_count;
      total.live_size  += stats[scope].live_size;
    }

    lnk_log(LNK_Log_Debug, "  %-9s: removed %M, %.*s sections; live %M, %.*s sections",
            "Total",
            total.size,
            str8_varg(str8_from_count(scratch.arena, total.count)),
            total.live_size,
            str8_varg(str8_from_count(scratch.arena, total.live_count)));
  }
  barrier_wait(tp->barrier);

  //
  // step 4: flatten COMDAT symlink chains so subsequent passes can assume symlinks are single hop
  //

  ProfBegin("Flatten COMDAT Symbol Links");
  for EachIndex(i, task->obj_indices[task_id].count) {
    U64      obj_idx = task->obj_indices[task_id].v[i];
    LNK_Obj *obj     = objs[obj_idx];
    for LNK_EachCoffSection(it, obj) {
      LNK_ObjSymbolRef symlink_ref = {0};
      if (!lnk_obj_get_comdat_symlink_from_section_number(obj, it.v.section_number, &symlink_ref)) { continue; }

      Temp temp = temp_begin(scratch.arena);
      HashMap seen_hm   = {0};
      U64     hop_count = 0;
      U64     hop_cap   = 1024;
      for(; hop_count < hop_cap; hop_count += 1) {
        COFF_ParsedSymbol symlink_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(symlink_ref.obj, symlink_ref.symbol_idx);
        LNK_ObjSymbolRef next_symlink_ref = {0};
        if (!lnk_obj_get_comdat_symlink_from_section_number(symlink_ref.obj, symlink_parsed.section_number, &next_symlink_ref)) { break; }
        if (MemoryMatchStruct(&next_symlink_ref, &symlink_ref)) { break; }
        if (hash_map_search_string_u64(&seen_hm, str8_struct(&next_symlink_ref)) != 0) {
          lnk_error_obj(LNK_Error_IllData, obj, "recursive COMDAT symlink in SECT%X", it.v.section_number);
          MemoryZeroStruct(&symlink_ref);
          break;
        }
        symlink_ref = next_symlink_ref;
        hash_map_push_string_u64(temp.arena, &seen_hm, str8_copy(temp.arena, str8_struct(&symlink_ref)), 1);
      }
      if (hop_count >= hop_cap) {
        lnk_error_obj(LNK_Error_IllData, obj, "failed to flatten symlink for SECT%X; max number of hops reached", it.v.section_number);
        MemoryZeroStruct(&symlink_ref);
      }
      temp_end(temp);

      obj->symlinks[it.v.section_number] = symlink_ref;
    }
  }
  ProfEnd();
  barrier_wait(tp->barrier);

  scratch_end(scratch2);
  scratch_end(scratch);
  ProfEnd();
}

internal void
lnk_opt_icf(TP_Context *tp, Arena *perm, LNK_SymbolTable *symtab, LNK_Config *config, LNK_Obj **objs, U64 objs_count)
{
  ProfBegin("/OPT:ICF");
  Temp scratch = scratch_begin(&perm, 1);

  lnk_log(LNK_Log_Debug, "/OPT:ICF:");

  // per-section fold map, consumed by the debug-aliasing pass after /OPT:REF
  // (lnk_icf_mark_folded_lines); allocated only when that pass will run
  if (config->opt_ref == LNK_SwitchState_Yes) {
    ProfScope("Alloc fold maps") {
      for EachIndex(obj_idx, objs_count) {
        objs[obj_idx]->icf_fold = push_array(perm, LNK_ICFFold, objs[obj_idx]->coff.sections.count_no_null + 1);
      }
    }
  }

  // BARRIER pass (path B): the task synchronizes with barrier_wait(tp->barrier)/tp_broadcast,
  // so under /RAD_SHARED_THREAD_POOL it must run at a pinned cohort via the reserve path --
  // a plain tp_for_parallel admits workers incrementally and the barrier never fills (deadlock).
  // Pin the cohort BEFORE sizing the per-lane obj distribution so both agree on the width.
  U32 C = tp_barrier_begin(tp);
  U32Array *obj_indices = lnk_obj_indices_from_section_counts(scratch.arena, C, objs, objs_count);
  LNK_OptTask task = { .symtab = symtab, .config = config, .objs = objs, .objs_count = objs_count, .obj_indices = obj_indices };
  tp_for_parallel_reserve(tp, 0, C, lnk_opt_icf_task, &task); // BARRIER pass (path B)
  tp_barrier_end(tp);

  scratch_end(scratch);
  ProfEnd();
}

internal int
lnk_section_definition_is_before(void *raw_a, void *raw_b)
{
  LNK_SectionDefinition **a = raw_a, **b = raw_b;
  U64 input_idx_a = Compose64Bit((*a)->obj->input_idx, (*a)->obj_section_number);
  U64 input_idx_b = Compose64Bit((*b)->obj->input_idx, (*b)->obj_section_number);
  return u64_compar_is_before(&input_idx_a, &input_idx_b);
}

internal B32
lnk_should_gather_section(LNK_Obj *obj, U64 section_number, COFF_SectionHeader *sect_header)
{
  COFF_SectionFlags sect_flags = obj->coff.sections.headers[section_number].flags;

  // removed sections were eliminated before image layout
  if (sect_flags & COFF_SectionFlag_LnkRemove) {
    return 0;
  }

  // linker-info sections carry metadata but are not copied to the image
  if (sect_flags & COFF_SectionFlag_LnkInfo) {
    return 0;
  }

  // empty COMDATs with symlinks can still anchor symbols at offset zero
  if (sect_header->fsize == 0) {
    if (~sect_flags & COFF_SectionFlag_LnkCOMDAT) {
      return 0;
    }

    LNK_ObjSymbolRef symlink_ref = {0};
    if (!lnk_obj_get_comdat_symlink_from_section_number(obj, section_number, &symlink_ref)) {
      return 0;
    }

    // gather only COMDAT leaders
    AssertAlways(symlink_ref.obj == obj);
  }

  return 1;
}

typedef struct
{
  LNK_Obj **objs; // indexed by input_idx (== task_id)
} LNK_ICFMarkFoldedLinesTask;

// Find the COMDAT-associative .debug$S child that carries a function section's
// per-function CodeView records.
internal U32
lnk_icf_debug_s_child_from_section(LNK_Obj *obj, U32 fn_sn)
{
  if (fn_sn == 0 || fn_sn > obj->coff.sections.count_no_null) { return 0; }
  U32Array associated_sections = lnk_obj_associated_sections_from_section_number(obj, fn_sn);
  for EachIndex(assoc_idx, associated_sections.count) {
    U32 sn = associated_sections.v[assoc_idx];
    if (sn == 0 || sn > obj->coff.sections.count_no_null) { continue; }
    if (~obj->coff.sections.headers[sn].flags & LNK_SECTION_FLAG_DEBUG) { continue; }
    if (str8_match(lnk_obj_section_name_from_section_number(obj, sn), str8_lit(".debug$S"), 0)) { return sn; }
  }
  return 0;
}


// FILECHKSMS of the obj-wide (non-COMDAT) .debug$S -- the table every per-function
// Lines fragment's file_off indexes into. Direct header walk with early-out instead of
// cv_debug_s_from_data: the obj-wide .debug$S is megabytes of subsections and the full
// parse pushes a list node per subsection; here we only need one slice.
internal String8
lnk_icf_obj_file_chksms_scan(LNK_Obj *obj)
{
  for LNK_EachCoffSection(it, obj) {
    COFF_SectionFlags flags = *it.v.flags;
    if (~flags & LNK_SECTION_FLAG_DEBUG)    { continue; }
    if ( flags & COFF_SectionFlag_LnkCOMDAT) { continue; }
    if (!str8_match(lnk_obj_section_name_from_section_number(obj, it.v.section_number), str8_lit(".debug$S"), 0)) { continue; }
    LNK_CObjDebugSView indexed = {0};
    if (lnk_compressed_obj_debug_s_index(obj->compressed_obj, it.v.frange, &indexed)) {
      for EachIndex(i, indexed.count) {
        LNK_CObjDebugSEntry *entry = &indexed.v[i];
        if (entry->kind == CV_C13SubSectionKind_FileChksms) {
          return str8(obj->coff.data.str + entry->raw_payload_offset, entry->raw_payload_size);
        }
      }
      continue;
    }
    String8 raw = lnk_obj_section_data_from_number(obj, it.v.section_number);
    if (raw.size < sizeof(CV_Signature) || cv_signature_from_debug_s(raw) != CV_Signature_C13) { continue; }
    for (U64 cursor = sizeof(CV_Signature); cursor + sizeof(CV_C13SubSectionHeader) <= raw.size; ) {
      CV_C13SubSectionHeader header = {0};
      cursor += str8_deserial_read_struct(raw, cursor, &header);
      if (header.kind == CV_C13SubSectionKind_FileChksms) {
        return str8_substr(raw, r1u64(cursor, cursor + header.size));
      }
      cursor += header.size;
      cursor = AlignPow2(cursor, CV_C13SubSectionAlign);
    }
  }
  return str8_zero();
}

// Memoized per obj: leaders are shared across many follower objs, so without the memo the
// scan reruns once per (follower obj x leader switch). The result slices the immutable
// obj->coff.data mapping, so the racy fill is idempotent (every worker writes identical bytes);
// the init flag is published last.
internal String8
lnk_icf_obj_file_chksms(LNK_Obj *obj)
{
  if (!ins_atomic_u32_eval((U32 *)&obj->icf_file_chksms_init)) {
    String8 chksms = lnk_icf_obj_file_chksms_scan(obj);
    obj->icf_file_chksms = chksms;
    ins_atomic_u32_eval_assign((U32 *)&obj->icf_file_chksms_init, 1);
  }
  return obj->icf_file_chksms;
}

// source identity of a function: (checksum of its file, first line). Two ICF fold members
// with equal keys are the same source text (template twins) -- their locals/labels are
// identical and the leader's record tree serves both.
typedef struct
{
  B32     valid;
  U32     line;
  U8      chksum_kind;
  String8 chksum;
} LNK_ICFSrcKey;

internal LNK_ICFSrcKey
lnk_icf_src_key_from_fn(Arena *scratch, LNK_Obj *obj, U32 fn_sn, String8 chksms)
{
  LNK_ICFSrcKey key = {0};
  U32 child_sn = lnk_icf_debug_s_child_from_section(obj, fn_sn);
  if (child_sn == 0 || chksms.size == 0) { return key; }
  LNK_ObjSection sect = lnk_obj_section_from_section_number(obj, child_sn);
  String8 frag = {0};
  LNK_CObjDebugSView indexed = {0};
  if (lnk_compressed_obj_debug_s_index(obj->compressed_obj, sect.frange, &indexed)) {
    for EachIndex(i, indexed.count) {
      LNK_CObjDebugSEntry *entry = &indexed.v[i];
      if (entry->kind == CV_C13SubSectionKind_Lines) {
        frag = str8(obj->coff.data.str + entry->raw_payload_offset, entry->raw_payload_size);
        break;
      }
    }
  } else {
    String8   raw   = lnk_obj_section_data_from_number(obj, child_sn);
    CV_DebugS ds    = cv_debug_s_from_data(scratch, raw);
    cv_debug_s_tag_prov_sect(&ds, child_sn-1);
    String8List lines = cv_sub_section_from_debug_s(ds, CV_C13SubSectionKind_Lines);
    if (lines.node_count) { frag = lines.first->string; }
  }
  if (frag.size == 0) { return key; }
  if (frag.size < sizeof(CV_C13SubSecLinesHeader) + sizeof(CV_C13File) + sizeof(CV_C13Line)) { return key; }
  CV_C13File *file = (CV_C13File *)(frag.str + sizeof(CV_C13SubSecLinesHeader));
  CV_C13Line *l0   = (CV_C13Line *)((U8 *)file + sizeof(CV_C13File));
  if ((U64)file->file_off + sizeof(CV_C13Checksum) > chksms.size) { return key; }
  CV_C13Checksum *ck = (CV_C13Checksum *)(chksms.str + file->file_off);
  if ((U64)file->file_off + sizeof(CV_C13Checksum) + ck->len > chksms.size) { return key; }
  key.valid       = 1;
  key.line        = (U32)(l0->flags & 0xFFFFFF);
  key.chksum_kind = ck->kind;
  key.chksum      = str8(chksms.str + file->file_off + sizeof(CV_C13Checksum), ck->len);
  return key;
}

internal B32
lnk_icf_debug_s_summary_has_locals(LNK_CObjDebugSView *indexed)
{
  for EachIndex(i, indexed->count) {
    if (indexed->v[i].kind == CV_C13SubSectionKind_Symbols &&
        (indexed->summaries[i].flags & LNK_COBJ_DEBUG_S_SUMMARY_HAS_LOCALS)) { return 1; }
  }
  return 0;
}

// does the function's record tree have anything a watch window would show?
internal B32
lnk_icf_debug_s_has_locals(Arena *scratch, LNK_Obj *obj, U32 child_sn)
{
  LNK_ObjSection sect = lnk_obj_section_from_section_number(obj, child_sn);
  LNK_CObjDebugSView indexed = {0};
  String8List syms = {0};
  if (lnk_compressed_obj_debug_s_index(obj->compressed_obj, sect.frange, &indexed)) {
    if (indexed.summaries) {
      return lnk_icf_debug_s_summary_has_locals(&indexed);
    }
    for EachIndex(i, indexed.count) {
      if (indexed.v[i].kind == CV_C13SubSectionKind_Symbols) {
        str8_list_push(scratch, &syms, str8(obj->coff.data.str + indexed.v[i].raw_payload_offset,
                                            indexed.v[i].raw_payload_size));
      }
    }
  } else {
    String8   raw = lnk_obj_section_data_from_number(obj, child_sn);
    CV_DebugS ds  = cv_debug_s_from_data(scratch, raw);
    cv_debug_s_tag_prov_sect(&ds, child_sn-1);
    syms = cv_sub_section_from_debug_s(ds, CV_C13SubSectionKind_Symbols);
  }
  for EachNode(n, String8Node, syms.first) {
    String8 s = n->string;
    for (U64 o = 0; o + 4 <= s.size; ) {
      U16 len, kind;
      MemoryCopy(&len,  s.str + o,     sizeof(len));
      MemoryCopy(&kind, s.str + o + 2, sizeof(kind));
      if (len < 2) { break; }
      switch (kind) {
      // stack locals
      case CV_SymKind_LOCAL:
      case CV_SymKind_REGREL32:
      // function-scoped statics (S_LDATA32 and friends): the record naming the static lives in
      // this tree; if it were dropped the debugger could no longer evaluate the follower's
      // static by name, even though the (folded) data itself survives in the image
      case CV_SymKind_LDATA32:
      case CV_SymKind_GDATA32:
      case CV_SymKind_LTHREAD32:
      case CV_SymKind_GTHREAD32:
      case CV_SymKind_FILESTATIC:
      case CV_SymKind_CONSTANT:
        return 1;
      }
      o += len + 2;
    }
  }
  return 0;
}

// /OPT:ICF folded-function debug-info slimming. Without this pass a folded function's associated
// .debug$S stays collected in full (REF marked it live with its then-live parent; the ICF fold
// removes only the .text follower), so the module stream receives every folded body's WHOLE record
// tree (S_GPROC/locals + Lines) bound to the leader RVA -- link.exe-parity content at ~+30% module
// bytes. The C13 Lines subsections are ~1% of that and are all a source breakpoint needs to bind.
// So mark each folded follower's associated .debug$S LnkRemove (drops it from full collection);
// the reloc patcher still patches it and the C13 pass merges back ONLY its Lines -- their
// SECREL/SECTION relocs target the folded function symbol, which resolves to the leader RVA
// through the redirected symlink/sect_map, so the lines land on the surviving body.
internal
THREAD_POOL_TASK_FUNC(lnk_icf_mark_folded_lines_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_ICFMarkFoldedLinesTask *task = raw_task;
  LNK_Obj                    *obj  = task->objs[task_id];
  if (obj->icf_fold == 0) { scratch_end(scratch); return; }

  for (U32 section_number = 1; section_number <= obj->coff.sections.count_no_null; section_number += 1) {
    LNK_ICFFold fold = obj->icf_fold[section_number];
    if (!fold.set)                                                    { continue; }
    if (~obj->coff.sections.headers[section_number].flags & COFF_SectionFlag_LnkRemove) { continue; } // follower kept live -> its own records emit
    LNK_Obj *leader_obj = task->objs[fold.leader_obj_idx];
    if (leader_obj->coff.sections.headers[fold.leader_sn].flags & COFF_SectionFlag_LnkRemove) { continue; } // whole class dead-stripped

    // Lines-only by default. Escalate to the FULL record tree (link.exe parity for this one
    // fold) when the follower comes from a DIFFERENT source location than the leader (else the
    // trees are textually identical -- template twins) AND it has locals to show. Measured on
    // the FN editor DLL: ~6.5% of folds differ in source, most of those are empty virtuals, so
    // the escalation set is small.
    U8 mark = 1;
    {
      Temp fold_temp = temp_begin(scratch.arena);
      U32 child_sn = lnk_icf_debug_s_child_from_section(obj, section_number);
      B32 needs_src_key = child_sn != 0;
      if (needs_src_key && obj->compressed_obj != 0) {
        // Only a follower with locals can escalate to a full record tree. A
        // negative summary proves that source-key reads cannot change the mark;
        // avoid faulting compressed Lines/checksum payloads just to compare them.
        // Without summaries, retain the old source-key-first order so raw symbol
        // trees are not parsed unnecessarily for same-source folds.
        LNK_ObjSection child = lnk_obj_section_from_section_number(obj, child_sn);
        LNK_CObjDebugSView indexed = {0};
        if (lnk_compressed_obj_debug_s_index(obj->compressed_obj, child.frange, &indexed) && indexed.summaries &&
            !lnk_icf_debug_s_summary_has_locals(&indexed)) {
          needs_src_key = 0;
        }
      }
      if (needs_src_key) {
        String8 follower_chksms = lnk_icf_obj_file_chksms(obj);        // per-obj memo -- leaders
        String8 leader_chksms   = lnk_icf_obj_file_chksms(leader_obj); // shared across follower objs
        LNK_ICFSrcKey fk = lnk_icf_src_key_from_fn(fold_temp.arena, obj, section_number, follower_chksms);
        LNK_ICFSrcKey lk = lnk_icf_src_key_from_fn(fold_temp.arena, leader_obj, fold.leader_sn, leader_chksms);
        B32 same_src = fk.valid && lk.valid &&
                       fk.line == lk.line && fk.chksum_kind == lk.chksum_kind &&
                       str8_match(fk.chksum, lk.chksum, 0);
        if (fk.valid && lk.valid && !same_src && lnk_icf_debug_s_has_locals(fold_temp.arena, obj, child_sn)) {
          mark = 2;
        }
      }
      temp_end(fold_temp);
    }

    U32Array associated_sections = lnk_obj_associated_sections_from_section_number(obj, section_number);
    for EachIndex(assoc_idx, associated_sections.count) {
      U32 assoc_sn = associated_sections.v[assoc_idx];
      if (assoc_sn == 0 || assoc_sn > obj->coff.sections.count_no_null) { continue; }
      if (~obj->coff.sections.headers[assoc_sn].flags & LNK_SECTION_FLAG_DEBUG) { continue; }
      // exclude the follower's .debug$S from full module collection (it would otherwise merge
      // its whole record tree at the leader RVA, link.exe-parity size); the consumers below
      // merge back just its Lines (mark 1) or, rarely, the whole tree (mark 2)
      obj->coff.sections.headers[assoc_sn].flags |= COFF_SectionFlag_LnkRemove;
      if (obj->icf_lines_only == 0) {
        obj->icf_lines_only = push_array(arena, B8, obj->coff.sections.count_no_null + 1);
      }
      obj->icf_lines_only[assoc_sn] = mark;
    }
  }

  scratch_end(scratch);
}

internal void
lnk_icf_mark_folded_lines(TP_Context *tp, TP_Arena *arena, LNK_Obj **objs, U64 objs_count)
{
  ProfBeginFunction();
  LNK_ICFMarkFoldedLinesTask task = { .objs = objs };
  tp_for_parallel(tp, arena, objs_count, lnk_icf_mark_folded_lines_task, &task); // arena: per-obj icf_lines_only bitmaps
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_gather_sections_task)
{
  Temp scratch = scratch_begin(&arena, 1);

  LNK_BuildImageTask *task         = raw_task;
  Rng1U64             range        = task->u.gather_sects.ranges[task_id];
  HashTable          *sect_defn_ht = hash_table_init(arena, 128);
  task->u.gather_sects.defns[task_id] = sect_defn_ht;

  ProfBegin("Gather Section Definitions");
  for EachInRange(obj_idx, range) {
    LNK_Obj *obj          = task->objs[obj_idx];
    String8  string_table = str8_substr(obj->coff.data, obj->coff.header.string_table_range);

    for LNK_EachCoffSection(it, obj) {
      COFF_SectionHeader *sect_header = it.v.header;

      if ( ! lnk_should_gather_section(obj, it.v.section_number, sect_header)) { continue; }

      Temp temp = temp_begin(scratch.arena);

      // was section defined?
      COFF_SectionFlags      image_sect_flags     = *it.v.flags & ~(COFF_SectionFlags_LnkFlags | COFF_SectionFlags_Reserved);
      String8                sect_name            = coff_name_from_section_header(string_table, sect_header);
      image_sect_flags = lnk_apply_section_directives_to_flags(task->config, sect_name, image_sect_flags);
      String8                sect_name_with_flags = lnk_make_name_with_flags(temp.arena, sect_name, image_sect_flags);
      LNK_SectionDefinition *sect_defn            = hash_table_search_string_raw(sect_defn_ht, sect_name_with_flags);

      // push new section definition
      if (sect_defn == 0) {
        sect_defn = push_array(arena, LNK_SectionDefinition, 1);
        sect_defn->name               = sect_name;
        sect_defn->obj                = obj;
        sect_defn->obj_section_number = it.v.section_number;
        sect_defn->flags              = image_sect_flags;

        sect_name_with_flags = push_str8_copy(arena, sect_name_with_flags);
        hash_table_push_string_raw(arena, sect_defn_ht, sect_name_with_flags, sect_defn);
      }

      // acc contrib count
      sect_defn->contribs_count += 1;
      
      temp_end(temp);
    }
  }
  ProfEnd();

  barrier_wait(tp->barrier);

  if (task_id == 0) {
    Arena            *main_arena = task->u.gather_sects.arena;
    LNK_Config       *config     = task->config;
    LNK_SectionTable *sectab     = task->sectab;

    ProfBegin("Merge Section Definitions Hash Tables");
    for (U64 worker_idx = 1; worker_idx < tp->worker_count; worker_idx += 1) {
      U64                     sect_defns_count = task->u.gather_sects.defns[worker_idx]->count;
      LNK_SectionDefinition **sect_defns       = values_from_hash_table_raw(main_arena, task->u.gather_sects.defns[worker_idx]);
      radsort(sect_defns, sect_defns_count, lnk_section_definition_is_before);

      for EachIndex(defn_idx, sect_defns_count) {
        LNK_SectionDefinition *defn            = sect_defns[defn_idx];
        String8                name_with_flags = lnk_make_name_with_flags(main_arena, defn->name, defn->flags);
        LNK_SectionDefinition *main_defn       = hash_table_search_string_raw(task->u.gather_sects.defns[0], name_with_flags);
        if (main_defn == 0) {
          main_defn = sect_defns[defn_idx];
          hash_table_push_string_raw(main_arena, task->u.gather_sects.defns[0], name_with_flags, main_defn);
        } else {
          if (lnk_section_definition_is_before(&sect_defns[defn_idx], &main_defn)) {
            main_defn->obj                = sect_defns[defn_idx]->obj;
            main_defn->obj_section_number = sect_defns[defn_idx]->obj_section_number;
          }
          main_defn->contribs_count += sect_defns[defn_idx]->contribs_count;
        }
      }
    }
    U64                     sect_defns_count = task->u.gather_sects.defns[0]->count;
    LNK_SectionDefinition **sect_defns       = values_from_hash_table_raw(main_arena, task->u.gather_sects.defns[0]);
    ProfEnd();

    ProfBegin("Sort Sections Definitions");
    radsort(sect_defns, sect_defns_count, lnk_section_definition_is_before);
    ProfEnd();

    ProfBegin("Push Sections And Reserve Section Contrib Memory");
    task->contribs_ht = hash_table_init(sectab->arena, sect_defns_count);
    for EachIndex(defn_idx, sect_defns_count) {
      LNK_SectionDefinition *sect_defn = sect_defns[defn_idx];

      // parse section name
      String8 sect_name, sort_idx;
      coff_parse_section_name(sect_defn->name, &sect_name, &sort_idx);

      // do not create definitions for sections that are removed from the image
      if (lnk_is_section_removed(config, sect_name)) { continue; }

      // warn about conflicting section flags
      for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
        if (str8_match(sect_n->data.name, sect_name, 0) && sect_n->data.flags != sect_defn->flags) {
          LNK_Obj            *obj                = sect_defn->obj;
          U32                 sect_number        = sect_defn->obj_section_number;
          COFF_SectionHeader *sect_header        = lnk_coff_section_header_from_section_number(obj, sect_number);
          String8             sect_name          = coff_name_from_section_header(str8_substr(obj->coff.data, obj->coff.header.string_table_range), sect_header);
          String8             expected_flags_str = coff_string_from_section_flags(main_arena, sect_n->data.flags);
          String8             current_flags_str  = coff_string_from_section_flags(main_arena, sect_defn->flags);
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
        LNK_SectionContribChunk *contrib_chunk        = hash_table_search_string_raw(task->contribs_ht, defn_name_with_flags);
        if (!contrib_chunk) {
          contrib_chunk = lnk_section_contrib_chunk_list_push_chunk(main_arena, &sect->contribs, sect_defn->contribs_count, sect_defn->name);
          hash_table_push_string_raw(sectab->arena, task->contribs_ht, defn_name_with_flags, contrib_chunk);
        }

        ProfEnd();
      }
    }
    ProfEnd();

    ProfBegin("Alloc Section Map");
    task->sect_map = push_array(main_arena, LNK_SectionContrib **, task->objs_count);
    for EachIndex(obj_idx, task->objs_count) { task->sect_map[obj_idx] = push_array(main_arena, LNK_SectionContrib *, task->objs[obj_idx]->coff.sections.count_no_null + 1); }
    ProfEnd();
  }

  barrier_wait(tp->barrier);

  ProfBegin("Gather Section Contribs");
  for EachInRange(obj_idx, range) {
    LNK_Obj *obj          = task->objs[obj_idx];
    String8  string_table = str8_substr(obj->coff.data, obj->coff.header.string_table_range);

    ProfBeginV("Gather Section Contribs [%S]", obj->path);
    for LNK_EachCoffSection(it, obj) {
      LNK_SectionContrib *sc          = task->null_sc;
      COFF_SectionHeader *sect_header = it.v.header;
      COFF_SectionFlags   sect_flags  = *it.v.flags;
      task->sect_map[obj_idx][it.v.section_number] = sc;

      if ( ! lnk_should_gather_section(obj, it.v.section_number, sect_header)) { continue; }

      LNK_SectionContribChunk *sc_chunk = 0;
      {
        Temp temp = temp_begin(scratch.arena);
        COFF_SectionFlags sect_flags_clean = sect_flags & ~(COFF_SectionFlags_LnkFlags | COFF_SectionFlags_Reserved);
        String8           sect_name        = coff_name_from_section_header(string_table, sect_header);
        sect_flags_clean = lnk_apply_section_directives_to_flags(task->config, sect_name, sect_flags_clean);
        String8           sect_key         = lnk_make_name_with_flags(temp.arena, sect_name, sect_flags_clean);
        sc_chunk = hash_table_search_string_raw(task->contribs_ht, sect_key);
        temp_end(temp);
      }

      if (sc_chunk) {
        String8 data;
        if (sect_flags & COFF_SectionFlag_CntUninitializedData) {
          data = str8(0, sect_header->fsize);
        } else {
          data = lnk_obj_section_data_from_number(obj, it.v.section_number);
        }

        U16 sc_align = coff_align_size_from_section_flags(sect_flags);
        sc = lnk_section_contrib_chunk_push_atomic(sc_chunk, 1);
        sc->first_data_node.next   = 0;
        sc->first_data_node.string = data;
        sc->last_data_node         = &sc->first_data_node;
        sc->align                  = sc_align == 0 ? task->default_align : sc_align;
        sc->u.obj_idx            = obj_idx;
        sc->u.obj_section_number = safe_cast_u32(it.v.section_number);
      }
      task->sect_map[obj_idx][it.v.section_number] = sc;
    }
    ProfEnd();
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
  for LNK_EachCoffSection(it, obj) {
    U64 section_number = it.v.section_number;

    if (~*it.v.flags & COFF_SectionFlag_LnkCOMDAT) { continue; }

    LNK_ObjSymbolRef symlink_ref = {0};
    if ( ! lnk_obj_get_comdat_symlink_from_section_number(obj, section_number, &symlink_ref)) { continue; }

    COFF_ParsedSymbol symlink_parsed = lnk_parsed_symbol_from_coff_symbol_idx_no_name(symlink_ref.obj, symlink_ref.symbol_idx);
    task->sect_map[obj_idx][section_number] = task->sect_map[symlink_ref.obj->input_idx][symlink_parsed.section_number];
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_flag_debug_symbols_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  for LNK_EachCoffSymbol(it, obj) {
    U64               symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol symbol     = it.v;
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Regular) {
      if (obj->coff.sections.headers[symbol.section_number].flags & LNK_SECTION_FLAG_DEBUG) {
        task->u.patch_symtabs.was_symbol_patched[obj_idx][symbol_idx] = 1;
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_patch_comdat_leaders_task)
{
  LNK_BuildImageTask *task    = raw_task;
  U64                 obj_idx = task_id;
  LNK_Obj            *obj     = task->objs[obj_idx];

  ProfBeginV("Patch COMDAT Offsets in %S", obj->path);
  for LNK_EachCoffSymbol(it, obj) {
    U64               symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol symbol     = it.v;

    COFF_SymbolValueInterpType interp = coff_interp_from_parsed_symbol(symbol);
    if (interp != COFF_SymbolValueInterp_Regular) { continue; }

    LNK_ObjSymbolRef symlink_ref = {0};
    if ( ! lnk_obj_get_comdat_symlink_from_section_number(obj, symbol.section_number, &symlink_ref)) { continue; }

    COFF_ParsedSymbol leader_symbol = lnk_parsed_symbol_from_coff_symbol_idx_no_name(symlink_ref.obj, symlink_ref.symbol_idx);
    if (symlink_ref.obj == obj && leader_symbol.section_number == symbol.section_number) { continue; }

    B32 is_external = symbol.storage_class == COFF_SymStorageClass_External;
    B32 is_same_obj = symlink_ref.obj == obj;

    U32 section_number = symbol.section_number;
    U32 value          = symbol.value;
    B32 should_patch   = 0;

    if (is_same_obj) {
      B32 is_static_comdat_leader = symbol.storage_class == COFF_SymStorageClass_Static && obj->coff.sections.comdats[symbol.section_number] == symbol_idx;
      if (is_external || is_static_comdat_leader) {
        section_number = leader_symbol.section_number;
        value          = leader_symbol.value;

        // ICF folds sections by linking to the leader section definition; preserve
        // public symbol offsets inside identical folded sections
        if (is_external && leader_symbol.storage_class == COFF_SymStorageClass_Static && leader_symbol.aux_symbol_count > 0) {
          value = symbol.value;
        }

        should_patch = 1;
      }
    } else {
      String8 symbol_name = lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx);
      String8 leader_name = lnk_symbol_name_from_coff_symbol_idx(symlink_ref.obj, symlink_ref.symbol_idx);
      if (is_external && str8_match(symbol_name, leader_name, 0)) {
        value = leader_symbol.value;
        should_patch = 1;
      }
    }

    if (should_patch) {
      obj->coff.symbols.section_numbers[it.primary_idx] = section_number;
      obj->coff.symbols.values         [it.primary_idx] = value;
    }
  }
  ProfEnd();
}

internal int
lnk_section_contrib_ptr_is_before(void *raw_a, void *raw_b)
{
  LNK_SectionContrib **a = raw_a, **b = raw_b;
  U64 input_idx_a = Compose64Bit((*a)->u.obj_idx, (*a)->u.obj_section_number);
  U64 input_idx_b = Compose64Bit((*b)->u.obj_idx, (*b)->u.obj_section_number);
  return u64_compar_is_before(&input_idx_a, &input_idx_b);
}

#define LNK_SORT_CONTRIBS_RADIX_BITS 8
#define LNK_SORT_CONTRIBS_RADIX_SIZE (1 << LNK_SORT_CONTRIBS_RADIX_BITS)
#define LNK_SORT_CONTRIBS_RADIX_MIN  (64u*1024u)

typedef struct LNK_SortContribsRadixTask
{
  Rng1U64 *ranges;
  U64     *keys_src;
  U32     *indices_src;
  U64     *keys_dst;
  U32     *indices_dst;
  U32     *hist;
  U64      shift;
} LNK_SortContribsRadixTask;

internal
THREAD_POOL_TASK_FUNC(lnk_sort_contribs_radix_hist_task)
{
  LNK_SortContribsRadixTask *task = raw_task;
  U32 *hist = task->hist + (U64)task_id * LNK_SORT_CONTRIBS_RADIX_SIZE;
  for EachInRange(i, task->ranges[task_id]) {
    hist[(task->keys_src[i] >> task->shift) & (LNK_SORT_CONTRIBS_RADIX_SIZE - 1)] += 1;
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_sort_contribs_radix_scatter_task)
{
  LNK_SortContribsRadixTask *task = raw_task;
  U32 *hist = task->hist + (U64)task_id * LNK_SORT_CONTRIBS_RADIX_SIZE;
  for EachInRange(i, task->ranges[task_id]) {
    U64 digit   = (task->keys_src[i] >> task->shift) & (LNK_SORT_CONTRIBS_RADIX_SIZE - 1);
    U32 dst_idx = hist[digit]++;
    task->keys_dst[dst_idx]    = task->keys_src[i];
    task->indices_dst[dst_idx] = task->indices_src[i];
  }
}

internal void
lnk_sort_contribs_chunk_radix(TP_Context *tp, Arena *arena, LNK_SectionContribChunk *chunk)
{
  ProfBeginFunction();

  Temp scratch = scratch_begin(&arena, 1);

  U64  count        = chunk->count;
  U64  worker_count = tp->worker_count;

  U64 *keys    = push_array_no_zero(scratch.arena, U64, count);
  U32 *indices = push_array_no_zero(scratch.arena, U32, count);
  U64  max_key = 0;
  for EachIndex(i, count) {
    U64 key = Compose64Bit(chunk->v[i]->u.obj_idx, chunk->v[i]->u.obj_section_number);
    keys[i] = key;
    indices[i] = (U32)i;
    max_key = Max(max_key, key);
  }

  U64      significant_pass_count = (64 - clz64(max_key) + LNK_SORT_CONTRIBS_RADIX_BITS - 1) / LNK_SORT_CONTRIBS_RADIX_BITS;
  U64      pass_count             = significant_pass_count + (significant_pass_count & 1);
  U64     *keys_buffer            = push_array_no_zero(scratch.arena, U64, count);
  U32     *indices_buffer         = push_array_no_zero(scratch.arena, U32, count);
  U32     *hist                   = push_array_no_zero(scratch.arena, U32, worker_count * LNK_SORT_CONTRIBS_RADIX_SIZE);
  Rng1U64 *ranges                 = tp_divide_work(scratch.arena, count, worker_count);

  U64 *keys_src = keys, *keys_dst = keys_buffer;
  U32 *indices_src = indices, *indices_dst = indices_buffer;
  LNK_SortContribsRadixTask task = { .ranges = ranges, .hist = hist };
  for EachIndex(pass, pass_count) {
    task.keys_src    = keys_src;
    task.indices_src = indices_src;
    task.keys_dst    = keys_dst;
    task.indices_dst = indices_dst;
    task.shift       = pass * LNK_SORT_CONTRIBS_RADIX_BITS;

    MemoryZero(hist, sizeof(U32) * worker_count * LNK_SORT_CONTRIBS_RADIX_SIZE);
    tp_for_parallel(tp, 0, worker_count, lnk_sort_contribs_radix_hist_task, &task);

    U64 offset = 0;
    for EachIndex(digit, LNK_SORT_CONTRIBS_RADIX_SIZE) {
      for EachIndex(worker_idx, worker_count) {
        U32 *slot  = &hist[worker_idx * LNK_SORT_CONTRIBS_RADIX_SIZE + digit];
        U32  count = *slot;
        *slot = (U32)offset;
        offset += count;
      }
    }
    tp_for_parallel(tp, 0, worker_count, lnk_sort_contribs_radix_scatter_task, &task);

    Swap(U64 *, keys_src, keys_dst);
    Swap(U32 *, indices_src, indices_dst);
  }

  LNK_SectionContrib **sorted = push_array_no_zero(scratch.arena, LNK_SectionContrib *, count);
  for EachIndex(i, count) {
    sorted[i] = chunk->v[indices[i]];
  }
  MemoryCopy(chunk->v, sorted, count * sizeof(*sorted));

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(lnk_sort_contribs_task)
{
  LNK_BuildImageTask *task = raw_task;
  LNK_SectionContribChunk *chunk = task->u.sort_contribs.chunks[task_id];
  if (chunk->count >= LNK_SORT_CONTRIBS_RADIX_MIN) { return; }
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
    U64                     section_number = task->u.patch_symtabs.common_block_sect->sect_idx + 1;

    U64 primary_idx = lnk_obj_primary_symbol_idx_from_coff_symbol_idx(symbol_ref.obj, symbol_ref.symbol_idx);
    symbol_ref.obj->coff.symbols.values         [primary_idx] = contrib->u.offset;
    symbol_ref.obj->coff.symbols.section_numbers[primary_idx] = safe_cast_u32(section_number);

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
  for LNK_EachCoffSymbol(it, obj) {
    U64               symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol symbol     = it.v;
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Common) {
      LNK_Symbol       *defn        = lnk_symbol_table_search(task->symtab, lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx));
      COFF_ParsedSymbol defn_parsed = lnk_parsed_from_symbol(defn);
      Assert(lnk_interp_from_symbol(defn) == COFF_SymbolValueInterp_Regular);
      if (defn) {
        obj->coff.symbols.section_numbers[it.primary_idx] = defn_parsed.section_number;
        obj->coff.symbols.values         [it.primary_idx] = safe_cast_u32(defn_parsed.value);
        obj->coff.symbols.storage_classes[it.primary_idx] = COFF_SymStorageClass_Static;
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
  for LNK_EachCoffSymbol(it, obj) {
    U64               symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol symbol     = it.v;

    if (task->u.patch_symtabs.was_symbol_patched[obj_idx][symbol_idx]) { continue; }

    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
    if (interp == COFF_SymbolValueInterp_Regular) {
      LNK_SectionContrib *sc = task->sect_map[obj_idx][symbol.section_number];
      U32                 section_number;
      U32                 value;
      if (sc == task->null_sc) {
        section_number = lnk_obj_get_removed_section_number(obj);
        value          = max_U32;
      } else {
        section_number = safe_cast_u32(sc->u.sect_idx + 1);
        value          = sc->u.off + symbol.value;
      }

      obj->coff.symbols.section_numbers[it.primary_idx] = section_number;
      obj->coff.symbols.values         [it.primary_idx] = value;
    }
  }
  ProfEnd();
}

internal void
lnk_patch_obj_symtab(LNK_SymbolTable *symtab, LNK_Obj *obj, B8 *was_symbol_patched, COFF_SymbolValueInterpType fixup_type)
{
  ProfBeginV("%S\n", obj->path);

  for LNK_EachCoffSymbol(it, obj) {
    U64               symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol fixup_dst  = it.v;
    if (was_symbol_patched[symbol_idx]) { continue; }

    COFF_SymbolValueInterpType fixup_dst_type = coff_interp_symbol(fixup_dst.section_number, fixup_dst.value, fixup_dst.storage_class);
    if (fixup_type != fixup_dst_type) { continue; }

    LNK_ObjSymbolRef symbol_to_resolve = { .obj = obj, .symbol_idx = symbol_idx };
    LNK_ObjSymbolRef fixup_symbol      = {0};
    B32               is_resolved       = lnk_resolve_symbol(symtab, symbol_to_resolve, &fixup_symbol);
    if (is_resolved) {
      COFF_ParsedSymbol          fixup_src          = lnk_parsed_symbol_from_coff_symbol_idx_no_name(fixup_symbol.obj, fixup_symbol.symbol_idx);
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

      obj->coff.symbols.section_numbers[it.primary_idx] = section_number;
      obj->coff.symbols.values         [it.primary_idx] = value;
      obj->coff.symbols.storage_classes[it.primary_idx] = COFF_SymStorageClass_Static;

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

// Non-temporal (streaming) stores for the write-once image buffer. The image is
// filled, then streamed straight to disk; these bytes are not re-read by the
// filling thread, so NT stores avoid polluting L2/L3 with ~1GB of write-once
// data. A later pass (lnk_obj_reloc_patcher) DOES read the image back, so every
// caller must _mm_sfence() before that pass runs to make the NT stores globally
// visible. NT stores require 32B alignment; the unaligned head/tail and small
// (<256B) copies fall back to MemoryCopy/MemorySet (identical bytes either way).
#define LNK_STREAM_MIN_SIZE 256

// SSE2 (baseline on x86-64, no -mavx required) 16B non-temporal stores.
internal void
lnk_stream_copy(void *dst, void *src, U64 size)
{
  if (size < LNK_STREAM_MIN_SIZE) { MemoryCopy(dst, src, size); return; }
  U8 *d = (U8 *)dst, *s = (U8 *)src;
  U64 head = (U64)(0x10 - ((U64)d & 0xf)) & 0xf; // bytes to reach 16B-aligned dst
  if (head) { MemoryCopy(d, s, head); d += head; s += head; size -= head; }
  U64 vec = size & ~(U64)0xf;
  for (U64 i = 0; i < vec; i += 0x10) {
    __m128i v = _mm_loadu_si128((__m128i const *)(s + i));
    _mm_stream_si128((__m128i *)(d + i), v);
  }
  U64 tail = size - vec;
  if (tail) { MemoryCopy(d + vec, s + vec, tail); }
}

internal void
lnk_stream_set(void *dst, U8 byte, U64 size)
{
  if (size < LNK_STREAM_MIN_SIZE) { MemorySet(dst, byte, size); return; }
  U8 *d = (U8 *)dst;
  U64 head = (U64)(0x10 - ((U64)d & 0xf)) & 0xf;
  if (head) { MemorySet(d, byte, head); d += head; size -= head; }
  __m128i v = _mm_set1_epi8((char)byte);
  U64 vec = size & ~(U64)0xf;
  for (U64 i = 0; i < vec; i += 0x10) {
    _mm_stream_si128((__m128i *)(d + i), v);
  }
  U64 tail = size - vec;
  if (tail) { MemorySet(d + vec, byte, tail); }
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
        lnk_stream_copy(image_data.str + image_off, sc->first_data_node.string.str, sc->first_data_node.string.size);
        continue;
      }
      U64 cursor = 0;
      for EachNode(data_n, String8Node, &sc->first_data_node) {
        U64 image_off = sc->u.off + n->base_foff + cursor;
        Assert(image_off + data_n->string.size <= image_data.size);
        lnk_stream_copy(image_data.str + image_off, data_n->string.str, data_n->string.size);
        cursor += data_n->string.size;
      }
    }
  }
  // NT stores above are not ordered wrt later normal reads on other cores; the
  // reloc-patch pass reads the image back. Make these stores globally visible.
  _mm_sfence();
  ProfEnd();
}

typedef struct
{
  U8 *dst;
  U64 size;
  U8  byte;
} LNK_FillAlignRange;

internal
THREAD_POOL_TASK_FUNC(lnk_fill_align_bytes_task)
{
  ProfBeginFunction();
  LNK_FillAlignRange *range = &((LNK_FillAlignRange *)raw_task)[task_id];
  lnk_stream_set(range->dst, range->byte, range->size);
  // make this task's NT stores globally visible before the completion counter is
  // bumped, so every thread past the join (contrib-fill / reloc passes) sees them
  _mm_sfence();
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

  String8 string_table = lnk_coff_string_table_from_obj(obj);

  for LNK_EachCoffSection(it, obj) {
    COFF_SectionHeader *section_header = it.v.header;
    COFF_SectionFlags   section_flags  = *it.v.flags;

    if (section_flags & COFF_SectionFlag_LnkInfo)              { continue; }
    if (section_flags & COFF_SectionFlag_LnkRemove) {
      // exception: ICF-folded functions' .debug$S stays dead but its Lines are merged into the
      // module bound to the leader RVA -- patch it so those Lines carry real addresses
      if (!(obj->icf_lines_only && obj->icf_lines_only[it.v.section_number])) { continue; }
    }
    if (section_flags & COFF_SectionFlag_CntUninitializedData) { continue; }

    COFF_RelocArray relocs = lnk_coff_relocs_from_section_header(obj, section_header);

    // get section bytes (special case debug info because it is not copied to the image)
    Rng1U64 section_frange = rng_1u64(section_header->foff, section_header->foff + section_header->fsize);
    String8 section_data;
    if (section_flags & LNK_SECTION_FLAG_DEBUG) {
      // Objs excluded from debug output have no later consumer for these bytes.
      if (obj->exclude_from_debug_info) { continue; }

      // A relocation-free debug section can stay on the clean input view.
      if (relocs.count == 0) { continue; }

      // With the default streaming window, .debug$S is reconstructed on demand during
      // module writing, so no persistent patched copy is needed.
      if (g_debug_s_window && str8_match(coff_name_from_section_header(string_table, section_header), str8_lit(".debug$S"), 0)) { continue; }

      if (obj->section_data_copies == 0) {
        obj->section_data_copies = push_array(arena, String8, obj->coff.sections.count_no_null + 1);
      }
      String8 src  = str8_substr(obj->coff.data, section_frange);
      U8     *copy = push_array_no_zero(g_sect_copy_arenas[worker_id], U8, src.size);
      MemoryCopy(copy, src.str, src.size);
      obj->section_data_copies[it.v.section_number] = str8(copy, src.size);
      section_data = obj->section_data_copies[it.v.section_number];
    } else {
      section_data = str8_substr(task->image_data, section_frange);
    }

    // apply relocs (factored: shared with the P3.3 module-write window fill)
    lnk_obj_apply_relocs_to_buffer(obj, it.v.section_number, section_header, section_data, task->image_base, task->image_section_table);
  }

  ProfEnd();
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
  
  if (obj->coff.hotpatch) {
    for LNK_EachCoffSymbol(it, obj) {
      COFF_ParsedSymbol symbol = it.v;
      COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);
      if (interp == COFF_SymbolValueInterp_Regular && COFF_SymbolType_IsFunc(symbol.type)) {
        LNK_SectionContrib *sc = task->sect_map[obj_idx][symbol.section_number];
        if (sc != task->null_sc) {
          sc->hotpatch = !!(obj->coff.sections.headers[symbol.section_number].flags & COFF_SectionFlag_CntCode);
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
  for LNK_EachCoffSection(it, obj) {
    COFF_SectionHeader *sect_header = it.v.header;
    if (~*it.v.flags & COFF_SectionFlag_LnkRemove) {
      LNK_SectionContrib *sc   = task->sect_map[obj_idx][it.v.section_number];
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
  for LNK_EachCoffSection(it, obj) {
    COFF_SectionHeader *sect_header = it.v.header;
    COFF_SectionFlags   sect_flags  = *it.v.flags;
    B32 patch_section_header = (~sect_flags & COFF_SectionFlag_LnkRemove) &&
                               (~sect_flags & LNK_SECTION_FLAG_DEBUG);
    if (patch_section_header) {
      LNK_SectionContrib *sc   = task->sect_map[obj_idx][it.v.section_number];
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
  for LNK_EachCoffSymbol(it, obj) {
    U64                        symbol_idx = it.symbol_idx;
    COFF_ParsedSymbol          symbol     = it.v;
    COFF_SymbolValueInterpType interp     = coff_interp_symbol(symbol.section_number, symbol.value, symbol.storage_class);

    if (interp == COFF_SymbolValueInterp_Undefined) {
      if (symbol.storage_class == COFF_SymStorageClass_Section) {

        B32 is_referenced = 0;
        for LNK_EachCoffSection(it, obj) {
          COFF_SectionHeader *section_header = it.v.header;
          COFF_SectionFlags   section_flags  = *it.v.flags;

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

        String8      symbol_name = lnk_symbol_name_from_coff_symbol_idx(obj, symbol_idx);
        LNK_Section *sect        = lnk_section_table_search(task->sectab, symbol_name, symbol.value);
        if (sect && (~sect->flags & COFF_SectionFlag_LnkRemove)) {
          if (~sect->flags & COFF_SectionFlag_MemDiscardable) {

            LNK_SectionContrib *first_sc = lnk_get_first_section_contrib(sect);
            obj->coff.symbols.section_numbers[it.primary_idx] = safe_cast_u32(first_sc->u.sect_idx + 1);
            obj->coff.symbols.values         [it.primary_idx] = first_sc->u.off;
            obj->coff.symbols.storage_classes[it.primary_idx] = COFF_SymStorageClass_Static;

          } else {
            lnk_error_obj(LNK_Error_SectRefsDiscardedMemory, obj, "symbol %S (No. 0x%llx) references section with discard flag", symbol_name, symbol_idx);
          }
        } else {
          U64 fallback_voff  = 0;
          U64 fallback_align = Max(task->sect_align, KB(4));
          for EachIndex(sect_idx, task->image_sects.count) {
            LNK_Section *image_sect      = task->image_sects.v[sect_idx];
            U64          image_sect_size = AlignPow2(Max(image_sect->vsize, image_sect->fsize), fallback_align);

            if (image_sect_size == 0) { image_sect_size = fallback_align; }

            fallback_voff = Max(fallback_voff, image_sect->voff + image_sect_size);
          }
          fallback_voff = AlignPow2(fallback_voff, fallback_align);

          LNK_Section *fallback_sect = task->image_sects.v[task->image_sects.count-1];
          obj->coff.symbols.section_numbers[it.primary_idx] = safe_cast_u32(fallback_sect->sect_idx + 1);
          obj->coff.symbols.values         [it.primary_idx] = safe_cast_u32(fallback_voff - fallback_sect->voff);
          obj->coff.symbols.storage_classes[it.primary_idx] = COFF_SymStorageClass_Static;

          lnk_error_obj(LNK_Warning_UndefinedSectionSymbol, obj, "undefined section symbol %S (No. 0x%llx) refers to an image section that doesn't exist; patching to %#llx", symbol_name, symbol_idx, fallback_voff);
        }
      }
    }
  }
  ProfEnd();
}

internal
void
lnk_gather_base_reloc_candidate(Arena *arena, LNK_BaseRelocsTask *task, LNK_Obj *obj,
                                HashTable *page_ht, LNK_BaseRelocPageList *pages,
                                U32 sect_idx, U32 apply_off, U32 isymbol, U64 is_addr)
{
  COFF_ParsedSymbol          symbol        = lnk_parsed_symbol_from_coff_symbol_idx_no_name(obj, isymbol);
  COFF_SymbolValueInterpType symbol_interp = coff_interp_from_parsed_symbol(symbol);
  if (symbol_interp == COFF_SymbolValueInterp_Abs) { return; }

  U64                    reloc_voff = obj->coff.sections.headers[sect_idx + 1].voff + apply_off;
  U64                    page_voff  = AlignDownPow2(reloc_voff, task->page_size);
  LNK_BaseRelocPageNode *page       = hash_table_search_u64_raw(page_ht, page_voff);
  if (page == 0) {
    page         = push_array(arena, LNK_BaseRelocPageNode, 1);
    page->v.voff = page_voff;
    page->v.entries_addr32 = push_array(arena, U64List, 1);
    page->v.entries_addr64 = push_array(arena, U64List, 1);
    SLLQueuePush(pages->first, pages->last, page);
    pages->count += 1;
    hash_table_push_u64_raw(arena, page_ht, page_voff, page);
  }

  switch (is_addr) {
  case 4: {
    if (task->is_large_addr_aware) {
      lnk_error_obj(LNK_Error_LargeAddrAwareRequired, obj, "found out of range ADDR32 relocation for '%S', link with /LARGEADDRESSAWARE:NO", lnk_symbol_name_from_coff_symbol_idx(obj, isymbol));
    } else {
      u64_list_push(arena, page->v.entries_addr32, reloc_voff);
    }
  } break;
  case 8: { u64_list_push(arena, page->v.entries_addr64, reloc_voff); } break;
  default: { InvalidPath; } break;
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_gather_base_reloc_pages_task)
{
  LNK_BaseRelocsTask    *task       = raw_task;
  HashTable             *page_ht    = task->gather.page_ht[worker_id];
  LNK_BaseRelocPageList *pages      = &task->gather.pages[worker_id];
  LNK_Obj               *obj        = task->gather.objs[task_id];

  ProfBeginV("%S", obj->path);
  LNK_CObjBaseRelocView compressed_index = {0};
  if (lnk_compressed_obj_base_reloc_index(obj->compressed_obj, &compressed_index)) {
    for EachIndex(i, compressed_index.count) {
      LNK_CObjBaseRelocEntry *entry = &compressed_index.v[i];
      if (entry->sect_idx >= obj->coff.sections.count_no_null || entry->isymbol >= obj->coff.header.symbol_count ||
          (entry->addr_size != 4 && entry->addr_size != 8)) {
        lnk_error_obj(LNK_Error_IllData, obj, "invalid compressed base relocation sidecar entry");
        continue;
      }
      if (obj->coff.sections.headers[entry->sect_idx + 1].flags & COFF_SectionFlag_LnkRemove) { continue; }
      lnk_gather_base_reloc_candidate(arena, task, obj, page_ht, pages, entry->sect_idx,
                                      entry->apply_off, entry->isymbol, entry->addr_size);
    }
    ProfEnd();
    return;
  }

  for LNK_EachCoffSection(it, obj) {
    U32                 sect_idx    = safe_cast_u32(it.v.section_number - 1);
    COFF_SectionHeader *sect_header = it.v.header;
    if (*it.v.flags & COFF_SectionFlag_LnkRemove) { continue; }

    COFF_RelocArray relocs = lnk_coff_relocs_from_section_header(obj, sect_header);
    for EachIndex(reloc_idx, relocs.count) {
      COFF_Reloc *r = &relocs.v[reloc_idx];

      U64 is_addr = coff_is_addr_reloc(obj->coff.header.machine, r->type);
      if (is_addr == 0) { continue; }
      lnk_gather_base_reloc_candidate(arena, task, obj, page_ht, pages, (U32)sect_idx,
                                      r->apply_off, r->isymbol, is_addr);
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
    if (sect->flags & COFF_SectionFlag_CntInitializedData) {
      sizeof_inited_data += Max(sect->fsize, AlignPow2(sect->vsize, config->file_align));
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

internal LNK_Section *
lnk_image_section_table_push(LNK_Config *config, LNK_SectionTable *sectab, String8 name, COFF_SectionFlags flags)
{
  flags = lnk_apply_section_directives_to_flags(config, name, flags);
  LNK_Section *sect = lnk_section_table_push(sectab, name, flags);
  return sect;
}

internal LNK_Section *
lnk_image_section_table_search(LNK_Config *config, LNK_SectionTable *sectab, String8 name, COFF_SectionFlags flags)
{
  flags = lnk_apply_section_directives_to_flags(config, name, flags);
  LNK_Section *sect = lnk_section_table_search(sectab, name, flags);
  return sect;
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
  lnk_image_section_table_push(config, sectab, str8_lit(".text" ), PE_TEXT_SECTION_FLAGS );
  lnk_image_section_table_push(config, sectab, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS);
  lnk_image_section_table_push(config, sectab, str8_lit(".data" ), PE_DATA_SECTION_FLAGS );
  lnk_image_section_table_push(config, sectab, str8_lit(".bss"  ), PE_BSS_SECTION_FLAGS  );
  lnk_image_section_table_push(config, sectab, str8_lit(".pdata"), PE_PDATA_SECTION_FLAGS);
  LNK_Section *common_block_sect = lnk_image_section_table_search(config, sectab, str8_lit(".bss"), PE_BSS_SECTION_FLAGS);

  LNK_BuildImageTask task = {
    .config           = config,
    .symtab           = symtab,
    .sectab           = sectab,
    .objs_count       = objs_count,
    .objs             = objs,
    .function_pad_min = config->function_pad_min,
    .default_align    = coff_default_align_from_machine(config->machine),
    .sect_align       = config->sect_align,
    .null_sc          = push_array(arena->v[0], LNK_SectionContrib, 1),
  };

  U64 expected_image_header_size;
  {
    ProfScope("Gather Sections")
    {
      TP_Temp temp = tp_temp_begin(arena);
      // BARRIER pass (path B): the task synchronizes with barrier_wait(tp->barrier), so under
      // /RAD_SHARED_THREAD_POOL it must run at a pinned cohort via the reserve path -- a plain
      // tp_for_parallel admits workers incrementally and the barrier never fills (deadlock).
      // Pin the cohort BEFORE sizing the per-lane ranges/defns so everything agrees on the width.
      U32 C = tp_barrier_begin(tp);
      task.u.gather_sects.arena  = arena->v[0];
      task.u.gather_sects.ranges = tp_divide_work(arena->v[0], objs_count, C);
      task.u.gather_sects.defns  = push_array(arena->v[0], HashTable *, C);
      ProfBegin("Gather Sections");
      tp_for_parallel_reserve(tp, arena, C, lnk_gather_sections_task, &task); // BARRIER pass (path B)
      ProfEnd();
      tp_barrier_end(tp);
      tp_temp_end(temp);
    }
    // ensure determinism by sorting section contribs in chunks by input index
    ProfScope("Sort Section Contribs")
    {
      U64 total_chunk_count = 0;
      for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
        total_chunk_count += sect_n->data.contribs.chunk_count;
      }

      U64 cursor = 0;
      task.u.sort_contribs.chunks = push_array(scratch.arena, LNK_SectionContribChunk *, total_chunk_count);
      for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
        for EachNode(chunk_n, LNK_SectionContribChunk, sect_n->data.contribs.first) {
          task.u.sort_contribs.chunks[cursor++] = chunk_n;
        }
      }
      Assert(cursor == total_chunk_count);

      for EachIndex(chunk_idx, total_chunk_count) {
        LNK_SectionContribChunk *chunk = task.u.sort_contribs.chunks[chunk_idx];
        if (chunk->count >= LNK_SORT_CONTRIBS_RADIX_MIN) {
          lnk_sort_contribs_chunk_radix(tp, scratch.arena, chunk);
        }
      }
      tp_for_parallel(tp, 0, total_chunk_count, lnk_sort_contribs_task, &task);
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
        LNK_SectionContribChunk *common_block_chunk = lnk_section_contrib_chunk_list_push_chunk(sectab->arena, &common_block_sect->contribs, 1, str8_lit(".bss"));
        LNK_SectionContrib      *common_block_sc    = lnk_section_contrib_chunk_push(common_block_chunk, 1);
        common_block_sc->u.obj_idx              = max_U32;
        common_block_sc->u.obj_section_number   = 0;
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
      // Preserve each source section's contribution range across the merge.
      ProfBegin("Sort Sections");
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        lnk_sort_section_contribs(&sect_n->data, 0);
      }
      ProfEnd();

      // merge sections
      if (config->flags & LNK_ConfigFlag_Merge) {
        lnk_section_table_merge(sectab, config->merge_list);
      }

      ProfBegin("Sort Merged Sections");
      for (LNK_SectionNode *sect_n = sectab->list.first; sect_n != 0; sect_n = sect_n->next) {
        lnk_sort_section_contribs(&sect_n->data, 1);
      }
      ProfEnd();

      if (config->do_function_pad_min == LNK_SwitchState_Yes) {
        tp_for_parallel_prof(tp, arena, objs_count, lnk_flag_hotpatch_contribs_task, &task, "Flag Hotpatch Section Contribs");
      }

      // assign contribs offsets, sizes, and section indices
      for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
        lnk_finalize_section_layout(&sect_n->data, config->file_align, config->function_pad_min);
      }

      // remove empty sections
      {
        String8List empty_sect_list = {0};
        for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
          if (sect_n->data.vsize == 0 && sect_n->data.contribs.chunk_count == 0) {
            str8_list_push(scratch.arena, &empty_sect_list, sect_n->data.name);
          }
        }
        for EachNode(name_n, String8Node, empty_sect_list.first) {
          lnk_section_table_purge(sectab, name_n->string);
        }
      }

      // assign section indices to sections
      for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
        lnk_assign_section_index(&sect_n->data, sectab->next_sect_idx++);
      }

      // assing layout offsets and sizes to merged sections
      for EachNode(sect_n, LNK_SectionNode, sectab->merge_list.first) {
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
      for EachIndex(obj_idx, objs_count) { task.u.patch_symtabs.was_symbol_patched[obj_idx] = push_array(temp.arena, B8, objs[obj_idx]->coff.header.symbol_count); }

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
        LNK_Section             *reloc          = lnk_image_section_table_push(config, sectab, str8_lit(".reloc"), PE_RELOC_SECTION_FLAGS);
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
    {
      // This is the first touch of the freshly committed ~image-size buffer: every
      // page is a demand-zero fault. Serial on main, that soft-fault storm (plus the
      // stream-set itself) parks all workers for the duration; range-split it across
      // the pool instead. Split points are PAGE-ALIGNED in the image buffer (the
      // reservation is page-aligned) so no two workers ever touch the same 4K page.
      // Writes are value-identical to the serial loop and byte-disjoint -> byte-safe.
      Temp fill_temp = temp_begin(scratch.arena);
      U64  range_quantum = MB(4);

      // upper bound on range count
      U64 range_cap = 0;
      for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
        range_cap += CeilIntegerDiv(sect_n->data.fsize, range_quantum) + 1;
      }

      LNK_FillAlignRange *ranges      = push_array_no_zero(fill_temp.arena, LNK_FillAlignRange, range_cap);
      U64                 range_count = 0;
      for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
        LNK_Section *sect      = &sect_n->data;
        U8           fill_byte = sect->flags & COFF_SectionFlag_CntCode ? coff_code_align_byte_from_machine(config->machine) : 0;
        U64          pos       = sect->foff;
        U64          end       = sect->foff + sect->fsize;
        for (; pos < end; ) {
          U64 next = AlignDownPow2(pos + range_quantum, KB(4));
          next     = ClampTop(next, end);
          if (next <= pos) { next = end; }
          Assert(range_count < range_cap);
          LNK_FillAlignRange *range = &ranges[range_count++];
          range->dst  = image_data.str + pos;
          range->size = next - pos;
          range->byte = fill_byte;
          pos = next;
        }
      }

      // write-once into the image buffer -> stream past the cache (see lnk_stream_set);
      // each task sfences its own NT stores before signalling completion, so after the
      // join every fill below is globally visible to the contrib-fill / reloc passes
      tp_for_parallel(tp, 0, range_count, lnk_fill_align_bytes_task, ranges);

      temp_end(fill_temp);
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
      // Streaming-ring P3.3: default to windowed $S consumption (patcher skips $S copies;
      // the module-write visit re-reads + patches into a reused per-worker window).
      // /OPT:GCTYPES needs the persistent patched+fixed-up $S backing (it reads and rewrites
      // type indices in place between the merge and the PDB build) -- keep the old copy-based
      // path wholesale there.
      g_debug_s_window = (config->opt_gc_types != LNK_SwitchState_Yes);

      // dedicated per-worker arenas for the patched debug-section copies: free-list
      // block reuse keeps the pages warm (a raw reserve+commit per copy paid ~11.6GB of
      // fresh zero-page faults per link at FN scale), and lnk_build_pdb hands the whole
      // set back with arena_release after the last $S reader
      g_sect_copy_arena_count = tp->worker_count;
      g_sect_copy_arenas      = push_array(arena->v[0], Arena *, g_sect_copy_arena_count);
      for EachIndex(i, g_sect_copy_arena_count) { g_sect_copy_arenas[i] = arena_alloc(.name = "SECT_DATA_COPIES"); }

      LNK_ObjRelocPatcher task = { .image_data = image_data, .objs = objs, .image_base = pe.image_base, .image_section_table = image_section_table };
      tp_for_parallel_prof(tp, arena, objs_count, lnk_obj_reloc_patcher, &task, "Patch Relocs"); // arena: the per-obj section_data_copies String8 tables
    }

    // patch load config
    {
      LNK_Symbol *load_config_symbol = lnk_symbol_table_search(symtab, str8_lit(MSCRT_LOAD_CONFIG_SYMBOL_NAME));
      if (load_config_symbol) {
        U64     load_config_foff   = lnk_foff_from_symbol(image_section_table, load_config_symbol);
        String8 load_config_data   = str8_skip(image_data, load_config_foff);

        U32 load_config_size = 0;
        if (sizeof(load_config_size) <= load_config_data.size) {
          MemoryCopyStruct(&load_config_size, load_config_data.str); // TODO: load config
          PE_DataDirectory *load_config_dir = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_LOAD_CONFIG);
          load_config_dir->virt_off  = lnk_voff_from_symbol(image_section_table, load_config_symbol);
          load_config_dir->virt_size = load_config_size;
        } else {
          // TODO: report corrupted load config
        }
      }
    }

    // patch exceptions
    {
      LNK_Section *pdata_sect = lnk_image_section_table_search(config, sectab, str8_lit(".pdata"), PE_PDATA_SECTION_FLAGS);
      if (pdata_sect) {
        String8 raw_pdata = str8_substr(image_data, rng_1u64(pdata_sect->foff, pdata_sect->foff + pdata_sect->vsize));
        pe_pdata_sort(config->machine, raw_pdata);

        PE_DataDirectory *pdata_dir = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_EXCEPTIONS);
        pdata_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, pdata_sect);
        pdata_dir->virt_size = lnk_get_section_contrib_size(pdata_sect);
      }
    }

    // patch export
    {
      LNK_Section *edata_sect = lnk_image_section_table_search(config, sectab, str8_lit(".edata"), PE_EDATA_SECTION_FLAGS);
      if (edata_sect) {
        PE_DataDirectory   *export_dir          = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_EXPORT);
        LNK_SectionContrib *edata_first_contrib = lnk_get_first_section_contrib(edata_sect);
        LNK_SectionContrib *edata_last_contrib  = lnk_get_last_section_contrib(edata_sect);
        export_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, edata_sect);
        export_dir->virt_size = lnk_get_section_contrib_size(edata_sect);
      }
    }

    // patch base relocs
    {
      LNK_Section *reloc_sect = lnk_image_section_table_search(config, sectab, str8_lit(".reloc"), PE_RELOC_SECTION_FLAGS);
      if (reloc_sect) {
        PE_DataDirectory *reloc_dir = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_BASE_RELOC);
        reloc_dir->virt_off  = lnk_get_first_section_contrib_voff(image_section_table, reloc_sect);
        reloc_dir->virt_size = lnk_get_section_contrib_size(reloc_sect);
      }
    }

    // patch import and import addr
    {
      LNK_Section *idata_sect       = lnk_image_section_table_search(config, sectab, str8_lit(".idata"), PE_IDATA_SECTION_FLAGS);
      LNK_Symbol  *null_import_desc = lnk_symbol_table_searchf(symtab, "__NULL_IMPORT_DESCRIPTOR");
      LNK_Symbol  *null_thunk_data  = lnk_symbol_table_searchf(symtab, "\x7f%S_NULL_THUNK_DATA", lnk_get_image_name(config));
      if (idata_sect && null_import_desc && null_thunk_data) {
        COFF_ParsedSymbol   null_import_desc_parsed = lnk_parsed_from_symbol(null_import_desc);
        LNK_SectionContrib *idata_first_contrib     = lnk_get_first_section_contrib(idata_sect);
        PE_DataDirectory   *import_dir              = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_IMPORT);
        import_dir->virt_off  = image_section_table[idata_first_contrib->u.sect_idx + 1]->voff + idata_first_contrib->u.off;
        import_dir->virt_size = null_import_desc_parsed.value - idata_first_contrib->u.off + sizeof(PE_ImportEntry);

        COFF_ParsedSymbol  null_thunk_data_parsed = lnk_parsed_from_symbol(null_thunk_data);
        U64                null_thunk_data_voff   = image_section_table[null_thunk_data_parsed.section_number]->voff + null_thunk_data_parsed.value;
        U64                first_import_foff      = image_section_table[idata_first_contrib->u.sect_idx+1]->foff + idata_first_contrib->u.off;
        PE_ImportEntry    *first_import           = str8_deserial_get_raw_ptr(image_data, first_import_foff, sizeof(*first_import));
        PE_DataDirectory  *import_addr_dir        = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_IMPORT_ADDR);
        import_addr_dir->virt_off  = first_import->import_addr_table_voff;
        import_addr_dir->virt_size = null_thunk_data_voff - first_import->import_addr_table_voff /* null */ + coff_word_size_from_machine(config->machine);
      }
    }

    // patch delay imports
    {
      LNK_Section *didat_sect       = lnk_image_section_table_search(config, sectab, str8_lit(".didat"), PE_IDATA_SECTION_FLAGS);
      LNK_Symbol  *null_import_desc = lnk_symbol_table_search(symtab, str8_lit("__NULL_DELAY_IMPORT_DESCRIPTOR"));
      LNK_Symbol  *last_null_thunk  = lnk_symbol_table_searchf(symtab,"\x7f%S_NULL_THUNK_DATA_DLA", lnk_get_image_name(config));
      if (didat_sect && null_import_desc && last_null_thunk) {
        COFF_ParsedSymbol   null_import_desc_parsed = lnk_parsed_from_symbol(null_import_desc);
        LNK_SectionContrib *didat_first_contrib     = lnk_get_first_section_contrib(didat_sect);
        PE_DataDirectory   *import_dir              = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_DELAY_IMPORT);
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
        LNK_Section *tls_sect  = lnk_image_section_table_search(config, sectab, str8_lit(".tls"), PE_TLS_SECTION_FLAGS);
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
        PE_DataDirectory *tls_dir = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_TLS);
        tls_dir->virt_off  = lnk_voff_from_symbol(image_section_table, tls_used_symbol);
        tls_dir->virt_size = is_tls_header64 ? sizeof(PE_TLSHeader64) : sizeof(PE_TLSHeader32);

        ProfEnd();
      }
    }

    // patch debug
    {
      LNK_Section *debug_dir_sect = lnk_image_section_table_search(config, sectab, str8_lit(".RAD_LINK_PE_DEBUG_DIR"), PE_RDATA_SECTION_FLAGS);
      if (debug_dir_sect) {
        // patch directory
        PE_DataDirectory *debug_dir = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_DEBUG);
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
      LNK_Section *rsrc_sect = lnk_image_section_table_search(config, sectab, str8_lit(".rsrc"), PE_RSRC_SECTION_FLAGS);
      if (rsrc_sect) {
        PE_DataDirectory *rsrc_dir = pe_data_directory_from_idx(image_data, &pe, PE_DataDirectoryIndex_RESOURCES);
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

typedef struct LNK_MapSection
{
  String8 name;
  U32     section_number;
  U32     off;
  U32     end;
  B32     is_code;
} LNK_MapSection;

typedef struct LNK_MapSymbol
{
  String8 name;
  String8 source;
  U64     va;
  U32     section_number;
  U32     off;
  B32     is_func;
} LNK_MapSymbol;

internal U32
lnk_map_image_section_number_from_voff(COFF_SectionHeader **section_table, U64 section_count, U64 voff)
{
  for (U32 section_number = 1; section_number <= section_count; section_number += 1) {
    COFF_SectionHeader *header = section_table[section_number];
    if (voff == header->voff) { return section_number; }
  }
  for (U32 section_number = 1; section_number <= section_count; section_number += 1) {
    COFF_SectionHeader *header = section_table[section_number];
    U64 section_size = Max(header->vsize, header->fsize);
    if (header->voff < voff && voff <= header->voff + section_size) { return section_number; }
  }
  return 0;
}

internal int
lnk_map_section_is_before(void *raw_a, void *raw_b)
{
  LNK_MapSection *a = raw_a, *b = raw_b;
  if (a->section_number != b->section_number) { return a->section_number < b->section_number; }
  if (a->off != b->off)                       { return a->off < b->off; }
  return str8_is_before_case_sensitive(&a->name, &b->name);
}

internal int
lnk_map_symbol_is_before(void *raw_a, void *raw_b)
{
  LNK_MapSymbol *a = *(LNK_MapSymbol **)raw_a, *b = *(LNK_MapSymbol **)raw_b;
  if (a->va != b->va)                         { return a->va < b->va; }
  if (a->section_number != b->section_number) { return a->section_number < b->section_number; }
  if (a->off != b->off)                       { return a->off < b->off; }
  return str8_is_before_case_sensitive(&a->name, &b->name);
}

internal String8
lnk_map_source_from_obj(Arena *arena, LNK_Obj *obj)
{
  if (obj == 0 || str8_starts_with(obj->path, str8_lit("*"))) {
    return str8_lit("<linker-defined>");
  }
  String8 obj_name = str8_skip_last_slash(obj->path);
  LNK_Lib *lib = lnk_obj_get_lib(obj);
  if (lib) {
    String8 lib_name = str8_chop_last_dot(str8_skip_last_slash(lib->path));
    return push_str8f(arena, "%S:%S", lib_name, obj_name);
  }
  return obj_name;
}

internal void
lnk_map_push_symbol_line(Arena *arena, String8List *map, LNK_MapSymbol *symbol)
{
  String8 function_marker = symbol->is_func ? str8_lit("f") : str8_lit(" ");
  str8_list_pushf(arena, map, " %04x:%08x       %-26S %016llx %S   %S\r\n",
                  symbol->section_number, symbol->off, symbol->name, symbol->va, function_marker, symbol->source);
}

internal String8List
lnk_build_map(Arena *arena, String8 image_data, LNK_Config *config, LNK_SymbolTable *symtab, LNK_SectionTable *sectab, U64 objs_count, LNK_Obj **objs)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  PE_BinInfo           pe                  = pe_bin_info_from_data(scratch.arena, image_data);
  COFF_SectionHeader **image_section_table = coff_section_table_from_data(scratch.arena, image_data, pe.section_table_range);
  String8List          map                 = {0};

  String8 image_name = str8_chop_last_dot(str8_skip_last_slash(config->out_path));
  DateTime universal_time = date_time_from_unix_time(config->time_stamp);
  DateTime local_time     = local_from_universal_time(&universal_time);
  String8 week_day        = string_from_week_day(local_time.week_day);
  String8 month           = string_from_month(local_time.month);

  str8_list_pushf(arena, &map, " %S\r\n\r\n", image_name);
  str8_list_pushf(arena, &map, " Timestamp is %08x (%S %S %02u %02u:%02u:%02u %04u)\r\n\r\n",
                  config->time_stamp, week_day, month, local_time.day, local_time.hour, local_time.min, local_time.sec, local_time.year);
  str8_list_pushf(arena, &map, " Preferred load address is %016llx\r\n\r\n", pe.image_base);
  str8_list_pushf(arena, &map, " Start         Length     Name                   Class\r\n");

  U64 section_cap = 0;
  for EachNode(sect_n, LNK_SectionNode, sectab->list.first) { section_cap += sect_n->data.contribs.chunk_count; }
  LNK_MapSection *sections = push_array(scratch.arena, LNK_MapSection, section_cap);
  U64             section_count = 0;
  HashMap        *section_maps = push_array(scratch.arena, HashMap, pe.section_count + 1);
  for EachNode(sect_n, LNK_SectionNode, sectab->list.first) {
    LNK_Section *image_section = &sect_n->data;
    U32 image_section_number = safe_cast_u32(image_section->sect_idx + 1);
    if (image_section_number > pe.section_count) { continue; }

    for EachNode(chunk, LNK_SectionContribChunk, image_section->contribs.first) {
      String8 name = chunk->sort_idx;
      if (name.size == 0) { continue; }

      LNK_MapSection *section = hash_map_search_string_raw(&section_maps[image_section_number], name);
      for EachIndex(sc_idx, chunk->count) {
        LNK_SectionContrib *sc = chunk->v[sc_idx];
        U32 off = sc->u.off;
        U32 end = off + safe_cast_u32(lnk_size_from_section_contrib(sc));
        if (section == 0) {
          Assert(section_count < section_cap);
          section = &sections[section_count++];
          *section = (LNK_MapSection){ name, image_section_number, off, end, !!(image_section->flags & COFF_SectionFlag_CntCode) };
          hash_map_push_string_raw(scratch.arena, &section_maps[image_section_number], name, section);
        } else {
          section->off = Min(section->off, off);
          section->end = Max(section->end, end);
        }
      }
    }
  }
  if (section_count > 1) { radsort(sections, section_count, lnk_map_section_is_before); }
  for EachIndex(i, section_count) {
    LNK_MapSection *section = &sections[i];
    str8_list_pushf(arena, &map, " %04x:%08x %08xH %-22S %s\r\n",
                    section->section_number, section->off, section->end - section->off,
                    section->name, section->is_code ? "CODE" : "DATA");
  }

  str8_list_pushf(arena, &map, "\r\n  Address         Publics by Value              Rva+Base               Lib:Object\r\n\r\n");

  U64 public_cap = 0;
  for EachIndex(worker_idx, symtab->arena->count) {
    for EachNode(chunk, LNK_SymbolHashTrieChunk, symtab->chunks[worker_idx].first) { public_cap += chunk->count; }
  }
  LNK_MapSymbol *publics = push_array(scratch.arena, LNK_MapSymbol, public_cap);
  U64 public_count = 0;
  for EachIndex(worker_idx, symtab->arena->count) {
    for EachNode(chunk, LNK_SymbolHashTrieChunk, symtab->chunks[worker_idx].first) {
      for EachIndex(i, chunk->count) {
        LNK_Symbol *symbol = chunk->v[i].symbol;
        if (symbol == 0 || str8_match(symbol->name, str8_lit(LNK_NULL_SYMBOL), 0) || str8_match(symbol->name, str8_lit(LNK_IMPORT_STUB), 0)) { continue; }

        LNK_ObjSymbolRef  ref    = lnk_ref_from_symbol(symbol);
        COFF_ParsedSymbol parsed = lnk_parsed_from_symbol(symbol);
        COFF_SymbolValueInterpType interp = coff_interp_from_parsed_symbol(parsed);
        LNK_MapSymbol map_symbol = { .name = symbol->name, .is_func = COFF_SymbolType_IsFunc(parsed.type) };

        if (interp == COFF_SymbolValueInterp_Regular) {
          if (parsed.section_number == lnk_obj_get_removed_section_number(ref.obj) || parsed.section_number == 0 || parsed.section_number > pe.section_count) { continue; }
          map_symbol.section_number = parsed.section_number;
          map_symbol.off            = safe_cast_u32(parsed.value);
          map_symbol.va             = pe.image_base + image_section_table[parsed.section_number]->voff + parsed.value;

          COFF_ParsedSymbol original = coff_parse_symbol_no_name(ref.obj->coff.header, lnk_coff_symbol_table_from_obj(ref.obj), ref.symbol_idx);
          COFF_SymbolValueInterpType original_interp = coff_interp_from_parsed_symbol(original);
          map_symbol.source = original_interp == COFF_SymbolValueInterp_Common ? str8_lit("<common>") : lnk_map_source_from_obj(scratch.arena, ref.obj);
        } else if (interp == COFF_SymbolValueInterp_Abs) {
          map_symbol.off = safe_cast_u32(parsed.value);
          if (str8_match(symbol->name, str8_lit("__ImageBase"), 0)) {
            map_symbol.va     = pe.image_base;
            map_symbol.source = str8_lit("<linker-defined>");
          } else {
            map_symbol.va     = parsed.value;
            map_symbol.source = str8_lit("<absolute>");
          }
        } else {
          continue;
        }

        Assert(public_count < public_cap);
        publics[public_count++] = map_symbol;
      }
    }
  }
  LNK_MapSymbol **public_ptrs = push_array_no_zero(scratch.arena, LNK_MapSymbol *, public_count);
  for EachIndex(i, public_count) { public_ptrs[i] = &publics[i]; }
  if (public_count > 1) { radsort(public_ptrs, public_count, lnk_map_symbol_is_before); }
  for EachIndex(i, public_count) { lnk_map_push_symbol_line(arena, &map, public_ptrs[i]); }

  U32 entry_section_number = lnk_map_image_section_number_from_voff(image_section_table, pe.section_count, pe.entry_point);
  U32 entry_section_off = entry_section_number ? pe.entry_point - image_section_table[entry_section_number]->voff : pe.entry_point;
  str8_list_pushf(arena, &map, "\r\n entry point at        %04x:%08x\r\n\r\n Static symbols\r\n\r\n", entry_section_number, entry_section_off);

  U64 static_cap = 0;
  for EachIndex(obj_idx, objs_count) { static_cap += objs[obj_idx]->coff.symbols.count; }
  LNK_MapSymbol *statics = push_array(scratch.arena, LNK_MapSymbol, static_cap);
  U64 static_count = 0;
  for EachIndex(obj_idx, objs_count) {
    LNK_Obj *obj = objs[obj_idx];
    String8 string_table = lnk_coff_string_table_from_obj(obj);
    String8 symbol_table = lnk_coff_symbol_table_from_obj(obj);
    for LNK_EachCoffSymbol(it, obj) {
      COFF_ParsedSymbol original = coff_parse_symbol(obj->coff.header, string_table, symbol_table, safe_cast_u32(it.symbol_idx));
      if (original.storage_class != COFF_SymStorageClass_Static || coff_interp_from_parsed_symbol(original) != COFF_SymbolValueInterp_Regular) { continue; }
      if (str8_starts_with(original.name, str8_lit("$pdata$"))) { continue; }
      if (original.section_number == 0 || original.section_number > obj->coff.sections.count_no_null) { continue; }
      if (*lnk_obj_section_from_section_number(obj, original.section_number).flags & (COFF_SectionFlag_LnkInfo | LNK_SECTION_FLAG_DEBUG)) { continue; }
      if (original.type.v == 0 && original.value == 0 && original.aux_symbol_count > 0 &&
          str8_match(original.name, lnk_obj_section_name_from_section_number(obj, original.section_number), 0)) { continue; }

      COFF_ParsedSymbol parsed = it.v;
      if (coff_interp_from_parsed_symbol(parsed) != COFF_SymbolValueInterp_Regular || parsed.section_number == 0 || parsed.section_number > pe.section_count) { continue; }

      Assert(static_count < static_cap);
      statics[static_count++] = (LNK_MapSymbol){
        .name           = original.name,
        .source         = lnk_map_source_from_obj(scratch.arena, obj),
        .va             = pe.image_base + image_section_table[parsed.section_number]->voff + parsed.value,
        .section_number = parsed.section_number,
        .off            = safe_cast_u32(parsed.value),
        .is_func        = COFF_SymbolType_IsFunc(parsed.type),
      };
    }
  }
  LNK_MapSymbol **static_ptrs = push_array_no_zero(scratch.arena, LNK_MapSymbol *, static_count);
  for EachIndex(i, static_count) { static_ptrs[i] = &statics[i]; }
  if (static_count > 1) { radsort(static_ptrs, static_count, lnk_map_symbol_is_before); }
  for EachIndex(i, static_count) { lnk_map_push_symbol_line(arena, &map, static_ptrs[i]); }

  scratch_end(scratch);
  ProfEnd();
  return map;
}

internal void
lnk_write_thread(void *raw_ctx)
{
  ProfBeginFunction();
  lnk_summary_phase_begin(LNK_SummaryPhase_Write);
  LNK_WriteThreadContext *ctx = raw_ctx;
  lnk_write_data_to_file_path(ctx->path, ctx->temp_path, ctx->data);
  lnk_summary_phase_end(LNK_SummaryPhase_Write);
  ProfEnd();
}

////////////////////////////////////////////////////////////////////////////////
//~ One-line end-of-link summary (always on; production triage). Everything
//  needed at print time is stashed in this global as the link progresses, so
//  the line can be emitted best-effort from ANY exit path (lnk_exit on error,
//  entry_point on success) with whatever was known by then.

typedef struct LNK_SummaryInfo
{
  volatile U32 printed;      // print-exactly-once latch
  U64          start_us;     // set first thing in entry_point
  U64          t0_ms;        // UTC ms epoch at link start (t1 is stamped at print time)
  U64          worker_count;
  U64          objs_count;
  U64          input_bytes;  // sum of obj data sizes (lib members count their slice)
  U64          libs_count;
  // physical-memory samples (GlobalMemoryStatusEx): storm triage -- prod storms
  // show pdb-phase kernel time exploding 54x for the same fault count, fitting
  // free-list exhaustion / page-repurpose; these 3 samples prove/refute that
  U64          mem_avail_t0;  // ullAvailPhys at link start
  U64          mem_avail_pdb; // ullAvailPhys at pdb-phase start (0 = phase never ran)
  U32          mem_load_max;  // max dwMemoryLoad seen across the samples
  // name COPIES: config strings parsed out of an @rsp point into the response
  // file buffer, whose scratch dies right after config parse -- capture the
  // bytes here instead of keeping String8s into freed memory
  U64          out_name_size;
  U64          pool_name_size; // non-zero => /RAD_SHARED_THREAD_POOL
  U8           out_name [128];
  U8           pool_name[128];
} LNK_SummaryInfo;

global LNK_SummaryInfo g_summary_info;

internal void
lnk_summary_copy_name(U8 *dst, U64 dst_cap, U64 *dst_size_out, String8 name)
{
  U64 size = Min(name.size, dst_cap);
  MemoryCopy(dst, name.str, size);
  *dst_size_out = size;
}

internal U64
lnk_summary_us_from_timer(LNK_TimerType timer)
{
  // guard against a fatal exit mid-phase (begin stamped, end still zero)
  return g_timers[timer].end > g_timers[timer].begin ? g_timers[timer].end - g_timers[timer].begin : 0;
}

internal LNK_SummaryCounters
lnk_summary_counters_from_timer(LNK_TimerType timer)
{
  LNK_SummaryCounters zero = {0};
  // same mid-phase guard as lnk_summary_us_from_timer
  if (g_timers[timer].end <= g_timers[timer].begin) { return zero; }
  return lnk_summary_counters_sub_sat(g_timer_counters_end[timer], g_timer_counters_begin[timer]);
}

// one GlobalMemoryStatusEx sample for the summary line: returns available
// physical bytes and folds dwMemoryLoad into the running max. 1 syscall per
// call, called 3x per link (link start, pdb-phase start, print time).
internal U64
lnk_summary_sample_mem(void)
{
  U64 avail = 0;
#if OS_WINDOWS
  MEMORYSTATUSEX msx = { sizeof(msx) };
  if (GlobalMemoryStatusEx(&msx)) {
    avail = msx.ullAvailPhys;
    if (msx.dwMemoryLoad > g_summary_info.mem_load_max) { g_summary_info.mem_load_max = msx.dwMemoryLoad; }
  }
#endif
  return avail;
}

internal U64
lnk_summary_utc_ms(void)
{
#if OS_WINDOWS
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  U64 t100 = ((U64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  return (t100 - 116444736000000000ULL) / 10000; // FILETIME epoch -> unix ms epoch
#else
  return 0;
#endif
}

// one phase bucket -> "wall-ms/user-ms/kernel-ms/faults-K" (process-wide deltas
// at the bucket's boundaries; user can exceed wall on parallel phases, and a
// bucket that overlaps another thread's work counts that work too)
internal String8
lnk_summary_str_from_counters(Arena *arena, LNK_SummaryCounters c)
{
  return push_str8f(arena, "%llu/%llu/%llu/%llu", c.wall_us / 1000, c.user_us / 1000, c.kern_us / 1000, c.faults / 1000);
}

internal void
lnk_print_summary(int exit_code)
{
  // run exactly once, no matter which exit path gets here first
  if (ins_atomic_u32_eval_cond_assign(&g_summary_info.printed, 1, 0) != 0) {
    return;
  }

  // detach from the shared-pool cross-process counter on every exit path, even
  // when the summary line is off -- the linker leaves through _exit and never
  // runs tp_release, so this is the only place the counter gets decremented
  F64 pool_grant_avg = 0, pool_park_seconds = 0;
  U32 pool_procs_now = 0, pool_procs_peak = 0;
  B32 pool_on = (g_summary_info.pool_name_size > 0);
  if (pool_on) {
    tp_stats_snapshot(&pool_grant_avg, &pool_park_seconds);
    tp_procs_snapshot(&pool_procs_now, &pool_procs_peak);
    tp_procs_detach();
  }

  // the line itself is opt-in (/RAD_LOG:Summary) -- always-on turned out to be
  // noise in build logs; farm convoy triage passes the switch explicitly
  if (!lnk_get_log_status(LNK_Log_Summary)) {
    return;
  }

  Temp scratch = scratch_begin(0, 0);

  F64 wall = g_summary_info.start_us ? (F64)(now_time_us() - g_summary_info.start_us) / 1000000.0 : 0;

  // process CPU + memory counters
  F64 user_time = 0, kernel_time = 0, peak_ws_gib = 0, page_faults_m = 0, peak_commit_gib = 0;
  U32 cow_promoted_pages = 0;
#if OS_WINDOWS
  {
    FILETIME create_ft, exit_ft, kernel_ft, user_ft;
    if (GetProcessTimes(GetCurrentProcess(), &create_ft, &exit_ft, &kernel_ft, &user_ft)) {
      user_time   = (F64)(((U64)user_ft.dwHighDateTime   << 32) | user_ft.dwLowDateTime)   / 10000000.0;
      kernel_time = (F64)(((U64)kernel_ft.dwHighDateTime << 32) | kernel_ft.dwLowDateTime) / 10000000.0;
    }
    PROCESS_MEMORY_COUNTERS pmc = { (DWORD)sizeof(pmc) };
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
      peak_ws_gib   = (F64)pmc.PeakWorkingSetSize / (F64)GB(1);
      page_faults_m = (F64)pmc.PageFaultCount / 1000000.0;
      // peak pagefile-backed commit charge -- the number build-farm memory admission
      // sees; with read-only input views this tracks ws minus the mapped input set
      peak_commit_gib = (F64)pmc.PeakPagefileUsage / (F64)GB(1);
    }
    cow_promoted_pages = (U32)g_lnk_cow_promoted_pages;
  }
#endif

  // process IO totals: hard page-ins on mapped inputs surface as read bytes,
  // UBA-detoured output writes as write bytes
  U64 io_read_mb = 0, io_write_mb = 0;
#if OS_WINDOWS
  {
    IO_COUNTERS ioc = {0};
    if (GetProcessIoCounters(GetCurrentProcess(), &ioc)) {
      io_read_mb  = ioc.ReadTransferCount  / MB(1);
      io_write_mb = ioc.WriteTransferCount / MB(1);
    }
  }
#endif

  // phase triplets. img/dbg/pdb come from the /RAD_LOG:TIMERS stamps; dbg is
  // the debug-info umbrella minus the PDB/RDI sub-phases it contains.
  LNK_SummaryCounters img_c = lnk_summary_counters_from_timer(LNK_Timer_Image);
  LNK_SummaryCounters pdb_c = lnk_summary_counters_from_timer(LNK_Timer_Pdb);
  LNK_SummaryCounters rdi_c = lnk_summary_counters_from_timer(LNK_Timer_Rdi);
  LNK_SummaryCounters dbg_c = lnk_summary_counters_sub_sat(lnk_summary_counters_sub_sat(lnk_summary_counters_from_timer(LNK_Timer_Debug), pdb_c), rdi_c);

  // residual catch-alls: umbrella bucket minus the sum of its printed
  // sub-buckets, clamped at 0 per field (a /PDBSTRIPPED link runs the pdb
  // sub-phases a second time OUTSIDE the Timer_Pdb bracket, which can push the
  // sub-bucket sum past the umbrella -- clamp instead of printing garbage).
  // Storm triage: prod shows the pdbg sub-buckets covering only ~19% of pdb
  // kernel time in a storm window vs ~96% locally -- other= pins the
  // uncovered span without waiting for a local repro.
  LNK_SummaryCounters pdb_other, dbg_other;
  {
    LNK_SummaryCounters pdbg_sum = g_summary_phase[LNK_SummaryPhase_PdbGsi];
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbHsh]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbIni]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbSym]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbMod]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbTpi]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbStr]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbSc]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbMsf]);
    pdbg_sum = lnk_summary_counters_add(pdbg_sum, g_summary_phase[LNK_SummaryPhase_PdbWr]);
    pdb_other = lnk_summary_counters_sub_sat(pdb_c, pdbg_sum);

    LNK_SummaryCounters dbgg_sum = lnk_summary_counters_add(g_summary_phase[LNK_SummaryPhase_DbgMcvi], g_summary_phase[LNK_SummaryPhase_DbgMerge]);
    dbg_other = lnk_summary_counters_sub_sat(dbg_c, dbgg_sum);
  }

  // governor stats snapshotted above, before the detach
  String8 pool_stats = str8_zero();
  if (pool_on) {
    pool_stats = push_str8f(scratch.arena, " pool=%S grant_avg=%.1f park=%.1f procs=%u/%u",
                            str8(g_summary_info.pool_name, g_summary_info.pool_name_size), pool_grant_avg, pool_park_seconds, pool_procs_now, pool_procs_peak);
  }

  // final memory sample (t1) -- 3rd and last GlobalMemoryStatusEx of the link
  U64 mem_avail_t1 = lnk_summary_sample_mem();

  lnk_fprintf(stdout,
              "[radlink summary] v=3 out=%S exit=%d t0=%llu t1=%llu wall=%.1f user=%.1f kern=%.1f ws=%.1fG cm=%.1fG cowp=%u pf=%.1fM io=%llu/%lluMB mem=%.1f/%.1f/%.1f/%u workers=%llu%S"
              " in=%lluo/%.1fG libs=%llu"
              " ph[inp=%S res=%S icf=%S ref=%S img=%S dbg=%S pdb=%S wr=%S]"
              " dbgg[mcvi=%S merge=%S other=%S]"
              " pdbg[hsh=%S ini=%S gsi=%S sym=%S mod=%S tpi=%S str=%S sc=%S msf=%S wr=%S other=%S]\n",
              g_summary_info.out_name_size ? str8(g_summary_info.out_name, g_summary_info.out_name_size) : str8_lit("-"),
              exit_code,
              g_summary_info.t0_ms,
              lnk_summary_utc_ms(),
              wall,
              user_time,
              kernel_time,
              peak_ws_gib,
              peak_commit_gib,
              cow_promoted_pages,
              page_faults_m,
              io_read_mb,
              io_write_mb,
              (F64)g_summary_info.mem_avail_t0  / (F64)GB(1),
              (F64)g_summary_info.mem_avail_pdb / (F64)GB(1),
              (F64)mem_avail_t1                 / (F64)GB(1),
              g_summary_info.mem_load_max,
              g_summary_info.worker_count,
              pool_stats,
              g_summary_info.objs_count,
              (F64)g_summary_info.input_bytes / (F64)GB(1),
              g_summary_info.libs_count,
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_Input]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_Resolve]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_Icf]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_Ref]),
              lnk_summary_str_from_counters(scratch.arena, img_c),
              lnk_summary_str_from_counters(scratch.arena, dbg_c),
              lnk_summary_str_from_counters(scratch.arena, pdb_c),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_Write]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_DbgMcvi]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_DbgMerge]),
              lnk_summary_str_from_counters(scratch.arena, dbg_other),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbHsh]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbIni]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbGsi]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbSym]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbMod]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbTpi]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbStr]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbSc]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbMsf]),
              lnk_summary_str_from_counters(scratch.arena, g_summary_phase[LNK_SummaryPhase_PdbWr]),
              lnk_summary_str_from_counters(scratch.arena, pdb_other));

  scratch_end(scratch);
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

// scratch free-list blocks detached during the decommit pass, released on a
// background thread (see lnk_scratch_decommit_worker)
global Arena *g_detached_scratch_blocks = 0;
global Thread g_scratch_freelist_reaper = {0};

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
  //
  // NOTE(perf): redistributing these decommits in chunks across the pool does
  // NOT help: MEM_DECOMMIT serializes in the kernel on the process address-space
  // lock (~14 GB/s aggregate no matter the thread count; measured 9.3 GiB in
  // 677 ms chunked-parallel vs ~500 ms with this per-worker scheme). And handing
  // the ACTIVE-CHAIN decommit to a background thread is UNSAFE here: workers push
  // to these scratch arenas as soon as the PDB build starts, and a push would
  // re-commit pages that the background decommit then rips out.
  //
  // The FREE-LIST blocks are different: they hold no live data and are only
  // touched again when a grow pops them. On the editor link ~84% of the
  // decommitted bytes (9.4 of 11.3 GiB) sit in free-list blocks, so instead of
  // decommitting them here (serialized kernel work on the critical path), each
  // worker DETACHES its arenas' free chains (pointer ops, same thread => safe)
  // onto a global list that a background thread releases while the PDB build
  // runs. A post-detach grow simply sees an empty free list and reserves a
  // fresh block -- same cost as the re-commit it would have paid anyway.
  TCTX *tctx = tctx_selected();
  for EachIndex(arena_idx, ArrayCount(tctx->arenas)) {
    Arena *arena = tctx->arenas[arena_idx];
    if (arena == 0) { continue; }
#if ARENA_FREE_LIST
    // detach this arena's free chain and publish the blocks for background release
    for (Arena *block = arena->free_last, *block_next = 0; block != 0; block = block_next) {
      block_next = block->prev;
      for (;;) {
        Arena *head = (Arena *)ins_atomic_u64_eval(&g_detached_scratch_blocks);
        block->prev = head;
        if ((Arena *)ins_atomic_u64_eval_cond_assign((U64 *)&g_detached_scratch_blocks, (U64)block, (U64)head) == head) { break; }
      }
    }
    arena->free_last = 0;
#endif
    // decommit the committed-but-unused pages above the live pos (active chain)
    arena_decommit_unused(arena);
  }
  barrier_wait(tp->barrier);
}

// Releases the scratch free-list blocks detached by lnk_scratch_decommit_worker.
// Runs in the background: MEM_RELEASE serializes on the process address-space
// lock in the kernel, so on the main thread this would extend the decommit
// window 1:1; off the main thread it overlaps the PDB build.
internal void
lnk_detached_scratch_release_thread(void *raw)
{
  ProfBeginFunction();
  U64 begin_us = now_time_us();

  U64    released_bytes = 0;
  U64    released_count = 0;
  Arena *chain          = (Arena *)ins_atomic_u64_eval_assign((U64 *)&g_detached_scratch_blocks, 0);
  for (Arena *block = chain, *block_next = 0; block != 0; block = block_next) {
    block_next      = block->prev;
    released_bytes += block->cmt;
    released_count += 1;
    AsanUnpoisonMemoryRegion(block, block->cmt);
    release_memory(block, block->res);
  }

  lnk_log(LNK_Log_Timers, "[teardown] background release of %llu detached scratch blocks (%llu MiB committed) took %.2f ms (off main thread)",
          released_count, released_bytes / MB(1), (F64)(now_time_us() - begin_us) / 1000.0);
  ProfEnd();
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
  // Link Image
  //
  LNK_LinkResult link = lnk_link_image(tp, arena, config, inputer, symtab);

  U64       objs_count = link.objs.count;
  U64       libs_count = link.libs.count;
  LNK_Obj **objs       = lnk_array_from_obj_list(scratch.arena, link.objs);
  LNK_Lib **libs       = lnk_array_from_lib_list(scratch.arena, link.libs);

  // summary: input volume (lib members count their member slice)
  {
    U64 input_bytes = 0;
    for EachIndex(obj_idx, objs_count) { input_bytes += objs[obj_idx]->coff.data.size; }
    g_summary_info.objs_count  = objs_count;
    g_summary_info.libs_count  = libs_count;
    g_summary_info.input_bytes = input_bytes;
  }

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

  LNK_BackgroundFileWriter background_file_writer = {0};
  LNK_PdbWriter            pdb_writer = { .file_writer = &background_file_writer };
  Temp                     pdb_huge_temp = temp_begin(lnk_get_huge_arena());
  lnk_background_file_writer_begin(pdb_writer.file_writer);

  //
  // Map
  //
  if (config->map == LNK_SwitchState_Yes) {
    String8List map = lnk_build_map(scratch.arena, image_ctx.image_data, config, symtab, image_ctx.sectab, objs_count, objs);
    lnk_write_data_list_to_file_path(config->map_name, config->temp_map_name, map);
  }

  //
  // Import Library
  //
  if (config->build_imp_lib && (config->file_characteristics & PE_ImageFileCharacteristic_DLL)) {
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
    lnk_summary_phase_begin(LNK_SummaryPhase_DbgMcvi);
    LNK_CodeViewInput cv        = lnk_make_code_view_input(tp, arena, config, debug_info_objs_count, debug_info_objs, rrt_input);
    lnk_summary_phase_end(LNK_SummaryPhase_DbgMcvi);
    lnk_summary_phase_begin(LNK_SummaryPhase_DbgMerge);
    LNK_MergedTypes   cv_types  = lnk_merge_types(tp, arena, &cv, 0);
    lnk_summary_phase_end(LNK_SummaryPhase_DbgMerge);

    // Streaming-ring P2 slice A: $S TI/kind fixups are journaled in lnk_merge_types and
    // normally replayed per obj into the window at the module-write visit
    // (lnk_write_pdb_modules). /OPT:GCTYPES consumes fixed-up $S bytes in place right below
    // (mark roots + compaction rewrite) and runs with the window disabled (persistent patched
    // copies), so it still needs the eager whole-input replay. P3.3: a /PDBSTRIPPED-only
    // build no longer takes this path -- its pre-build stripping loop re-reads each Symbols
    // node through lnk_obj_window_debug_s, which applies relocs AND replays the journal per
    // node, so nothing is written into the raw mapped views.
    if (config->opt_gc_types == LNK_SwitchState_Yes) {
      lnk_apply_debug_s_fixups_eager(tp, &cv);
    }

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
      U64 decommit_begin_us = now_time_us();
      // task_count == worker_count + the in-worker barrier => every worker
      // (worker 0 IS the main thread) runs exactly once, covering main's scratch.
      tp_for_parallel_reserve(tp, 0, tp->worker_count, lnk_scratch_decommit_worker, 0); // BARRIER pass (path B)
      if (g_detached_scratch_blocks != 0) {
        g_scratch_freelist_reaper = thread_launch(lnk_detached_scratch_release_thread, 0);
      }
      lnk_log(LNK_Log_Timers, "[teardown] scratch decommit pass in %.2f ms", (F64)(now_time_us() - decommit_begin_us) / 1000.0);
      ProfEnd();
    }

    // Type merging is the last bulk consumer of type payload pages. Apply the configured cache
    // generation transition before PDB construction so type residency does not stack with the
    // later GSI and module-stream allocations.
    lnk_compressed_obj_trim_working_set();

    //
    // Debug Info
    //
    // TODO: Parallel debug info builds are currently blocked by the patch
    // strings in $$FILE_CHECKSUM step in `lnk_process_c13_data_task`.
    if (config->debug_mode == LNK_DebugMode_Full || config->rad_debug == LNK_SwitchState_Yes) {
      LNK_FileArtifact pdb_artifact = {0};
      {
        g_summary_info.mem_avail_pdb = lnk_summary_sample_mem();
        lnk_timer_begin(LNK_Timer_Pdb);
        lnk_summary_phase_begin(LNK_SummaryPhase_PdbHsh);
        if (config->pdb_hash_type_names != LNK_TypeNameHashMode_None) {
          lnk_replace_type_names_with_hashes(tp,
                                             arena,
                                             cv_types.count[CV_TypeIndexSource_TPI],
                                             cv_types.v    [CV_TypeIndexSource_TPI],
                                             config->pdb_hash_type_names,
                                             config->pdb_hash_type_name_length,
                                             config->pdb_hash_type_name_map);
        }
        lnk_summary_phase_end(LNK_SummaryPhase_PdbHsh);
        pdb_writer.output_path      = config->debug_mode == LNK_DebugMode_Full ? config->pdb_name      : str8_zero();
        pdb_writer.temp_output_path = config->debug_mode == LNK_DebugMode_Full ? config->temp_pdb_name : str8_zero();
        lnk_summary_phase_begin(LNK_SummaryPhase_PdbWr);
        pdb_artifact                = lnk_build_pdb(tp, arena, image_ctx.image_data, config, symtab, &cv, cv_types, pdb_writer, LNK_PDB_BuilderFlag_All, inputer);
        lnk_summary_phase_end(LNK_SummaryPhase_PdbWr);

        lnk_timer_end(LNK_Timer_Pdb);
      }

      if (config->rad_debug == LNK_SwitchState_Yes) {
        lnk_timer_begin(LNK_Timer_Rdi);

        LNK_P2R p2r = { .config = config, .pdb_data = lnk_data_from_file_artifact(lnk_get_huge_arena(), &pdb_artifact), .image_data = image_ctx.image_data };
        tp_for_parallel_reserve(tp, arena, tp->worker_count, lnk_p2r_worker, &p2r); // BARRIER pass (path B)

        String8List rdi_blobs = rdim_file_blobs_from_section_bundle(scratch.arena, &p2r.bake_results.section_bundle);
        lnk_write_data_list_to_file_path(config->rad_debug_name, config->temp_rad_debug_name, rdi_blobs);

        lnk_timer_end(LNK_Timer_Rdi);
      }
    }

    //
    // stripped PDB
    //
    if (config->pdb_stripped_name.size != 0) {
      // P3.3 fold: with the $S window enabled there are no patched copies and no persisted
      // fixup replay -- the raw mapped Symbols bytes are pre-reloc and pre-TI-fixup. Re-read
      // each obj's Symbols nodes through lnk_obj_window_debug_s (raw view -> window copy +
      // relocs + journal replay: exactly the bytes the module-write pass consumed); the
      // journal was kept alive across lnk_build_pdb for this (released below). The strip
      // loop already copies every surviving record out, so the window is transient per obj.
      // With the window disabled (/OPT:GCTYPES) the old flow is intact: free_sect_copies==0
      // kept the patched+eagerly-fixed copies alive and the nodes are read in place.
      PE_BinInfo           stripped_pe            = {0};
      COFF_SectionHeader **stripped_image_sectab  = 0;
      Temp                 wscratch               = scratch_begin(&scratch.arena, 1);
      if (g_debug_s_window) {
        stripped_pe           = pe_bin_info_from_data(scratch.arena, image_ctx.image_data);
        stripped_image_sectab = coff_section_table_from_data(scratch.arena, image_ctx.image_data, stripped_pe.section_table_range);
      }

      CV_DebugS *debug_s_arr = push_array(scratch.arena, CV_DebugS, cv.obj_count);
      for EachIndex(obj_idx, cv.obj_count) {

        Temp wtemp = temp_begin(wscratch.arena);

        CV_DebugS   *debug_s_dst = &debug_s_arr[obj_idx];
        CV_DebugS    debug_s_win = {0};
        String8List *src;
        if (g_debug_s_window) {
          debug_s_win = lnk_obj_window_debug_s(wtemp.arena, &cv, obj_idx, stripped_pe.image_base, stripped_image_sectab, 1 /* symbols_only */);
          src         = cv_sub_section_ptr_from_debug_s(&debug_s_win, CV_C13SubSectionKind_Symbols);
        } else {
          src         = cv_sub_section_ptr_from_debug_s(&cv.debug_s_arr[obj_idx], CV_C13SubSectionKind_Symbols);
        }

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
                // strip the type index in the DESTINATION copy -- the source $$S stays untouched
                // (patching the source would dirty its private/CoW backing pages for no reason)
                U64 rec_off = buffer_cursor;
                buffer_cursor += cv_write_symbol(buffer, buffer_cursor, buffer_size, &symbol, CV_SymbolAlign);
                memory_write32(buffer + rec_off + sizeof(CV_SymbolHeader) + OffsetOf(CV_SymProc32, itype), 0);
                buffer_cursor += cv_write_symbol(buffer, buffer_cursor, buffer_size, &(CV_Symbol){ .kind = CV_SymKind_END }, CV_SymbolAlign);
              }
            }
          }
          Assert(buffer_cursor == buffer_size);

          // synthesized bytes (TI-stripped copies): provenance marked synthetic
          cv_debug_s_push_synthetic_sub_section(scratch.arena, debug_s_dst, CV_C13SubSectionKind_Symbols, str8(buffer, buffer_size));
        }

        temp_end(wtemp); // window bytes are consumed (records copied into `buffer`)
      }
      scratch_end(wscratch);

      // last $S-journal reader on the stripped path is done (lnk_build_pdb kept the journal
      // alive when a /PDBSTRIPPED build follows); no-op when already consumed/never built
      lnk_release_debug_s_fixup_journal(&cv);

      LNK_CodeViewInput stripped_cv = {0};
      stripped_cv.config              = config;
      stripped_cv.is_stripped         = 1;
      stripped_cv.obj_arr             = cv.obj_arr;
      stripped_cv.obj_count           = cv.obj_count; 
      stripped_cv.count               = cv.obj_count;
      stripped_cv.debug_s_arr         = debug_s_arr;
      stripped_cv.symbol_input_ranges = push_array(scratch.arena, Rng1U64, tp->worker_count);

      // inputer==0: never early-release from the stripped build (and the first build was
      // already gated off by pdb_stripped_name) -- its window fills read the raw views
      LNK_FileArtifact pdb_artifact = lnk_build_pdb(tp, arena, image_ctx.image_data, config, symtab, &stripped_cv, (LNK_MergedTypes){0}, (LNK_PdbWriter){0}, LNK_PDB_BuilderFlag_All, 0);
      lnk_summary_phase_begin(LNK_SummaryPhase_PdbWr);
      lnk_write_data_list_to_file_path(config->pdb_stripped_name, str8f(scratch.arena, "%S.tmp", config->pdb_stripped_name), pdb_artifact.data);
      lnk_summary_phase_end(LNK_SummaryPhase_PdbWr);
    }

    lnk_timer_end(LNK_Timer_Debug);
    ProfEnd();
  }

#if OS_WINDOWS
  // for unexplained reasons, file mappings on Windows cause slow process exit times
  ProfBegin("Release Input File Maps");
  lnk_inputer_release_file_maps(tp, config->debug_worker_cap, inputer);
  ProfEnd();
#endif

  // PDB output borrows pages from the huge arena, so drain after map release
  lnk_background_file_writer_end(pdb_writer.file_writer);
  temp_end(pdb_huge_temp);

  // wait for the thread to finish writing image to disk
  thread_join(image_write_thread, -1);

  // reap the background arena-release thread, if one is still in flight
  if (g_arena_reaper_thread.u64[0] != 0) {
    thread_join(g_arena_reaper_thread, max_U64);
    MemoryZeroStruct(&g_arena_reaper_thread);
  }

  // reap the background scratch free-list release thread, if one was launched
  if (g_scratch_freelist_reaper.u64[0] != 0) {
    thread_join(g_scratch_freelist_reaper, max_U64);
    MemoryZeroStruct(&g_scratch_freelist_reaper);
  }

  // image is on disk and no longer read by anyone -- release its ~1GB now so the kernel reclaims it
  // concurrently with the remaining work + exit, not single-threaded in the process rundown.
  release_memory(image_ctx.image_data.str, image_ctx.image_data.size);
  //
  // Timers
  //
  {
    lnk_compressed_obj_log_stats();
    lnk_obj_log_compressed_census();
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
  {
    LNK_CmdLine default_line = lnk_cmd_line_from_stringf_windows_rules(scratch.arena, "/DEBUG:GHASH /NOD /RAD_WRITE_TEMP_FILES");
    for EachNode(cmd, LNK_CmdOption, default_line.first_option) {
      lnk_apply_cmd_option_to_config(config, cmd->string, cmd->value, &(LNK_Obj){0});
    }
  }

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
      if (obj->coff.debug_p_section_number > 0) {
        u64_list_push(scratch.arena, &include_obj_list, obj_idx);
      } else if (obj->coff.debug_t_section_number > 0) {
        // read first leaf
        String8 raw_debug_t = lnk_obj_section_data_from_number(obj, obj->coff.debug_t_section_number);
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

  LNK_RRT rrt = { .debug_types_hash = config->debug_types_hash };
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
  g_summary_info.start_us     = now_time_us();
  g_summary_info.t0_ms        = lnk_summary_utc_ms();
  g_summary_info.mem_avail_t0 = lnk_summary_sample_mem();
  lnk_log_begin();

  // init config from the command line
  LNK_Config *config = lnk_config_init(cmdline->argc, cmdline->argv);
  lnk_compressed_obj_configure(config);

  // Snapshot summary identity immediately after command-line parsing, before
  // pool initialization and later scratch allocations.
  lnk_summary_copy_name(g_summary_info.out_name,  sizeof(g_summary_info.out_name),  &g_summary_info.out_name_size,  str8_skip_last_slash(config->out_path));
  lnk_summary_copy_name(g_summary_info.pool_name, sizeof(g_summary_info.pool_name), &g_summary_info.pool_name_size, config->shared_thread_pool_name);
  g_summary_info.worker_count = config->worker_count;

  if (lnk_get_log_status(LNK_Log_Debug)) {
    lnk_fprintf(stderr, "--------------------------------------------------------------------------------\n");
    lnk_fprintf(stderr, "Command Line: %S\n", config->raw_cmd_line);
    lnk_fprintf(stderr, "Work Dir    : %S\n", config->work_dir);
    lnk_fprintf(stderr, "--------------------------------------------------------------------------------\n");
  }

  // init thread pool
  TP_Context *tp       = tp_alloc(scratch.arena, config->worker_count, config->max_worker_count, config->shared_thread_pool_name);
  TP_Arena   *tp_arena = tp_arena_alloc(tp);

  // pick entry point
  switch (config->boot_mode) {
  case LNK_BootMode_Linker:     lnk_run_linker     (tp, tp_arena, config); break;
  case LNK_BootMode_TypeServer: lnk_run_type_server(tp, tp_arena, config); break;
  }

  lnk_print_summary(0);

  lnk_log_end();
  scratch_end(scratch);
}
