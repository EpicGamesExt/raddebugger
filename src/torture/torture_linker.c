// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
t_codec_coff_section_name(String8 string_table, COFF_SectionHeader *header, String8 *name_out)
{
  String8 name = str8_cstring_capped(header->name, header->name + sizeof(header->name));
  if (name.size != 0 && name.str[0] == '/') {
    U64 offset = 0;
    if (!try_u64_from_str8_c_rules(str8_skip(name, 1), &offset) || offset >= string_table.size) { return 0; }
    name = str8_cstring_capped(string_table.str + offset, string_table.str + string_table.size);
    if (name.size >= string_table.size - offset) { return 0; }
  }
  *name_out = name;
  return 1;
}

internal void
t_codec_validate_fields(T_ParseContext *ctx, MD_Node *node, char **allowed)
{
  for
    MD_EachNode(child, node->first)
    {
      B32 is_allowed = 0;
      for (U64 i = 0; allowed[i] != 0; i += 1) {
        if (str8_match(child->string, str8_cstring(allowed[i]), StringMatchFlag_CaseInsensitive)) {
          is_allowed = 1;
          break;
        }
      }
      if (!is_allowed) { t_parse_errorf(ctx, T_ResultCode_ValidationError, child, "unknown field '%S'", child->string); }
      for (MD_Node *previous = node->first; previous != child; previous = previous->next) {
        if (str8_match(previous->string, child->string, StringMatchFlag_CaseInsensitive)) {
          t_parse_errorf(ctx, T_ResultCode_ValidationError, child, "duplicate field '%S'", child->string);
          break;
        }
      }
    }
}

internal void
t_codec_validate_scalar(T_ParseContext *ctx, MD_Node *node, char *name, B32 required)
{
  MD_Node *field = t_codec_child(node, name);
  if (md_node_is_nil(field)) {
    if (required) { t_parse_errorf(ctx, T_ResultCode_ValidationError, node, "operation requires %s", name); }
  } else if (md_node_is_nil(field->first) || !md_node_is_nil(field->first->next)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, field, "%s requires exactly one value", name);
  }
}

internal T_Result
t_codec_pe_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out)
{
  String8 data = artifact->data;
  if (!pe_check_magic(data)) { return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_zero(), "artifact is not a PE image"); }
  PE_BinInfo pe = pe_bin_info_from_data(ctx->arena, data);
  MD_Node *root = t_codec_push_node(ctx->arena, 0, str8_lit("pe"));
  t_codec_push_field(ctx, root, "is_pe32", pe.is_pe32 ? str8_lit("true") : str8_lit("false"));
  t_codec_push_field(ctx, root, "arch", string_from_arch(pe.arch));
  t_codec_push_field(ctx, root, "subsystem", pe_string_from_subsystem(pe.subsystem));
  t_codec_push_u64(ctx, root, "section_count", pe.section_count);
  t_codec_push_u64(ctx, root, "section_alignment", pe.virt_section_align);
  t_codec_push_u64(ctx, root, "file_alignment", pe.file_section_align);
  t_codec_push_u64(ctx, root, "symbol_count", pe.symbol_count);
  t_codec_push_u64(ctx, root, "data_directory_count", pe.data_dir_count);
  t_codec_push_u64(ctx, root, "entry_point", pe.entry_point);

  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(data, pe.section_table_range).str;
  String8 string_table = str8_substr(data, pe.string_table_range);
  MD_Node *section_list = t_codec_push_node(ctx->arena, root, str8_lit("sections"));
  MD_Node *indexed_section_list = t_codec_push_node(ctx->arena, root, str8_lit("sections_by_index"));
  for
    EachIndex(i, pe.section_count)
    {
      COFF_SectionHeader *header = &section_table[i];
      String8 section_name = {0};
      if (!t_codec_coff_section_name(string_table, header, &section_name)) {
        return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("pe"), "PE section %llu has an invalid name", i + 1);
      }
      MD_Node *section = t_codec_push_node(ctx->arena, section_list, section_name);
      MD_Node *indexed_section = t_codec_push_node(ctx->arena, indexed_section_list, str8f(ctx->arena, "section_%llu", i + 1));
      t_codec_push_field(ctx, indexed_section, "name", section_name);
      t_codec_push_u64(ctx, section, "virtual_size", header->vsize);
      t_codec_push_u64(ctx, indexed_section, "virtual_size", header->vsize);
      t_codec_push_u64(ctx, section, "virtual_offset", header->voff);
      t_codec_push_u64(ctx, indexed_section, "virtual_offset", header->voff);
      t_codec_push_u64(ctx, section, "file_size", header->fsize);
      t_codec_push_u64(ctx, indexed_section, "file_size", header->fsize);
      t_codec_push_u64(ctx, section, "file_offset", header->foff);
      t_codec_push_u64(ctx, indexed_section, "file_offset", header->foff);
      t_codec_push_u64(ctx, section, "alignment", coff_align_size_from_section_flags(header->flags));
      t_codec_push_u64(ctx, indexed_section, "alignment", coff_align_size_from_section_flags(header->flags));
      t_codec_push_u64(ctx, section, "raw_flags", header->flags);
      t_codec_push_u64(ctx, indexed_section, "raw_flags", header->flags);
      if (header->fsize != 0 && (header->foff > data.size || header->fsize > data.size - header->foff)) {
        return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("pe"), "PE section %llu has an invalid raw data range", i + 1);
      }
      U64 section_data_size = Min(header->vsize, header->fsize);
      if (section_data_size != 0 && (header->foff > data.size || section_data_size > data.size - header->foff)) {
        return t_context_errorf(ctx, T_ResultCode_ValidationError, artifact->definition, str8_lit("pe"), "PE section %llu has an invalid data range", i + 1);
      }
      if (section_data_size != 0) {
        String8 section_data = str8_substr(data, rng_1u64(header->foff, header->foff + section_data_size));
        t_codec_push_field(ctx, section, "data", t_hex_from_data(ctx->arena, section_data));
        t_codec_push_field(ctx, indexed_section, "data", t_hex_from_data(ctx->arena, section_data));
      }
    }

  local_persist char *directory_names[] = {"exports",     "imports",      "resources", "exceptions", "certificates", "base_relocations",
                                            "debug",       "architecture", "global_ptr", "tls",        "load_config",  "bound_imports",
                                            "import_address", "delay_imports", "com_descriptor", "reserved"};
  MD_Node *directories = t_codec_push_node(ctx->arena, root, str8_lit("data_directories"));
  U64 directory_count = Min(pe.data_dir_count, ArrayCount(directory_names));
  for EachIndex(i, directory_count)
  {
    MD_Node *directory = t_codec_push_node(ctx->arena, directories, str8_cstring(directory_names[i]));
    t_codec_push_u64(ctx, directory, "virtual_offset", pe.data_dir_vranges[i].min);
    t_codec_push_u64(ctx, directory, "virtual_size", dim_1u64(pe.data_dir_vranges[i]));
    t_codec_push_u64(ctx, directory, "file_offset", pe.data_dir_franges[i].min);
    t_codec_push_u64(ctx, directory, "file_size", dim_1u64(pe.data_dir_franges[i]));
  }

  U16 *optional_magic = str8_deserial_get_raw_ptr(data, pe.optional_header_off, sizeof(*optional_magic));
  U64 optional_header_size = pe.section_table_range.min >= pe.optional_header_off ? pe.section_table_range.min - pe.optional_header_off : 0;
  if (optional_magic == 0 || (*optional_magic != PE_PE32_MAGIC && *optional_magic != PE_PE32PLUS_MAGIC)) {
    return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_zero(), "PE optional header has invalid magic");
  }
  if (*optional_magic == PE_PE32PLUS_MAGIC) {
    if (optional_header_size < sizeof(PE_OptionalHeader32Plus)) {
      return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_zero(), "declared PE optional header is truncated");
    }
    PE_OptionalHeader32Plus *opt = str8_deserial_get_raw_ptr(data, pe.optional_header_off, sizeof(*opt));
    if (opt == 0) { return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_zero(), "PE optional header is truncated"); }
    MD_Node *optional = t_codec_push_node(ctx->arena, root, str8_lit("optional"));
    t_codec_push_u64(ctx, optional, "sizeof_code", opt->sizeof_code);
    t_codec_push_u64(ctx, optional, "sizeof_initialized_data", opt->sizeof_inited_data);
    t_codec_push_u64(ctx, optional, "sizeof_uninitialized_data", opt->sizeof_uninited_data);
    t_codec_push_u64(ctx, optional, "code_base", opt->code_base);
    t_codec_push_u64(ctx, optional, "image_base", opt->image_base);
    t_codec_push_u64(ctx, optional, "major_os_version", opt->major_os_ver);
    t_codec_push_u64(ctx, optional, "minor_os_version", opt->minor_os_ver);
    t_codec_push_u64(ctx, optional, "major_image_version", opt->major_img_ver);
    t_codec_push_u64(ctx, optional, "minor_image_version", opt->minor_img_ver);
    t_codec_push_u64(ctx, optional, "major_subsystem_version", opt->major_subsystem_ver);
    t_codec_push_u64(ctx, optional, "minor_subsystem_version", opt->minor_subsystem_ver);
    t_codec_push_u64(ctx, optional, "win32_version", opt->win32_version_value);
    t_codec_push_u64(ctx, optional, "sizeof_image", opt->sizeof_image);
    t_codec_push_u64(ctx, optional, "sizeof_headers", opt->sizeof_headers);
    t_codec_push_u64(ctx, optional, "dll_characteristics", opt->dll_characteristics);
    t_codec_push_u64(ctx, optional, "sizeof_stack_reserve", opt->sizeof_stack_reserve);
    t_codec_push_u64(ctx, optional, "sizeof_stack_commit", opt->sizeof_stack_commit);
    t_codec_push_u64(ctx, optional, "sizeof_heap_reserve", opt->sizeof_heap_reserve);
    t_codec_push_u64(ctx, optional, "sizeof_heap_commit", opt->sizeof_heap_commit);
    t_codec_push_u64(ctx, optional, "loader_flags", opt->loader_flags);
  } else {
    if (optional_header_size < sizeof(PE_OptionalHeader32)) {
      return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_zero(), "declared PE optional header is truncated");
    }
    PE_OptionalHeader32 *opt = str8_deserial_get_raw_ptr(data, pe.optional_header_off, sizeof(*opt));
    if (opt == 0) { return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_zero(), "PE optional header is truncated"); }
    MD_Node *optional = t_codec_push_node(ctx->arena, root, str8_lit("optional"));
    t_codec_push_u64(ctx, optional, "sizeof_code", opt->sizeof_code);
    t_codec_push_u64(ctx, optional, "sizeof_initialized_data", opt->sizeof_inited_data);
    t_codec_push_u64(ctx, optional, "sizeof_uninitialized_data", opt->sizeof_uninited_data);
    t_codec_push_u64(ctx, optional, "code_base", opt->code_base);
    t_codec_push_u64(ctx, optional, "data_base", opt->data_base);
    t_codec_push_u64(ctx, optional, "image_base", opt->image_base);
    t_codec_push_u64(ctx, optional, "major_os_version", opt->major_os_ver);
    t_codec_push_u64(ctx, optional, "minor_os_version", opt->minor_os_ver);
    t_codec_push_u64(ctx, optional, "major_image_version", opt->major_img_ver);
    t_codec_push_u64(ctx, optional, "minor_image_version", opt->minor_img_ver);
    t_codec_push_u64(ctx, optional, "major_subsystem_version", opt->major_subsystem_ver);
    t_codec_push_u64(ctx, optional, "minor_subsystem_version", opt->minor_subsystem_ver);
    t_codec_push_u64(ctx, optional, "win32_version", opt->win32_version_value);
    t_codec_push_u64(ctx, optional, "sizeof_image", opt->sizeof_image);
    t_codec_push_u64(ctx, optional, "sizeof_headers", opt->sizeof_headers);
    t_codec_push_u64(ctx, optional, "dll_characteristics", opt->dll_characteristics);
    t_codec_push_u64(ctx, optional, "sizeof_stack_reserve", opt->sizeof_stack_reserve);
    t_codec_push_u64(ctx, optional, "sizeof_stack_commit", opt->sizeof_stack_commit);
    t_codec_push_u64(ctx, optional, "sizeof_heap_reserve", opt->sizeof_heap_reserve);
    t_codec_push_u64(ctx, optional, "sizeof_heap_commit", opt->sizeof_heap_commit);
    t_codec_push_u64(ctx, optional, "loader_flags", opt->loader_flags);
  }

  if (pe.data_dir_count > PE_DataDirectoryIndex_EXPORT && dim_1u64(pe.data_dir_vranges[PE_DataDirectoryIndex_EXPORT]) != 0) {
    PE_ParsedExportTable table = pe_exports_from_data(ctx->arena, pe.section_count, section_table, data, pe.data_dir_franges[PE_DataDirectoryIndex_EXPORT],
                                                      pe.data_dir_vranges[PE_DataDirectoryIndex_EXPORT]);
    MD_Node *exports = t_codec_push_node(ctx->arena, root, str8_lit("exports"));
    t_codec_push_u64(ctx, exports, "flags", table.flags);
    t_codec_push_u64(ctx, exports, "timestamp", table.time_stamp);
    t_codec_push_u64(ctx, exports, "major_version", table.major_ver);
    t_codec_push_u64(ctx, exports, "minor_version", table.minor_ver);
    t_codec_push_u64(ctx, exports, "ordinal_base", table.ordinal_base);
    t_codec_push_u64(ctx, exports, "count", table.export_count);
    MD_Node *entries = t_codec_push_node(ctx->arena, exports, str8_lit("entries"));
    for EachIndex(i, table.export_count)
    {
      PE_ParsedExport *entry = &table.exports[i];
      MD_Node *entry_node = t_codec_push_node(ctx->arena, entries, str8f(ctx->arena, "export_%llu", i));
      t_codec_push_field(ctx, entry_node, "name", entry->name);
      t_codec_push_field(ctx, entry_node, "forwarder", entry->forwarder);
      t_codec_push_u64(ctx, entry_node, "virtual_offset", entry->voff);
      t_codec_push_u64(ctx, entry_node, "ordinal", entry->ordinal);
    }
  }

  if (pe.data_dir_count > PE_DataDirectoryIndex_IMPORT && dim_1u64(pe.data_dir_vranges[PE_DataDirectoryIndex_IMPORT]) != 0) {
    PE_ParsedStaticImportTable table = pe_static_imports_from_data(ctx->arena, pe.is_pe32, pe.section_count, section_table, data, pe.data_dir_franges[PE_DataDirectoryIndex_IMPORT]);
    MD_Node *imports = t_codec_push_node(ctx->arena, root, str8_lit("imports"));
    t_codec_push_u64(ctx, imports, "count", table.count);
    for EachIndex(dll_idx, table.count)
    {
      PE_ParsedStaticDLLImport *dll = &table.v[dll_idx];
      MD_Node *dll_node = t_codec_push_node(ctx->arena, imports, str8f(ctx->arena, "dll_%llu", dll_idx));
      t_codec_push_field(ctx, dll_node, "name", dll->name);
      t_codec_push_u64(ctx, dll_node, "import_address_table", dll->import_address_table_voff);
      t_codec_push_u64(ctx, dll_node, "import_name_table", dll->import_name_table_voff);
      t_codec_push_u64(ctx, dll_node, "timestamp", dll->time_stamp);
      t_codec_push_u64(ctx, dll_node, "forwarder_chain", dll->forwarder_chain);
      t_codec_push_u64(ctx, dll_node, "count", dll->import_count);
      MD_Node *entries = t_codec_push_node(ctx->arena, dll_node, str8_lit("entries"));
      for EachIndex(import_idx, dll->import_count)
      {
        PE_ParsedImport *import = &dll->imports[import_idx];
        MD_Node *import_node = t_codec_push_node(ctx->arena, entries, str8f(ctx->arena, "import_%llu", import_idx));
        if (import->type == PE_ParsedImport_Name) {
          t_codec_push_field(ctx, import_node, "type", str8_lit("name"));
          t_codec_push_field(ctx, import_node, "name", import->u.name.string);
          t_codec_push_u64(ctx, import_node, "hint", import->u.name.hint);
        } else if (import->type == PE_ParsedImport_Ordinal) {
          t_codec_push_field(ctx, import_node, "type", str8_lit("ordinal"));
          t_codec_push_u64(ctx, import_node, "ordinal", import->u.ordinal);
        }
      }
    }
  }

  if (pe.data_dir_count > PE_DataDirectoryIndex_DELAY_IMPORT && dim_1u64(pe.data_dir_vranges[PE_DataDirectoryIndex_DELAY_IMPORT]) != 0) {
    PE_ParsedDelayImportTable table = pe_delay_imports_from_data(ctx->arena, pe.is_pe32, pe.section_count, section_table, data, pe.data_dir_franges[PE_DataDirectoryIndex_DELAY_IMPORT]);
    MD_Node *imports = t_codec_push_node(ctx->arena, root, str8_lit("delay_imports"));
    t_codec_push_u64(ctx, imports, "count", table.count);
    for EachIndex(dll_idx, table.count)
    {
      PE_ParsedDelayDLLImport *dll = &table.v[dll_idx];
      MD_Node *dll_node = t_codec_push_node(ctx->arena, imports, str8f(ctx->arena, "dll_%llu", dll_idx));
      t_codec_push_field(ctx, dll_node, "name", dll->name);
      t_codec_push_u64(ctx, dll_node, "attributes", dll->attributes);
      t_codec_push_u64(ctx, dll_node, "module_handle", dll->module_handle_voff);
      t_codec_push_u64(ctx, dll_node, "import_address_table", dll->iat_voff);
      t_codec_push_u64(ctx, dll_node, "import_name_table", dll->name_table_voff);
      t_codec_push_u64(ctx, dll_node, "bound_table", dll->bound_table_voff);
      t_codec_push_u64(ctx, dll_node, "unload_table", dll->unload_table_voff);
      t_codec_push_u64(ctx, dll_node, "timestamp", dll->time_stamp);
      t_codec_push_u64(ctx, dll_node, "bound_count", dll->bound_table_count);
      t_codec_push_u64(ctx, dll_node, "unload_count", dll->unload_table_count);
      t_codec_push_u64(ctx, dll_node, "count", dll->import_count);
      MD_Node *entries = t_codec_push_node(ctx->arena, dll_node, str8_lit("entries"));
      for EachIndex(import_idx, dll->import_count)
      {
        PE_ParsedImport *import = &dll->imports[import_idx];
        MD_Node *import_node = t_codec_push_node(ctx->arena, entries, str8f(ctx->arena, "import_%llu", import_idx));
        if (import->type == PE_ParsedImport_Name) {
          t_codec_push_field(ctx, import_node, "type", str8_lit("name"));
          t_codec_push_field(ctx, import_node, "name", import->u.name.string);
          t_codec_push_u64(ctx, import_node, "hint", import->u.name.hint);
        } else if (import->type == PE_ParsedImport_Ordinal) {
          t_codec_push_field(ctx, import_node, "type", str8_lit("ordinal"));
          t_codec_push_u64(ctx, import_node, "ordinal", import->u.ordinal);
        }
      }
    }
  }

  *semantic_tree_out = root;
  return ctx->result;
}

typedef struct T_PdbMsfInfo T_PdbMsfInfo;
struct T_PdbMsfInfo
{
  U64 stream_count;
  B32 *stream_present;
};

typedef struct T_PdbKindCount T_PdbKindCount;
struct T_PdbKindCount
{
  T_PdbKindCount *next;
  U16 kind;
  U64 count;
};

internal B32
t_codec_pdb_range_is_valid(U64 off, U64 size, U64 cap)
{
  return off <= cap && size <= cap - off;
}

internal B32
t_codec_pdb_cstr_from_offset(String8 data, U64 off, String8 *string_out)
{
  if (off >= data.size) { return 0; }
  String8 string = str8_cstring_capped(data.str + off, data.str + data.size);
  if (string.size >= data.size - off) { return 0; }
  *string_out = string;
  return 1;
}

internal String8
t_codec_pdb_sym_kind_name(Arena *arena, CV_SymKind kind)
{
  String8 name = cv_string_from_symbol_kind(arena, kind);
  if (name.size == 2) { name = str8f(arena, "S_0x%04x", kind); }
  return name;
}

internal String8
t_codec_pdb_leaf_kind_name(Arena *arena, CV_LeafKind kind)
{
  String8 name = cv_string_from_leaf_name(arena, kind);
  if (name.size == 3) { name = str8f(arena, "LF_0x%04x", kind); }
  return name;
}

internal B32
t_codec_pdb_validate_msf(Arena *arena, String8 data, T_PdbMsfInfo *info, String8 *error)
{
  if (!msf_check_magic_70(data) || data.size < sizeof(MSF_Header70)) {
    *error = str8_lit("file is not an MSF 7.0 PDB");
    return 0;
  }
  MSF_Header70 *header = (MSF_Header70 *)data.str;
  U64 page_size = header->page_size;
  U64 page_count = header->page_count;
  if (page_size < MSF_MIN_PAGE_SIZE || page_size > MSF_MAX_PAGE_SIZE || !IsPow2OrZero(page_size)) {
    *error = str8_lit("MSF has an invalid page size");
    return 0;
  }
  if (page_count == 0) {
    *error = str8_lit("MSF has an invalid page count");
    return 0;
  }
  U64 directory_size = header->stream_table_size;
  U64 directory_page_count = CeilIntegerDiv(directory_size, page_size);
  U64 directory_map_size = directory_page_count * sizeof(U32);
  if (directory_size < sizeof(U32) || directory_size > data.size || directory_map_size > page_size || header->root_pn >= page_count) {
    *error = str8_lit("MSF stream directory map is invalid");
    return 0;
  }
  U64 map_off = (U64)header->root_pn * page_size;
  if (!t_codec_pdb_range_is_valid(map_off, directory_map_size, data.size)) {
    *error = str8_lit("MSF stream directory map is truncated");
    return 0;
  }
  U8 *directory = push_array_no_zero(arena, U8, directory_size);
  for EachIndex(i, directory_page_count)
  {
    U32 pn = 0;
    MemoryCopy(&pn, data.str + map_off + i * sizeof(pn), sizeof(pn));
    U64 copy_size = Min(page_size, directory_size - i * page_size);
    if (pn >= page_count || !t_codec_pdb_range_is_valid((U64)pn * page_size, copy_size, data.size)) {
      *error = str8f(arena, "MSF stream directory page %llu is invalid", i);
      return 0;
    }
    MemoryCopy(directory + i * page_size, data.str + (U64)pn * page_size, copy_size);
  }
  U32 stream_count = 0;
  MemoryCopy(&stream_count, directory, sizeof(stream_count));
  if (stream_count > (directory_size - sizeof(U32)) / sizeof(U32)) {
    *error = str8_lit("MSF stream size table is truncated");
    return 0;
  }
  U64 index_cursor = sizeof(U32) + (U64)stream_count * sizeof(U32);
  B32 *present = push_array(arena, B32, stream_count);
  for EachIndex(i, stream_count)
  {
    U32 stream_size = 0;
    MemoryCopy(&stream_size, directory + sizeof(U32) + i * sizeof(U32), sizeof(stream_size));
    if (stream_size == MSF_DELETED_STREAM_STAMP) { continue; }
    present[i] = 1;
    U64 stream_page_count = CeilIntegerDiv((U64)stream_size, page_size);
    U64 indices_size = stream_page_count * sizeof(U32);
    if (!t_codec_pdb_range_is_valid(index_cursor, indices_size, directory_size)) {
      *error = str8f(arena, "MSF page list for stream %llu is truncated", i);
      return 0;
    }
    for EachIndex(page_idx, stream_page_count)
    {
      U32 pn = 0;
      MemoryCopy(&pn, directory + index_cursor + page_idx * sizeof(pn), sizeof(pn));
      U64 page_data_size = Min(page_size, (U64)stream_size - page_idx * page_size);
      if (pn >= page_count || !t_codec_pdb_range_is_valid((U64)pn * page_size, page_data_size, data.size)) {
        *error = str8f(arena, "MSF stream %llu references invalid page %u", i, pn);
        return 0;
      }
    }
    index_cursor += indices_size;
  }
  info->stream_count = stream_count;
  info->stream_present = present;
  return 1;
}

internal B32
t_codec_pdb_symbol_name(CV_Symbol symbol, String8 *name_out)
{
  U64 off = symbol.data.size;
  switch (symbol.kind)
  {
    case CV_SymKind_OBJNAME: off = sizeof(CV_SymObjName); break;
    case CV_SymKind_CONSTANT:
    {
      off = sizeof(CV_SymConstant);
      if (off > symbol.data.size) { return 0; }
      CV_NumericParsed numeric = {0};
      U64 numeric_size = cv_read_numeric(symbol.data, off, &numeric);
      if (numeric_size == 0) { return 0; }
      off += numeric_size;
    } break;
    case CV_SymKind_UDT: off = sizeof(CV_SymUDT); break;
    case CV_SymKind_LDATA32: case CV_SymKind_GDATA32:
    case CV_SymKind_LTHREAD32: case CV_SymKind_GTHREAD32: off = sizeof(CV_SymData32); break;
    case CV_SymKind_PUB32: off = sizeof(CV_SymPub32); break;
    case CV_SymKind_LPROC32: case CV_SymKind_GPROC32:
    case CV_SymKind_LPROC32_ID: case CV_SymKind_GPROC32_ID: off = sizeof(CV_SymProc32); break;
    case CV_SymKind_PROCREF: case CV_SymKind_LPROCREF: case CV_SymKind_DATAREF: off = sizeof(CV_SymRef2); break;
    default: return 0;
  }
  return t_codec_pdb_cstr_from_offset(symbol.data, off, name_out);
}

internal B32
t_codec_pdb_read_symbol(String8 data, U64 off, U64 align, CV_Symbol *symbol_out, U64 *read_size_out, String8 *error)
{
  if (!t_codec_pdb_range_is_valid(off, sizeof(CV_SymbolHeader), data.size)) {
    *error = str8_lit("CodeView symbol header is truncated");
    return 0;
  }
  CV_SymbolHeader header = {0};
  MemoryCopy(&header, data.str + off, sizeof(header));
  U64 raw_size = sizeof(CV_SymSize) + header.size;
  if (header.size < sizeof(CV_SymKind) || !t_codec_pdb_range_is_valid(off, raw_size, data.size)) {
    *error = str8_lit("CodeView symbol record is truncated");
    return 0;
  }
  U64 read_size = AlignPow2(raw_size, align);
  if (read_size < raw_size || !t_codec_pdb_range_is_valid(off, read_size, data.size)) {
    *error = str8_lit("CodeView symbol padding is truncated");
    return 0;
  }
  String8 record_data = str8(data.str + off + sizeof(CV_SymbolHeader), header.size - sizeof(CV_SymKind));
  U64 fixed_size = cv_header_struct_size_from_sym_kind(header.kind);
  if (CV_IsProc32(header.kind)) { fixed_size = sizeof(CV_SymProc32); }
  if (fixed_size > record_data.size) {
    *error = str8_lit("CodeView symbol fixed fields are truncated");
    return 0;
  }
  CV_Symbol symbol = {0};
  cv_read_symbol(str8_skip(data, off), 0, align, &symbol);
  String8 ignored = {0};
  if ((header.kind == CV_SymKind_CONSTANT || header.kind == CV_SymKind_UDT || header.kind == CV_SymKind_PUB32 || header.kind == CV_SymKind_PROCREF ||
       header.kind == CV_SymKind_LPROCREF || header.kind == CV_SymKind_DATAREF || CV_IsProc32(header.kind) || header.kind == CV_SymKind_LDATA32 ||
       header.kind == CV_SymKind_GDATA32) && !t_codec_pdb_symbol_name(symbol, &ignored)) {
    *error = str8_lit("CodeView symbol name is truncated");
    return 0;
  }
  *symbol_out = symbol;
  *read_size_out = read_size;
  return 1;
}

internal MD_Node *
t_codec_pdb_push_symbol(T_Context *ctx, MD_Node *parent, String8 key, CV_Symbol symbol, U64 off)
{
  MD_Node *node = t_codec_push_node(ctx->arena, parent, key);
  t_codec_push_field(ctx, node, "kind", t_codec_pdb_sym_kind_name(ctx->arena, symbol.kind));
  t_codec_push_u64(ctx, node, "kind_value", symbol.kind);
  t_codec_push_u64(ctx, node, "offset", off);
  t_codec_push_u64(ctx, node, "data_size", symbol.data.size);
  String8 name = {0};
  if (t_codec_pdb_symbol_name(symbol, &name)) { t_codec_push_field(ctx, node, "name", name); }
  if (symbol.kind == CV_SymKind_PROCREF || symbol.kind == CV_SymKind_LPROCREF || symbol.kind == CV_SymKind_DATAREF) {
    CV_SymRef2 value = {0}; MemoryCopy(&value, symbol.data.str, sizeof(value));
    t_codec_push_u64(ctx, node, "suc_name", value.suc_name);
    t_codec_push_u64(ctx, node, "sym_off", value.sym_off);
    t_codec_push_u64(ctx, node, "imod", value.imod);
  } else if (symbol.kind == CV_SymKind_PUB32) {
    CV_SymPub32 value = {0}; MemoryCopy(&value, symbol.data.str, sizeof(value));
    t_codec_push_u64(ctx, node, "flags", value.flags);
    t_codec_push_u64(ctx, node, "section", value.sec);
    t_codec_push_u64(ctx, node, "section_offset", value.off);
  } else if (symbol.kind == CV_SymKind_LDATA32 || symbol.kind == CV_SymKind_GDATA32) {
    CV_SymData32 value = {0}; MemoryCopy(&value, symbol.data.str, sizeof(value));
    t_codec_push_u64(ctx, node, "type_index", value.itype);
    t_codec_push_u64(ctx, node, "section", value.sec);
    t_codec_push_u64(ctx, node, "section_offset", value.off);
  } else if (CV_IsProc32(symbol.kind)) {
    CV_SymProc32 value = {0}; MemoryCopy(&value, symbol.data.str, sizeof(value));
    t_codec_push_u64(ctx, node, "type_index", value.itype);
    t_codec_push_u64(ctx, node, "section", value.sec);
    t_codec_push_u64(ctx, node, "section_offset", value.off);
    t_codec_push_u64(ctx, node, "length", value.len);
  }
  return node;
}

internal B32
t_codec_pdb_push_symbol_stream(T_Context *ctx, MD_Node *parent, String8 data, U64 align, String8 *error)
{
  MD_Node *symbols = t_codec_push_node(ctx->arena, parent, str8_lit("symbols"));
  MD_Node *by_name = t_codec_push_node(ctx->arena, parent, str8_lit("by_name"));
  T_PdbKindCount *first_count = 0;
  U64 count = 0;
  U64 proc_stub_count = 0;
  U64 public_or_proc_ref_count = 0;
  for (U64 cursor = 0; cursor < data.size; count += 1)
  {
    CV_Symbol symbol = {0}; U64 read_size = 0;
    if (!t_codec_pdb_read_symbol(data, cursor, align, &symbol, &read_size, error)) { return 0; }
    MD_Node *symbol_node = t_codec_pdb_push_symbol(ctx, symbols, str8f(ctx->arena, "symbol_%llu", count), symbol, cursor);
    String8 name = {0};
    if (t_codec_pdb_symbol_name(symbol, &name) && md_node_is_nil(md_child_from_string(by_name, name, 0))) {
      MD_Node *named = t_codec_push_node(ctx->arena, by_name, name);
      for MD_EachNode(field, symbol_node->first) { MD_Node *copy = t_codec_push_node(ctx->arena, named, field->string); t_codec_push_node(ctx->arena, copy, field->first->string); }
    }
    T_PdbKindCount *kind_count = first_count;
    for (; kind_count != 0 && kind_count->kind != symbol.kind; kind_count = kind_count->next) {}
    if (kind_count == 0) { kind_count = push_array(ctx->arena, T_PdbKindCount, 1); kind_count->kind = symbol.kind; kind_count->next = first_count; first_count = kind_count; }
    kind_count->count += 1;
    proc_stub_count += symbol.kind == CV_SymKind_LPROC32 || symbol.kind == CV_SymKind_END;
    public_or_proc_ref_count += symbol.kind == CV_SymKind_PUB32 || symbol.kind == CV_SymKind_LPROCREF;
    cursor += read_size;
  }
  t_codec_push_u64(ctx, parent, "size", data.size);
  t_codec_push_u64(ctx, parent, "count", count);
  t_codec_push_u64(ctx, parent, "proc_stub_symbol_count", proc_stub_count);
  t_codec_push_u64(ctx, parent, "non_proc_stub_symbol_count", count - proc_stub_count);
  t_codec_push_u64(ctx, parent, "public_or_proc_ref_symbol_count", public_or_proc_ref_count);
  t_codec_push_u64(ctx, parent, "non_public_or_proc_ref_symbol_count", count - public_or_proc_ref_count);
  MD_Node *kind_counts = t_codec_push_node(ctx->arena, parent, str8_lit("kind_counts"));
  MD_Node *kind_set = t_codec_push_node(ctx->arena, parent, str8_lit("kind_set"));
  for (T_PdbKindCount *item = first_count; item != 0; item = item->next) {
    String8 name = t_codec_pdb_sym_kind_name(ctx->arena, item->kind);
    t_codec_push_u64_s8(ctx, kind_counts, name, item->count);
    t_codec_push_node(ctx->arena, kind_set, name);
  }
  return 1;
}

internal B32
t_codec_pdb_push_tpi(T_Context *ctx, MD_Node *root, char *name, MSF_Parsed *msf, MSF_StreamNumber sn, String8 *error)
{
  MD_Node *node = t_codec_push_node(ctx->arena, root, str8_cstring(name));
  String8 data = msf_data_from_stream(msf, sn);
  t_codec_push_u64(ctx, node, "stream_index", sn);
  t_codec_push_u64(ctx, node, "size", data.size);
  if (data.size < sizeof(PDB_TpiHeader)) { *error = str8f(ctx->arena, "%s stream header is truncated", name); return 0; }
  PDB_TpiHeader *header = (PDB_TpiHeader *)data.str;
  if (header->version != PDB_TpiVersion_IMPV80 || header->header_size < sizeof(*header) || header->header_size > data.size ||
      header->leaf_data_size > data.size - header->header_size || header->ti_hi < header->ti_lo) {
    *error = str8f(ctx->arena, "%s stream header is invalid", name); return 0;
  }
  U64 expected_count = (U64)header->ti_hi - header->ti_lo;
  String8 leaf_data = str8(data.str + header->header_size, header->leaf_data_size);
  U64 parsed_count = 0;
  for (U64 cursor = 0; cursor < leaf_data.size; parsed_count += 1) {
    CV_Leaf leaf = {0}; U64 read_size = cv_read_leaf(str8_skip(leaf_data, cursor), 0, PDB_LEAF_ALIGN, &leaf);
    if (read_size == 0 || cv_header_struct_size_from_leaf_kind(leaf.kind) > leaf.data.size) {
      *error = str8f(ctx->arena, "%s leaf %llu is malformed", name, parsed_count); return 0;
    }
    cursor += read_size;
  }
  if (parsed_count != expected_count) { *error = str8f(ctx->arena, "%s type index range does not match its leaf count", name); return 0; }
  PDB_TpiParsed *tpi = pdb_tpi_from_data(ctx->arena, data);
  CV_DebugT debug_t = cv_debug_t_from_data(ctx->arena, pdb_leaf_data_from_tpi(tpi), PDB_LEAF_ALIGN);
  if (debug_t.count != parsed_count) { *error = str8f(ctx->arena, "%s CodeView leaf parse is inconsistent", name); return 0; }
  t_codec_push_u64(ctx, node, "header_size", header->header_size);
  t_codec_push_u64(ctx, node, "index_first", tpi->itype_first);
  t_codec_push_u64(ctx, node, "index_opl", tpi->itype_opl);
  t_codec_push_u64(ctx, node, "leaf_count", parsed_count);
  t_codec_push_field(ctx, node, "header_only", parsed_count == 0 && data.size == header->header_size ? str8_lit("true") : str8_lit("false"));
  MD_Node *leaves = t_codec_push_node(ctx->arena, node, str8_lit("leaves"));
  for EachIndex(i, debug_t.count)
  {
    CV_Leaf leaf = cv_debug_t_get_leaf(&debug_t, i);
    MD_Node *leaf_node = t_codec_push_node(ctx->arena, leaves, str8f(ctx->arena, "leaf_%llu", i));
    t_codec_push_field(ctx, leaf_node, "kind", t_codec_pdb_leaf_kind_name(ctx->arena, leaf.kind));
    t_codec_push_u64(ctx, leaf_node, "kind_value", leaf.kind);
    t_codec_push_u64(ctx, leaf_node, "type_index", tpi->itype_first + i);
    t_codec_push_u64(ctx, leaf_node, "data_size", leaf.data.size);
    if (leaf.kind == CV_LeafKind_POINTER) {
      CV_LeafPointer value = {0}; MemoryCopy(&value, leaf.data.str, sizeof(value));
      t_codec_push_u64(ctx, leaf_node, "type", value.itype); t_codec_push_u64(ctx, leaf_node, "attributes", value.attribs);
    } else if (leaf.kind == CV_LeafKind_PROCEDURE) {
      CV_LeafProcedure value = {0}; MemoryCopy(&value, leaf.data.str, sizeof(value));
      t_codec_push_u64(ctx, leaf_node, "return_type", value.ret_itype); t_codec_push_u64(ctx, leaf_node, "call_kind", value.call_kind);
      t_codec_push_u64(ctx, leaf_node, "attributes", value.attribs); t_codec_push_u64(ctx, leaf_node, "argument_count", value.arg_count);
      t_codec_push_u64(ctx, leaf_node, "argument_list_type", value.arg_itype);
    }
  }
  return 1;
}

internal B32
t_codec_pdb_validate_gsi(T_Context *ctx, String8 data, String8 symbols, String8 *error)
{
  if (data.size < sizeof(PDB_GsiHeader)) { *error = str8_lit("GSI header is truncated"); return 0; }
  PDB_GsiHeader *header = (PDB_GsiHeader *)data.str;
  U64 bitmap_size = CeilIntegerDiv(4097, 32) * sizeof(U32);
  U64 hash_off = sizeof(*header);
  U64 bitmap_off = hash_off + header->hash_record_arr_size;
  U64 total_size = bitmap_off + header->bucket_data_size;
  if (header->signature != PDB_GsiSignature_Basic || header->version != PDB_GsiVersion_V70 || header->hash_record_arr_size % sizeof(PDB_GsiHashRecord) != 0 ||
      header->bucket_data_size < bitmap_size || total_size != data.size) {
    *error = str8_lit("GSI header or ranges are invalid"); return 0;
  }
  U64 hash_count = header->hash_record_arr_size / sizeof(PDB_GsiHashRecord);
  U64 packed_count = (header->bucket_data_size - bitmap_size) / sizeof(U32);
  U64 occupied_count = 0;
  for EachIndex(i, CeilIntegerDiv(4097, 32)) { U32 bits = 0; MemoryCopy(&bits, data.str + bitmap_off + i * sizeof(bits), sizeof(bits)); occupied_count += count_bits_set32(bits); }
  if ((header->bucket_data_size - bitmap_size) % sizeof(U32) != 0 || occupied_count != packed_count) { *error = str8_lit("GSI bucket bitmap and offset array disagree"); return 0; }
  U64 offsets_off = bitmap_off + bitmap_size;
  U32 previous = 0;
  for EachIndex(i, packed_count) {
    U32 value = 0; MemoryCopy(&value, data.str + offsets_off + i * sizeof(value), sizeof(value));
    if ((i == 0 && hash_count != 0 && value != 0) || value % sizeof(PDB_GsiHashRecordOffsetCalc) != 0 ||
        value / sizeof(PDB_GsiHashRecordOffsetCalc) > hash_count || (i != 0 && value < previous)) {
      *error = str8_lit("GSI bucket offset is invalid"); return 0;
    }
    previous = value;
  }
  if (hash_count != 0 && packed_count == 0) { *error = str8_lit("GSI has hash records but no occupied bucket"); return 0; }
  for EachIndex(i, hash_count) {
    PDB_GsiHashRecord record = {0}; MemoryCopy(&record, data.str + hash_off + i * sizeof(record), sizeof(record));
    if (record.symbol_off == 0) { *error = str8_lit("GSI references the null symbol offset"); return 0; }
    CV_Symbol symbol = {0}; U64 read_size = 0;
    if (!t_codec_pdb_read_symbol(symbols, record.symbol_off - 1, 1, &symbol, &read_size, error)) { return 0; }
    String8 name = {0};
    if (!t_codec_pdb_symbol_name(symbol, &name)) { *error = str8_lit("GSI references a symbol without a valid name"); return 0; }
  }
  return 1;
}

internal B32
t_codec_pdb_push_gsi(T_Context *ctx, MD_Node *root, char *name, MSF_Parsed *msf, MSF_StreamNumber sn, String8 symbols, B32 is_psi, String8 *error)
{
  MD_Node *node = t_codec_push_node(ctx->arena, root, str8_cstring(name));
  t_codec_push_u64(ctx, node, "stream_index", sn);
  if (sn >= msf->stream_count) { t_codec_push_field(ctx, node, "present", str8_lit("false")); return 1; }
  String8 stream = msf_data_from_stream(msf, sn);
  t_codec_push_field(ctx, node, "present", str8_lit("true"));
  t_codec_push_u64(ctx, node, "size", stream.size);
  String8 gsi_data = stream;
  if (is_psi) {
    if (stream.size < sizeof(PDB_PsiHeader)) { *error = str8_lit("PSI header is truncated"); return 0; }
    PDB_PsiHeader *header = (PDB_PsiHeader *)stream.str;
    U64 ranges_size = sizeof(*header);
    U64 range_sizes[] = {header->sym_hash_size, header->addr_map_size};
    for EachElement(i, range_sizes) { if (range_sizes[i] > stream.size - Min(ranges_size, stream.size)) { *error = str8_lit("PSI ranges are truncated"); return 0; } ranges_size += range_sizes[i]; }
    if (header->thunk_count != 0 && header->thunk_size > (stream.size - Min(ranges_size, stream.size)) / header->thunk_count) { *error = str8_lit("PSI thunk table is truncated"); return 0; }
    ranges_size += (U64)header->thunk_count * header->thunk_size;
    if ((U64)header->sec_count > (stream.size - Min(ranges_size, stream.size)) / sizeof(U32)) { *error = str8_lit("PSI section table is truncated"); return 0; }
    t_codec_push_u64(ctx, node, "address_map_size", header->addr_map_size);
    t_codec_push_u64(ctx, node, "thunk_count", header->thunk_count);
    t_codec_push_u64(ctx, node, "section_count", header->sec_count);
    gsi_data = str8(stream.str + sizeof(*header), header->sym_hash_size);
  }
  if (!t_codec_pdb_validate_gsi(ctx, gsi_data, symbols, error)) { return 0; }
  PDB_GsiParsed *gsi = pdb_gsi_from_data(ctx->arena, gsi_data);
  MD_Node *indexed = t_codec_push_node(ctx->arena, node, str8_lit("symbols"));
  MD_Node *kind_counts = t_codec_push_node(ctx->arena, node, str8_lit("kind_counts"));
  T_PdbKindCount *first_count = 0;
  U64 count = 0;
  for EachElement(bucket_idx, gsi->buckets) {
    PDB_GsiBucket bucket = gsi->buckets[bucket_idx];
    for EachIndex(i, bucket.count) {
      U64 off = bucket.offs[i]; CV_Symbol symbol = {0}; U64 read_size = 0;
      if (!t_codec_pdb_read_symbol(symbols, off, 1, &symbol, &read_size, error)) { return 0; }
      String8 symbol_name = {0}; t_codec_pdb_symbol_name(symbol, &symbol_name);
      if (pdb_gsi_symbol_from_string(gsi, symbols, symbol_name) >= symbols.size) { *error = str8_lit("GSI symbol is not queryable by name"); return 0; }
      if (md_node_is_nil(md_child_from_string(indexed, symbol_name, 0))) { t_codec_pdb_push_symbol(ctx, indexed, symbol_name, symbol, off); }
      T_PdbKindCount *item = first_count; for (; item != 0 && item->kind != symbol.kind; item = item->next) {}
      if (item == 0) { item = push_array(ctx->arena, T_PdbKindCount, 1); item->kind = symbol.kind; item->next = first_count; first_count = item; }
      item->count += 1; count += 1;
    }
  }
  for (T_PdbKindCount *item = first_count; item != 0; item = item->next) { String8 kind = t_codec_pdb_sym_kind_name(ctx->arena, item->kind); t_codec_push_u64_s8(ctx, kind_counts, kind, item->count); }
  t_codec_push_u64(ctx, node, "indexed_symbol_count", count);
  return 1;
}

internal B32
t_codec_pdb_push_dbi(T_Context *ctx, MD_Node *root, MSF_Parsed *msf, String8 *error)
{
  String8 data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
  MD_Node *node = t_codec_push_node(ctx->arena, root, str8_lit("dbi"));
  t_codec_push_u64(ctx, node, "stream_index", PDB_FixedStream_Dbi);
  t_codec_push_u64(ctx, node, "size", data.size);
  if (data.size < sizeof(PDB_DbiHeader)) { *error = str8_lit("DBI header is truncated"); return 0; }
  PDB_DbiHeader *header = (PDB_DbiHeader *)data.str;
  U64 ranges_size = header->module_info_size;
  U32 sizes[] = {header->sec_con_size, header->sec_map_size, header->file_info_size, header->tsm_size, header->ec_info_size, header->dbg_header_size};
  for EachElement(i, sizes) { if (sizes[i] > data.size - Min(ranges_size, data.size)) { *error = str8_lit("DBI substream ranges are truncated"); return 0; } ranges_size += sizes[i]; }
  if (header->sig != PDB_DbiHeaderSignature_V1 || ranges_size > data.size - sizeof(*header)) { *error = str8_lit("DBI header or substream ranges are invalid"); return 0; }
  PDB_DbiParsed *dbi = pdb_dbi_from_data(ctx->arena, data);
  t_codec_push_field(ctx, node, "machine", coff_string_from_machine_type(dbi->machine_type));
  t_codec_push_u64(ctx, node, "machine_value", dbi->machine_type);
  t_codec_push_u64(ctx, node, "gsi_stream", dbi->gsi_sn); t_codec_push_u64(ctx, node, "psi_stream", dbi->psi_sn); t_codec_push_u64(ctx, node, "symbol_stream", dbi->sym_sn);
  String8 module_info = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
  for (U64 cursor = 0; cursor < module_info.size;) {
    if (!t_codec_pdb_range_is_valid(cursor, sizeof(PDB_DbiCompUnitHeader), module_info.size)) { *error = str8_lit("DBI module header is truncated"); return 0; }
    PDB_DbiCompUnitHeader *mod = (PDB_DbiCompUnitHeader *)(module_info.str + cursor);
    U64 name_off = cursor + sizeof(*mod); String8 first = {0}, second = {0};
    if (!t_codec_pdb_cstr_from_offset(module_info, name_off, &first) || !t_codec_pdb_cstr_from_offset(module_info, name_off + first.size + 1, &second)) { *error = str8_lit("DBI module name is truncated"); return 0; }
    cursor = AlignPow2(name_off + first.size + 1 + second.size + 1, 4);
    U64 ranges = (U64)mod->symbols_size + mod->c11_lines_size + mod->c13_lines_size;
    if ((ranges != 0 && mod->sn >= msf->stream_count) || (mod->sn < msf->stream_count && ranges > msf_data_from_stream(msf, mod->sn).size)) {
      *error = str8_lit("DBI module stream ranges are invalid"); return 0;
    }
  }
  PDB_CompUnitArray *modules = pdb_comp_unit_array_from_data(ctx->arena, module_info);
  t_codec_push_u64(ctx, node, "module_count", modules->count);
  MD_Node *module_nodes = t_codec_push_node(ctx->arena, node, str8_lit("modules"));
  for EachIndex(i, modules->count) {
    PDB_CompUnit *mod = modules->units[i];
    MD_Node *mod_node = t_codec_push_node(ctx->arena, module_nodes, str8f(ctx->arena, "module_%llu", i));
    t_codec_push_field(ctx, mod_node, "object_name", mod->obj_name);
    t_codec_push_field(ctx, mod_node, "object_file_name", str8_skip_last_slash(mod->obj_name));
    t_codec_push_field(ctx, mod_node, "group_name", mod->group_name);
    t_codec_push_u64(ctx, mod_node, "stream_index", mod->sn);
    String8 sym_data = pdb_data_from_unit_range(msf, mod, PDB_DbiCompUnitRange_Symbols);
    String8 c11_data = pdb_data_from_unit_range(msf, mod, PDB_DbiCompUnitRange_C11);
    String8 c13_data = pdb_data_from_unit_range(msf, mod, PDB_DbiCompUnitRange_C13);
    t_codec_push_u64(ctx, mod_node, "symbol_size", sym_data.size); t_codec_push_u64(ctx, mod_node, "c11_size", c11_data.size); t_codec_push_u64(ctx, mod_node, "c13_size", c13_data.size);
    MD_Node *module_symbols = t_codec_push_node(ctx->arena, mod_node, str8_lit("module_symbols"));
    if (!t_codec_pdb_push_symbol_stream(ctx, module_symbols, sym_data, PDB_SYMBOL_ALIGN, error)) { return 0; }
  }
  MD_Node *global_symbols = t_codec_push_node(ctx->arena, root, str8_lit("global_symbols"));
  t_codec_push_u64(ctx, global_symbols, "stream_index", dbi->sym_sn);
  String8 symbols = {0};
  if (dbi->sym_sn < msf->stream_count) {
    t_codec_push_field(ctx, global_symbols, "present", str8_lit("true"));
    symbols = msf_data_from_stream(msf, dbi->sym_sn);
    if (!t_codec_pdb_push_symbol_stream(ctx, global_symbols, symbols, PDB_SYMBOL_ALIGN, error)) { return 0; }
  } else {
    t_codec_push_field(ctx, global_symbols, "present", str8_lit("false"));
  }
  if (!t_codec_pdb_push_gsi(ctx, root, "gsi", msf, dbi->gsi_sn, symbols, 0, error)) { return 0; }
  if (!t_codec_pdb_push_gsi(ctx, root, "psi", msf, dbi->psi_sn, symbols, 1, error)) { return 0; }
  return 1;
}

internal T_Result
t_codec_pdb_decode(T_Context *ctx, String8 data, MD_Node **semantic_tree_out)
{
  T_PdbMsfInfo msf_info = {0}; String8 error = {0};
  if (!t_codec_pdb_validate_msf(ctx->arena, data, &msf_info, &error)) { return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_lit("expect_pdb"), "%S", error); }
  MSF_Parsed *msf = msf_parsed_from_data(ctx->arena, data);
  if (msf == 0 || msf->stream_count != msf_info.stream_count || msf->stream_count <= PDB_FixedStream_Ipi) {
    return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_lit("expect_pdb"), "PDB is missing fixed streams");
  }
  MD_Node *root = t_codec_push_node(ctx->arena, 0, str8_lit("pdb"));
  t_codec_push_u64(ctx, root, "file_size", data.size); t_codec_push_u64(ctx, root, "page_size", msf->page_size); t_codec_push_u64(ctx, root, "page_count", msf->page_count);
  t_codec_push_u64(ctx, root, "stream_count", msf->stream_count);
  MD_Node *streams = t_codec_push_node(ctx->arena, root, str8_lit("streams"));
  for EachIndex(i, msf->stream_count) {
    MD_Node *stream = t_codec_push_node(ctx->arena, streams, str8f(ctx->arena, "stream_%llu", i));
    t_codec_push_field(ctx, stream, "present", msf_info.stream_present[i] ? str8_lit("true") : str8_lit("false"));
    t_codec_push_u64(ctx, stream, "size", msf->streams[i].size);
  }
  MD_Node *fixed = t_codec_push_node(ctx->arena, root, str8_lit("fixed_streams"));
  struct { char *name; MSF_StreamNumber sn; } fixed_defs[] = {{"info", PDB_FixedStream_Info}, {"tpi", PDB_FixedStream_Tpi}, {"dbi", PDB_FixedStream_Dbi}, {"ipi", PDB_FixedStream_Ipi}};
  for EachElement(i, fixed_defs) {
    MD_Node *stream = t_codec_push_node(ctx->arena, fixed, str8_cstring(fixed_defs[i].name));
    t_codec_push_u64(ctx, stream, "index", fixed_defs[i].sn); t_codec_push_field(ctx, stream, "present", msf_info.stream_present[fixed_defs[i].sn] ? str8_lit("true") : str8_lit("false"));
    t_codec_push_u64(ctx, stream, "size", msf->streams[fixed_defs[i].sn].size);
    if (!msf_info.stream_present[fixed_defs[i].sn]) { return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_lit("expect_pdb"), "PDB fixed stream %s is absent", fixed_defs[i].name); }
  }
  if (!t_codec_pdb_push_tpi(ctx, root, "tpi", msf, PDB_FixedStream_Tpi, &error) || !t_codec_pdb_push_tpi(ctx, root, "ipi", msf, PDB_FixedStream_Ipi, &error) ||
      !t_codec_pdb_push_dbi(ctx, root, msf, &error)) {
    return t_context_errorf(ctx, T_ResultCode_ValidationError, &md_nil_node, str8_lit("expect_pdb"), "%S", error);
  }
  *semantic_tree_out = root;
  return ctx->result;
}

internal T_Result
t_codec_run_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"path", "args", "expect_exit", "timeout_ms", "stdout_matches", "stderr_matches", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  t_codec_validate_scalar(ctx, arguments, "path", 1);
  for (U64 i = 1; allowed[i] != 0; i += 1) { t_codec_validate_scalar(ctx, arguments, allowed[i], 0); }
  String8 path = t_codec_scalar(t_codec_child(arguments, "path"));
  if (path.size == 0 || str8_find_needle(path, 0, str8_lit("/"), 0) < path.size || str8_find_needle(path, 0, str8_lit("\\"), 0) < path.size || str8_find_needle(path, 0, str8_lit(":"), 0) < path.size) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "path"), "run path must be a relative file name");
  }
  String8 expected = t_codec_scalar(t_codec_child(arguments, "expect_exit"));
  U64 value = 0;
  if (expected.size != 0 && !str8_matchi(expected, str8_lit("nonzero")) && !str8_matchi(expected, str8_lit("any")) && !try_u64_from_str8_c_rules(expected, &value)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "expect_exit"), "invalid expected process exit code");
  }
  MD_Node *timeout = t_codec_child(arguments, "timeout_ms");
  if (!md_node_is_nil(timeout) && !try_u64_from_str8_c_rules(t_codec_scalar(timeout), &value)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, timeout, "timeout_ms must be an integer");
  }
  return ctx->run->result;
}

internal T_Result
t_codec_run_execute(T_Context *ctx, MD_Node *arguments)
{
  String8 path = t_make_file_path(ctx->arena, t_codec_scalar(t_codec_child(arguments, "path")));
  U64 timeout_ms = max_U64;
  MD_Node *timeout = t_codec_child(arguments, "timeout_ms");
  if (!md_node_is_nil(timeout)) { try_u64_from_str8_c_rules(t_codec_scalar(timeout), &timeout_ms); }
  U64 timeout_us = timeout_ms == max_U64 || timeout_ms > max_U64 / 1000 ? max_U64 : timeout_ms * 1000;
  if (!t_invoke(path, t_codec_scalar(t_codec_child(arguments, "args")), timeout_us)) {
    return t_context_errorf(ctx, T_ResultCode_IoError, arguments, str8_lit("run"), "unable to launch '%S'", path);
  }
  if (g_last_exit_code == max_U64) { return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("run"), "process did not exit before timeout"); }
  String8 expected = t_codec_scalar(t_codec_child(arguments, "expect_exit"));
  B32 any = str8_matchi(expected, str8_lit("any"));
  B32 nonzero = str8_matchi(expected, str8_lit("nonzero"));
  U64 expected_code = 0;
  if (expected.size != 0 && !any && !nonzero) { try_u64_from_str8_c_rules(expected, &expected_code); }
  if (!any && (nonzero ? g_last_exit_code == 0 : g_last_exit_code != expected_code)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("run"), "process exited with %llu, expected %S\n%S", g_last_exit_code,
                            nonzero ? str8_lit("nonzero") : str8f(ctx->arena, "%llu", expected_code), g_errors);
  }
  StringMatchFlags flags = StringMatchFlag_CaseInsensitive | StringMatchFlag_SlashInsensitive;
  String8 stdout_pattern = t_codec_scalar(t_codec_child(arguments, "stdout_matches"));
  String8 stderr_pattern = t_codec_scalar(t_codec_child(arguments, "stderr_matches"));
  if (stdout_pattern.size != 0 && !str8_match_wildcard(g_output, stdout_pattern, flags)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("run"), "process stdout does not match '%S'\n%S", stdout_pattern, g_output);
  }
  if (stderr_pattern.size != 0 && !str8_match_wildcard(g_errors, stderr_pattern, flags)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("run"), "process stderr does not match '%S'\n%S", stderr_pattern, g_errors);
  }
  return ctx->result;
}

internal B32
t_codec_is_safe_file_name(String8 name)
{
  return name.size != 0 && str8_find_needle(name, 0, str8_lit("/"), 0) >= name.size && str8_find_needle(name, 0, str8_lit("\\"), 0) >= name.size &&
         str8_find_needle(name, 0, str8_lit(":"), 0) >= name.size;
}

internal T_Result
t_codec_clang_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"input", "output", "args", "expect_exit", "timeout_ms", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  for (U64 i = 0; i < 3; i += 1) { t_codec_validate_scalar(ctx, arguments, allowed[i], 1); }
  t_codec_validate_scalar(ctx, arguments, "expect_exit", 0);
  t_codec_validate_scalar(ctx, arguments, "timeout_ms", 0);
  for (U64 i = 0; i < 2; i += 1) {
    if (!t_codec_is_safe_file_name(t_codec_scalar(t_codec_child(arguments, allowed[i])))) {
      t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, allowed[i]), "clang %s must be a relative file name", allowed[i]);
    }
  }
  U64 value = 0;
  MD_Node *expect_exit = t_codec_child(arguments, "expect_exit");
  if (!md_node_is_nil(expect_exit) && !try_u64_from_str8_c_rules(t_codec_scalar(expect_exit), &value)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, expect_exit, "clang expect_exit must be an integer");
  }
  MD_Node *timeout = t_codec_child(arguments, "timeout_ms");
  if (!md_node_is_nil(timeout) && !try_u64_from_str8_c_rules(t_codec_scalar(timeout), &value)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, timeout, "clang timeout_ms must be an integer");
  }
  return ctx->run->result;
}

internal T_Result
t_codec_clang_execute(T_Context *ctx, MD_Node *arguments)
{
  String8 input = t_make_file_path(ctx->arena, t_codec_scalar(t_codec_child(arguments, "input")));
  String8 output = t_make_file_path(ctx->arena, t_codec_scalar(t_codec_child(arguments, "output")));
  String8 args = str8f(ctx->arena, "%S -o %S %S", input, output, t_codec_scalar(t_codec_child(arguments, "args")));
  U64 timeout_ms = max_U64;
  MD_Node *timeout = t_codec_child(arguments, "timeout_ms");
  if (!md_node_is_nil(timeout)) { try_u64_from_str8_c_rules(t_codec_scalar(timeout), &timeout_ms); }
  U64 timeout_us = timeout_ms == max_U64 || timeout_ms > max_U64 / 1000 ? max_U64 : timeout_ms * 1000;
  if (!t_invoke(t_clang_path(), args, timeout_us)) { return t_context_errorf(ctx, T_ResultCode_IoError, arguments, str8_lit("clang"), "unable to launch Clang"); }
  U64 expected = 0;
  MD_Node *expect_exit = t_codec_child(arguments, "expect_exit");
  if (!md_node_is_nil(expect_exit)) { try_u64_from_str8_c_rules(t_codec_scalar(expect_exit), &expected); }
  if (g_last_exit_code != expected) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("clang"), "Clang exited with %llu, expected %llu\n%S", g_last_exit_code, expected, g_errors);
  }
  String8 data = t_read_file(ctx->arena, t_codec_scalar(t_codec_child(arguments, "output")));
  if (data.size == 0) { return t_context_errorf(ctx, T_ResultCode_IoError, arguments, str8_lit("clang"), "Clang did not produce '%S'", output); }
  return ctx->result;
}

internal T_Result
t_codec_expect_pe_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"artifact", "expected", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  t_codec_validate_scalar(ctx, arguments, "artifact", 1);
  MD_Node *artifact_node = t_codec_child(arguments, "artifact");
  T_Artifact *artifact = t_artifact_from_name(ctx->run, t_codec_scalar(artifact_node));
  if (artifact == 0 || !str8_match(artifact->codec->kind, str8_lit("pe"), StringMatchFlag_CaseInsensitive)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, artifact_node, "expect_pe requires a PE artifact");
  }
  MD_Node *expected = t_codec_child(arguments, "expected");
  if (md_node_is_nil(expected) || md_node_is_nil(expected->first) || !md_node_is_nil(expected->first->next) || !str8_match(expected->first->string, str8_lit("pe"), 0)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, expected, "expect_pe requires one PE expectation tree");
  }
  return ctx->run->result;
}

internal T_Result
t_codec_expect_pe_execute(T_Context *ctx, MD_Node *arguments)
{
  T_Artifact *artifact = t_artifact_from_name(ctx, t_codec_scalar(t_codec_child(arguments, "artifact")));
  MD_Node *actual = 0;
  T_Result result = artifact->codec->decode(ctx, artifact, &actual);
  if (!t_result_is_ok(result)) { return result; }
  MD_Node *expected = t_codec_child(arguments, "expected")->first;
  return t_semantic_match(ctx, expected, actual);
}

internal T_Result
t_codec_expect_pdb_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"path", "expected", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  t_codec_validate_scalar(ctx, arguments, "path", 1);
  String8 path = t_codec_scalar(t_codec_child(arguments, "path"));
  if (path.size == 0 || str8_match(path, str8_lit("."), 0) || str8_match(path, str8_lit(".."), 0) ||
      str8_find_needle(path, 0, str8_lit("/"), 0) < path.size || str8_find_needle(path, 0, str8_lit("\\"), 0) < path.size ||
      str8_find_needle(path, 0, str8_lit(":"), 0) < path.size) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "path"), "expect_pdb path must be a safe relative file name");
  }
  MD_Node *expected = t_codec_child(arguments, "expected");
  if (md_node_is_nil(expected) || md_node_is_nil(expected->first) || !md_node_is_nil(expected->first->next) || !str8_match(expected->first->string, str8_lit("pdb"), 0)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, expected, "expect_pdb requires one PDB expectation tree");
  }
  return ctx->run->result;
}

internal T_Result
t_codec_expect_pdb_execute(T_Context *ctx, MD_Node *arguments)
{
  String8 path = t_codec_scalar(t_codec_child(arguments, "path"));
  String8 data = t_read_file(ctx->arena, path);
  if (data.size == 0 && !file_path_exists(t_make_file_path(ctx->arena, path))) {
    return t_context_errorf(ctx, T_ResultCode_IoError, arguments, str8_lit("expect_pdb"), "file '%S' does not exist", path);
  }
  MD_Node *actual = 0;
  T_Result result = t_codec_pdb_decode(ctx, data, &actual);
  if (!t_result_is_ok(result)) { return result; }
  return t_semantic_match(ctx, t_codec_child(arguments, "expected")->first, actual);
}

internal T_Result
t_codec_expect_coff_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"artifact", "expected", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  t_codec_validate_scalar(ctx, arguments, "artifact", 1);
  MD_Node *artifact_node = t_codec_child(arguments, "artifact");
  T_Artifact *artifact = t_artifact_from_name(ctx->run, t_codec_scalar(artifact_node));
  if (artifact == 0 || !str8_match(artifact->codec->kind, str8_lit("coff"), StringMatchFlag_CaseInsensitive)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, artifact_node, "expect_coff requires a COFF artifact");
  }
  MD_Node *expected = t_codec_child(arguments, "expected");
  if (md_node_is_nil(expected) || md_node_is_nil(expected->first) || !md_node_is_nil(expected->first->next) || !str8_match(expected->first->string, str8_lit("coff"), 0)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, expected, "expect_coff requires one COFF expectation tree");
  }
  return ctx->run->result;
}

internal T_Result
t_codec_expect_coff_execute(T_Context *ctx, MD_Node *arguments)
{
  T_Artifact *artifact = t_artifact_from_name(ctx, t_codec_scalar(t_codec_child(arguments, "artifact")));
  MD_Node *actual = 0;
  T_Result result = artifact->codec->decode(ctx, artifact, &actual);
  if (!t_result_is_ok(result)) { return result; }
  MD_Node *expected = t_codec_child(arguments, "expected")->first;
  return t_semantic_match(ctx, expected, actual);
}

internal T_Result
t_codec_expect_file_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"path", "equals_artifact", "contains", "nonempty", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  t_codec_validate_scalar(ctx, arguments, "path", 1);
  t_codec_validate_scalar(ctx, arguments, "equals_artifact", 0);
  t_codec_validate_scalar(ctx, arguments, "contains", 0);
  t_codec_validate_scalar(ctx, arguments, "nonempty", 0);
  String8 path = t_codec_scalar(t_codec_child(arguments, "path"));
  if (str8_find_needle(path, 0, str8_lit("/"), 0) < path.size || str8_find_needle(path, 0, str8_lit("\\"), 0) < path.size || str8_find_needle(path, 0, str8_lit(":"), 0) < path.size) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "path"), "expect_file path must be a relative file name");
  }
  String8 artifact_name = t_codec_scalar(t_codec_child(arguments, "equals_artifact"));
  if (artifact_name.size != 0 && t_artifact_from_name(ctx->run, artifact_name) == 0) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "equals_artifact"), "unknown artifact '%S'", artifact_name);
  }
  B32 nonempty = 0;
  MD_Node *nonempty_node = t_codec_child(arguments, "nonempty");
  if (!md_node_is_nil(nonempty_node) && !t_bool_from_scalar(nonempty_node, &nonempty)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, nonempty_node, "nonempty must be true or false");
  }
  if (artifact_name.size == 0 && md_node_is_nil(t_codec_child(arguments, "contains")) && md_node_is_nil(nonempty_node)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, arguments, "expect_file requires equals_artifact, contains, or nonempty");
  }
  return ctx->run->result;
}

internal T_Result
t_codec_expect_file_execute(T_Context *ctx, MD_Node *arguments)
{
  String8 path = t_codec_scalar(t_codec_child(arguments, "path"));
  String8 data = t_read_file(ctx->arena, path);
  if (data.size == 0 && !file_path_exists(t_make_file_path(ctx->arena, path))) {
    return t_context_errorf(ctx, T_ResultCode_IoError, arguments, str8_lit("expect_file"), "file '%S' does not exist", path);
  }
  String8 artifact_name = t_codec_scalar(t_codec_child(arguments, "equals_artifact"));
  if (artifact_name.size != 0) {
    T_Artifact *artifact = t_artifact_from_name(ctx, artifact_name);
    if (!str8_match(data, artifact->data, 0)) {
      return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_file"), "file '%S' differs from artifact '%S'", path, artifact_name);
    }
  }
  String8 needle = t_codec_scalar(t_codec_child(arguments, "contains"));
  if (needle.size != 0 && str8_find_needle(data, 0, needle, 0) >= data.size) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_file"), "file '%S' does not contain '%S'", path, needle);
  }
  B32 nonempty = 0;
  MD_Node *nonempty_node = t_codec_child(arguments, "nonempty");
  if (!md_node_is_nil(nonempty_node)) {
    t_bool_from_scalar(nonempty_node, &nonempty);
    if (nonempty && data.size == 0) { return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_file"), "file '%S' is empty", path); }
  }
  return ctx->result;
}

internal COFF_SectionHeader *
t_codec_pe_section_from_name(T_Context *ctx, T_Artifact *artifact, PE_BinInfo *pe_out, String8 name)
{
  PE_BinInfo pe = pe_bin_info_from_data(ctx->arena, artifact->data);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(artifact->data, pe.section_table_range).str;
  String8 string_table = str8_substr(artifact->data, pe.string_table_range);
  if (pe_out != 0) { *pe_out = pe; }
  return coff_section_header_from_name(string_table, section_table, pe.section_count, name);
}

internal T_Result
t_codec_expect_pe_word_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"artifact",       "section",       "offset",       "type",       "nonzero",      "equals",       "target_section",
                     "target_offset",  "target_address", "other_section", "other_offset", "other_type",   "relation",     "modulo",
                     "remainder", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  char *required[] = {"artifact", "section", "offset", "type", 0};
  for (U64 i = 0; required[i] != 0; i += 1) { t_codec_validate_scalar(ctx, arguments, required[i], 1); }
  for (U64 i = 4; allowed[i] != 0; i += 1) { t_codec_validate_scalar(ctx, arguments, allowed[i], 0); }
  T_Artifact *artifact = t_artifact_from_name(ctx->run, t_codec_scalar(t_codec_child(arguments, "artifact")));
  if (artifact == 0 || !str8_match(artifact->codec->kind, str8_lit("pe"), StringMatchFlag_CaseInsensitive)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "artifact"), "expect_pe_word requires a PE artifact");
  }
  String8 type = t_codec_scalar(t_codec_child(arguments, "type"));
  if (!str8_match(type, str8_lit("u32"), StringMatchFlag_CaseInsensitive) && !str8_match(type, str8_lit("u64"), StringMatchFlag_CaseInsensitive) &&
      !str8_match(type, str8_lit("rel32"), StringMatchFlag_CaseInsensitive)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "type"), "word type must be u32, u64, or rel32");
  }
  String8 relation = t_codec_scalar(t_codec_child(arguments, "relation"));
  if (relation.size != 0 && !str8_match(relation, str8_lit("equal"), StringMatchFlag_CaseInsensitive) &&
      !str8_match(relation, str8_lit("not_equal"), StringMatchFlag_CaseInsensitive)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "relation"), "relation must be equal or not_equal");
  }
  return ctx->run->result;
}

internal B32
t_codec_pe_read_word(T_Context *ctx, T_Artifact *artifact, String8 section_name, U64 offset, String8 type, U64 *value_out)
{
  PE_BinInfo pe = {0};
  COFF_SectionHeader *section = t_codec_pe_section_from_name(ctx, artifact, &pe, section_name);
  if (section == 0) { return 0; }
  U64 size = str8_match(type, str8_lit("u64"), StringMatchFlag_CaseInsensitive) ? 8 : 4;
  if (offset > section->fsize || size > section->fsize - offset || section->foff > artifact->data.size || section->fsize > artifact->data.size - section->foff) { return 0; }
  U8 *ptr = artifact->data.str + section->foff + offset;
  if (str8_match(type, str8_lit("u64"), StringMatchFlag_CaseInsensitive)) {
    MemoryCopy(value_out, ptr, sizeof(U64));
  } else {
    U32 raw = 0;
    MemoryCopy(&raw, ptr, sizeof(raw));
    if (str8_match(type, str8_lit("rel32"), StringMatchFlag_CaseInsensitive)) {
      *value_out = pe.image_base + section->voff + offset + sizeof(S32) + (S32)raw;
    } else {
      *value_out = raw;
    }
  }
  return 1;
}

internal T_Result
t_codec_expect_pe_word_execute(T_Context *ctx, MD_Node *arguments)
{
  T_Artifact *artifact = t_artifact_from_name(ctx, t_codec_scalar(t_codec_child(arguments, "artifact")));
  String8 section_name = t_codec_scalar(t_codec_child(arguments, "section"));
  String8 type = t_codec_scalar(t_codec_child(arguments, "type"));
  U64 offset = 0, value = 0;
  try_u64_from_str8_c_rules(t_codec_scalar(t_codec_child(arguments, "offset")), &offset);
  if (!t_codec_pe_read_word(ctx, artifact, section_name, offset, type, &value)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_word"), "unable to read %S at %S+%llu", type, section_name, offset);
  }
  MD_Node *nonzero_node = t_codec_child(arguments, "nonzero");
  B32 nonzero = 0;
  if (!md_node_is_nil(nonzero_node) && t_bool_from_scalar(nonzero_node, &nonzero) && nonzero && value == 0) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_word"), "value at %S+%llu is zero", section_name, offset);
  }
  MD_Node *equals_node = t_codec_child(arguments, "equals");
  U64 expected = 0;
  if (!md_node_is_nil(equals_node) && (!try_u64_from_str8_c_rules(t_codec_scalar(equals_node), &expected) || value != expected)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_word"), "value at %S+%llu is %llu, expected %llu", section_name, offset, value, expected);
  }
  String8 target_section_name = t_codec_scalar(t_codec_child(arguments, "target_section"));
  if (target_section_name.size != 0) {
    PE_BinInfo pe = {0};
    COFF_SectionHeader *target = t_codec_pe_section_from_name(ctx, artifact, &pe, target_section_name);
    U64 target_offset = 0;
    try_u64_from_str8_c_rules(t_codec_scalar(t_codec_child(arguments, "target_offset")), &target_offset);
    String8 address = t_codec_scalar(t_codec_child(arguments, "target_address"));
    if (target != 0) { expected = (str8_match(address, str8_lit("rva"), StringMatchFlag_CaseInsensitive) ? 0 : pe.image_base) + target->voff + target_offset; }
    if (target == 0 || value != expected) {
      return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_word"), "value at %S+%llu does not target %S+%llu", section_name, offset, target_section_name,
                              target_offset);
    }
  }
  String8 other_section = t_codec_scalar(t_codec_child(arguments, "other_section"));
  if (other_section.size != 0) {
    U64 other_offset = 0, other_value = 0;
    try_u64_from_str8_c_rules(t_codec_scalar(t_codec_child(arguments, "other_offset")), &other_offset);
    String8 other_type = t_codec_scalar(t_codec_child(arguments, "other_type"));
    if (other_type.size == 0) { other_type = type; }
    if (!t_codec_pe_read_word(ctx, artifact, other_section, other_offset, other_type, &other_value)) {
      return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_word"), "unable to read comparison word at %S+%llu", other_section, other_offset);
    }
    B32 equal = str8_match(t_codec_scalar(t_codec_child(arguments, "relation")), str8_lit("equal"), StringMatchFlag_CaseInsensitive);
    if ((equal && value != other_value) || (!equal && value == other_value)) {
      return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_word"), "word relation at %S+%llu and %S+%llu does not hold", section_name, offset, other_section,
                              other_offset);
    }
  }
  MD_Node *modulo_node = t_codec_child(arguments, "modulo");
  if (!md_node_is_nil(modulo_node)) {
    U64 modulo = 0, remainder = 0;
    try_u64_from_str8_c_rules(t_codec_scalar(modulo_node), &modulo);
    try_u64_from_str8_c_rules(t_codec_scalar(t_codec_child(arguments, "remainder")), &remainder);
    if (modulo == 0 || value % modulo != remainder) {
      return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_word"), "value at %S+%llu has unexpected remainder", section_name, offset);
    }
  }
  return ctx->result;
}

internal T_Result
t_codec_expect_pe_bytes_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  char *allowed[] = {"artifact", "section", "offset", "hex", 0};
  t_codec_validate_fields(ctx, arguments, allowed);
  for (U64 i = 0; allowed[i] != 0; i += 1) { t_codec_validate_scalar(ctx, arguments, allowed[i], 1); }
  T_Artifact *artifact = t_artifact_from_name(ctx->run, t_codec_scalar(t_codec_child(arguments, "artifact")));
  if (artifact == 0 || !str8_match(artifact->codec->kind, str8_lit("pe"), StringMatchFlag_CaseInsensitive)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, t_codec_child(arguments, "artifact"), "expect_pe_bytes requires a PE artifact");
  }
  return ctx->run->result;
}

internal T_Result
t_codec_expect_pe_bytes_execute(T_Context *ctx, MD_Node *arguments)
{
  T_Artifact *artifact = t_artifact_from_name(ctx, t_codec_scalar(t_codec_child(arguments, "artifact")));
  String8 section_name = t_codec_scalar(t_codec_child(arguments, "section"));
  COFF_SectionHeader *section = t_codec_pe_section_from_name(ctx, artifact, 0, section_name);
  U64 offset = 0;
  try_u64_from_str8_c_rules(t_codec_scalar(t_codec_child(arguments, "offset")), &offset);
  MD_Node hex_node = {0};
  MD_Node value_node = {0};
  hex_node.kind = MD_NodeKind_Main;
  hex_node.string = str8_lit("hex");
  hex_node.first = hex_node.last = &value_node;
  value_node.kind = MD_NodeKind_Main;
  value_node.string = t_codec_scalar(t_codec_child(arguments, "hex"));
  String8 expected = {0};
  T_Result result = t_bytes_from_producer(ctx, &hex_node, &expected);
  if (!t_result_is_ok(result)) { return result; }
  if (section == 0 || offset > section->fsize || expected.size > section->fsize - offset || section->foff > artifact->data.size || section->fsize > artifact->data.size - section->foff ||
      !str8_match(str8(artifact->data.str + section->foff + offset, expected.size), expected, 0)) {
    return t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, str8_lit("expect_pe_bytes"), "bytes at %S+%llu do not match", section_name, offset);
  }
  return ctx->result;
}

internal T_Result
t_codec_no_arguments_validate(T_ParseContext *ctx, MD_Node *arguments)
{
  if (!md_node_is_nil(arguments->first)) {
    t_parse_errorf(ctx, T_ResultCode_ValidationError, arguments->first, "operation takes no arguments");
  }
  return ctx->run->result;
}

#define T_CODEC_INTERNAL_API_CHECK(expr) do { if (!(expr)) { t_context_errorf(ctx, T_ResultCode_Mismatch, arguments, operation, "assertion failed: %s", #expr); } } while (0)

internal T_Result
t_codec_get_msf_stream_pages_execute(T_Context *ctx, MD_Node *arguments)
{
  Arena *arena = ctx->arena;
  String8 operation = str8_lit("get_msf_stream_pages");
  MSF_Context *msf = msf_alloc(MSF_DEFAULT_PAGE_SIZE, MSF_DEFAULT_FPM);

  {
    U64 stream_size = MB(150) + 1;

    MSF_StreamNumber sn = msf_stream_alloc_ex(msf, stream_size);

    U8 *test = push_array(arena, U8, stream_size);
    MemorySet(test, 0xca, stream_size/2);
    MemorySet(test + stream_size/2, 0xbe, stream_size/2);

    String8List stream_data = msf_data_from_sn(arena, msf, sn);
    T_CODEC_INTERNAL_API_CHECK(stream_data.total_size == stream_size);
    T_CODEC_INTERNAL_API_CHECK(stream_data.node_count == 12);

    String8Array a = str8_array_from_list(arena, &stream_data);
    T_CODEC_INTERNAL_API_CHECK(a.v[0].size  == 0xffd000);
    T_CODEC_INTERNAL_API_CHECK(a.v[1].size  == 0xffe000);
    T_CODEC_INTERNAL_API_CHECK(a.v[2].size  == 0xffe000);
    T_CODEC_INTERNAL_API_CHECK(a.v[3].size  == 0xffe000);
    T_CODEC_INTERNAL_API_CHECK(a.v[4].size  == 0xffe000);
    T_CODEC_INTERNAL_API_CHECK(a.v[5].size  == 0xffe000);
    T_CODEC_INTERNAL_API_CHECK(a.v[6].size  == 0xffe000);
    T_CODEC_INTERNAL_API_CHECK(a.v[7].size  == 0xffd000);
    T_CODEC_INTERNAL_API_CHECK(a.v[8].size  == 0x1000);
    T_CODEC_INTERNAL_API_CHECK(a.v[9].size  == 0xffe000);
    T_CODEC_INTERNAL_API_CHECK(a.v[10].size == 0x613000);
    T_CODEC_INTERNAL_API_CHECK(a.v[11].size == 1);

    String8Node buf     = *stream_data.first;
    U64         buf_pos = 0;
    str8_buffer_write(&buf, &buf_pos, str8(test, stream_size));

    String8 cmp = msf_stream_read_block(arena, msf, sn, stream_size);
    T_CODEC_INTERNAL_API_CHECK(cmp.size == stream_size);
    T_CODEC_INTERNAL_API_CHECK(MemoryCompare(cmp.str, test, stream_size) == 0);
  }

  {
    MSF_StreamNumber sn = msf_stream_alloc_ex(msf, 1);
    String8List stream_data = msf_data_from_sn(arena, msf, sn);
    T_CODEC_INTERNAL_API_CHECK(stream_data.node_count == 1);
    T_CODEC_INTERNAL_API_CHECK(stream_data.total_size == 1);
    T_CODEC_INTERNAL_API_CHECK(stream_data.first->string.size == 1);
  }

  msf_release(msf);
  return ctx->result;
}

internal T_Result
t_codec_data_from_pdb(T_Context *ctx, MD_Node *arguments, PDB_Context *pdb, String8 *data_out)
{
  Arena      *arena    = ctx->arena;
  String8     operation = str8_lit("validate_info_stream");
  TP_Context *tp       = tp_alloc(arena, 1, 1, str8_lit("foo"));
  TP_Arena   *tp_arena = tp_arena_alloc(tp);
  pdb_build(tp, tp_arena, pdb, (CV_StringHashTable){0}, 1, 0, 0);

  MSF_Error msf_error = msf_build(pdb->msf);
  T_CODEC_INTERNAL_API_CHECK(msf_error == MSF_Error_OK);
  String8List raw_msf_list = msf_get_page_data_nodes(arena, pdb->msf);
  T_CODEC_INTERNAL_API_CHECK(t_write_file_list(str8_lit("test.pdb"), raw_msf_list));

  *data_out = str8_list_join(arena, &raw_msf_list, 0);

  tp_arena_release(&tp_arena);
  tp_release(tp);

  return ctx->result;
}

internal T_Result
t_codec_validate_info_stream_execute(T_Context *ctx, MD_Node *arguments)
{
  Arena *arena = ctx->arena;
  String8 operation = str8_lit("validate_info_stream");
  COFF_TimeStamp  time_stamp = 123;
  U32             age        = 1;
  Guid            guid       = { .data1 = max_U32, .data2 = max_U16 - 1, .data3 = max_U16 - 2, .data4 = { 1, 2, 3, 4, 5, 6, 7, 8 } };
  PDB_Context    *pdb        = pdb_alloc(MSF_DEFAULT_PAGE_SIZE, COFF_MachineType_X64, time_stamp, age, guid);

  char *stream_names[] = { "one", "two", "three", "four", "five" };
  MSF_StreamNumber stream_numbers[ArrayCount(stream_names)] = {0};

  for EachElement(i, stream_names) {
    stream_numbers[i] = pdb_push_named_stream(&pdb->info->named_stream_ht, pdb->msf, str8_cstring(stream_names[i]));
    T_CODEC_INTERNAL_API_CHECK(stream_numbers[i] != MSF_INVALID_STREAM_NUMBER);
  }

  String8 raw_msf = {0};
  t_codec_data_from_pdb(ctx, arguments, pdb, &raw_msf);
  MSF_Parsed *msf_parsed = msf_parsed_from_data(arena, raw_msf);
  String8     info_data  = msf_data_from_stream(msf_parsed, PDB_FixedStream_Info);

#if 0
  fprintf(stderr, "\n");
  for EachIndex(i, info_data.size) {
    fprintf(stderr, "0x%02x, ", info_data.str[i]);
    if (i % 19 == 18 && i > 0) { fprintf(stderr, "\n"); }
  }
#endif
  U8 expected_info_data[] = {
    0x94, 0x2e, 0x31, 0x01, 0x7b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xfd,
    0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x22, 0x00, 0x00, 0x00, 0x6f, 0x6e, 0x65, 0x00, 0x74, 0x77,
    0x6f, 0x00, 0x74, 0x68, 0x72, 0x65, 0x65, 0x00, 0x66, 0x6f, 0x75, 0x72, 0x00, 0x66, 0x69, 0x76, 0x65, 0x00, 0x2f,
    0x4c, 0x69, 0x6e, 0x6b, 0x49, 0x6e, 0x66, 0x6f, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x13,
    0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x51, 0x33, 0x01,
  };
  T_CODEC_INTERNAL_API_CHECK(str8_match(info_data, str8_array_fixed(expected_info_data), 0));

  pdb_release(pdb);
  return ctx->result;
}

internal T_Result
t_codec_psi_addr_map_radix_sort_execute(T_Context *ctx, MD_Node *arguments)
{
  Arena *arena = ctx->arena;
  String8 operation = str8_lit("psi_addr_map_radix_sort");
  String8 names[] = {
    str8_lit("alpha"),
    str8_lit("bravo"),
    str8_lit("charlie"),
    str8_lit("delta"),
  };
  U64 address_count = (1 << 15) + 1;
  U64 record_count  = address_count * ArrayCount(names);

  PDB_GsiSortRecord *records  = push_array_no_zero(arena, PDB_GsiSortRecord, record_count);
  PDB_GsiSortRecord *expected = push_array_no_zero(arena, PDB_GsiSortRecord, record_count);
  for EachIndex(i, record_count) {
    U64 address_idx = address_count - 1 - i / ArrayCount(names);
    records[i].isect_off.isect = 1 + address_idx % 257;
    records[i].isect_off.off   = address_idx / 257;
    records[i].name            = names[ArrayCount(names) - 1 - i % ArrayCount(names)];
    records[i].offset          = i * sizeof(U32);
  }
  MemoryCopyTyped(expected, records, record_count);
  radsort(expected, record_count, psi_addr_map_compar_is_before);

  TP_Context *tp = tp_alloc(arena, 1, 1, str8_lit("psi addr map sort test"));
  U32 *addr_map = psi_addr_map_from_gsi_records(tp, arena, records, record_count);

  for EachIndex(i, record_count) {
    T_CODEC_INTERNAL_API_CHECK(addr_map[i] == expected[i].offset);
  }
  tp_release(tp);
  return ctx->result;
}

internal T_Result
t_codec_u64_array_radix_sort_parallel_execute(T_Context *ctx, MD_Node *arguments)
{
  Arena *arena = ctx->arena;
  String8 operation = str8_lit("u64_array_radix_sort_parallel");
  TP_Context *tp = tp_alloc(arena, 1, 1, str8_zero());

  U64 small_values[] = { max_U64, 0, 7, 1, 7, 0x100000000ull, 2 };
  u64_array_sort_radix_parallel(tp,  ArrayCount(small_values), small_values);
  for (U64 i = 1; i < ArrayCount(small_values); i += 1) {
    T_CODEC_INTERNAL_API_CHECK(small_values[i-1] <= small_values[i]);
  }

  U64  count      = 200003;
  U64 *values     = push_array_no_zero(arena, U64, count);
  U64  state      = 0x9e3779b97f4a7c15ull;
  U64  sum_before = 0;
  U64  xor_before = 0;
  for EachIndex(i, count) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    values[i] = state * 0x2545f4914f6cdd1dull;
    if ((i % 97) == 0) { values[i] = 0; }
    if ((i % 193) == 0) { values[i] = max_U64; }
    if ((i % 389) == 0) { values[i] = 0x100000001ull; }
    sum_before += values[i];
    xor_before ^= values[i];
  }

  u64_array_sort_radix_parallel(tp, count, values);
  U64 sum_after = 0;
  U64 xor_after = 0;
  B32 is_sorted = 1;
  for EachIndex(i, count) {
    if (i > 0 && values[i-1] > values[i]) { is_sorted = 0; }
    sum_after += values[i];
    xor_after ^= values[i];
  }
  T_CODEC_INTERNAL_API_CHECK(is_sorted);
  T_CODEC_INTERNAL_API_CHECK(sum_before == sum_after);
  T_CODEC_INTERNAL_API_CHECK(xor_before == xor_after);
  tp_release(tp);
  return ctx->result;
}

internal T_Result
t_codec_patch_cv_symbol_tree_execute(T_Context *ctx, MD_Node *arguments)
{
  Arena *arena = ctx->arena;
  String8 operation = str8_lit("patch_cv_symbol_tree");
  String8List raw_symbols = {0};
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_OBJNAME,        cv_make_obj_name(arena, str8_lit("foo.obj"), 123)));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_GPROC32,        cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("Proc"))));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_INLINESITE,     cv_make_inline_site(arena, (CV_SymInlineSite){0}, str8_zero())));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_INLINESITE_END, cv_make_inline_site_end(arena)));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_END,            cv_make_end(arena)));

  U64 tree_size = cv_patch_symbol_tree_offsets(raw_symbols, sizeof(CV_Signature), 4);
  T_CODEC_INTERNAL_API_CHECK(tree_size == 84);

  {
    String8Node buf     = *raw_symbols.first;
    U64         buf_pos = 0;

    CV_SymbolHeader obj_header;
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_read(&buf, &buf_pos, sizeof(obj_header), &obj_header) == sizeof(obj_header));
    T_CODEC_INTERNAL_API_CHECK(obj_header.kind == CV_SymKind_OBJNAME);
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_skip(&buf, &buf_pos, obj_header.size - sizeof(CV_SymKind)));

    CV_SymbolHeader proc_header;
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_read(&buf, &buf_pos, sizeof(proc_header), &proc_header) == sizeof(proc_header));
    T_CODEC_INTERNAL_API_CHECK(proc_header.kind == CV_SymKind_GPROC32);

    CV_SymProc32 proc;
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_read(&buf, &buf_pos, sizeof(proc), &proc) == sizeof(proc));
    T_CODEC_INTERNAL_API_CHECK(proc.end == 0x54);
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_skip(&buf, &buf_pos, proc_header.size - sizeof(CV_SymKind) - sizeof(proc)));

    CV_SymbolHeader inline_site_header;
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_read(&buf, &buf_pos, sizeof(inline_site_header), &inline_site_header) == sizeof(inline_site_header));
    T_CODEC_INTERNAL_API_CHECK(inline_site_header.kind == CV_SymKind_INLINESITE);

    CV_SymInlineSite inline_site;
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_read(&buf, &buf_pos, sizeof(inline_site), &inline_site));
    T_CODEC_INTERNAL_API_CHECK(inline_site.parent == 0x14);
    T_CODEC_INTERNAL_API_CHECK(inline_site.end == 0x50);
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_skip(&buf, &buf_pos, inline_site_header.size - sizeof(CV_SymKind) - sizeof(inline_site)));

    CV_SymbolHeader inline_end_header;
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_read(&buf, &buf_pos, sizeof(inline_end_header), &inline_end_header) == sizeof(inline_end_header));
    T_CODEC_INTERNAL_API_CHECK(inline_end_header.kind == CV_SymKind_INLINESITE_END);

    CV_SymbolHeader proc_end_header;
    T_CODEC_INTERNAL_API_CHECK(str8_buffer_read(&buf, &buf_pos, sizeof(proc_end_header), &proc_end_header) == sizeof(proc_end_header));
    T_CODEC_INTERNAL_API_CHECK(proc_end_header.kind == CV_SymKind_END);

    T_CODEC_INTERNAL_API_CHECK(buf.string.size == 0);
    T_CODEC_INTERNAL_API_CHECK(buf.string.str == 0);
    T_CODEC_INTERNAL_API_CHECK(buf_pos == 0);
  }
  return ctx->result;
}

#undef T_CODEC_INTERNAL_API_CHECK

global T_Codec t_codec_script_codecs[] = {
    {str8_lit_comp("bytes"), 0, t_codec_bytes_encode, t_codec_bytes_decode},
    {str8_lit_comp("text"), 0, t_codec_text_encode, t_codec_text_decode},
    {str8_lit_comp("coff"), t_coff_validate, t_coff_encode, t_coff_decode},
    {str8_lit_comp("pe"), 0, 0, t_codec_pe_decode},
};

global T_OpSpec t_codec_script_ops[] = {
    {str8_lit_comp("compare"), t_op_compare_validate, t_op_compare_execute},
    {str8_lit_comp("compare_file"), t_op_compare_file_validate, t_op_compare_file_execute},
    {str8_lit_comp("repeat"), t_op_repeat_validate, t_op_repeat_execute},
    {str8_lit_comp("run"), t_codec_run_validate, t_codec_run_execute},
    {str8_lit_comp("clang"), t_codec_clang_validate, t_codec_clang_execute},
    {str8_lit_comp("expect_coff"), t_codec_expect_coff_validate, t_codec_expect_coff_execute},
    {str8_lit_comp("expect_pe"), t_codec_expect_pe_validate, t_codec_expect_pe_execute},
    {str8_lit_comp("expect_pdb"), t_codec_expect_pdb_validate, t_codec_expect_pdb_execute},
    {str8_lit_comp("expect_file"), t_codec_expect_file_validate, t_codec_expect_file_execute},
    {str8_lit_comp("expect_pe_word"), t_codec_expect_pe_word_validate, t_codec_expect_pe_word_execute},
    {str8_lit_comp("expect_pe_bytes"), t_codec_expect_pe_bytes_validate, t_codec_expect_pe_bytes_execute},
    {str8_lit_comp("get_msf_stream_pages"), t_codec_no_arguments_validate, t_codec_get_msf_stream_pages_execute},
    {str8_lit_comp("validate_info_stream"), t_codec_no_arguments_validate, t_codec_validate_info_stream_execute},
    {str8_lit_comp("psi_addr_map_radix_sort"), t_codec_no_arguments_validate, t_codec_psi_addr_map_radix_sort_execute},
    {str8_lit_comp("u64_array_radix_sort_parallel"), t_codec_no_arguments_validate, t_codec_u64_array_radix_sort_parallel_execute},
    {str8_lit_comp("patch_cv_symbol_tree"), t_codec_no_arguments_validate, t_codec_patch_cv_symbol_tree_execute},
};

global T_SuiteSpec t_codec_script_suite = {
    .name = str8_lit_comp("linker"),
    .codecs = t_codec_script_codecs,
    .codec_count = ArrayCount(t_codec_script_codecs),
    .ops = t_codec_script_ops,
    .op_count = ArrayCount(t_codec_script_ops),
};
