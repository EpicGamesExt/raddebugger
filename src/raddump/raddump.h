// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDUMP_H
#define RADDUMP_H

#define RD_INDENT_WIDTH 4
#define RD_INDENT_MAX   4096

#define rd_printf(f, ...) str8_list_pushf(arena, out, "%S" f, indent, __VA_ARGS__)
#define rd_newline()      str8_list_pushf(arena, out, "");
#define rd_errorf(f, ...) rd_printf("ERROR: "f, __VA_ARGS__)
#define rd_indent()       do { if (indent.size + RD_INDENT_WIDTH <= RD_INDENT_MAX) { indent.size += RD_INDENT_WIDTH; } else { Assert(!"indent overflow");   } } while (0)
#define rd_unindent()     do { if (indent.size >= RD_INDENT_WIDTH)                   { indent.size -= RD_INDENT_WIDTH; } else { Assert(!"unbalanced indent"); } } while (0)

typedef U64 RD_Option;
enum RD_OptionEnum
{
  RD_Option_Help       = (1 << 0),
  RD_Option_Version    = (1 << 1),
  RD_Option_Headers    = (1 << 2),
  RD_Option_Sections   = (1 << 3),
  RD_Option_Debug      = (1 << 4),
  RD_Option_Imports    = (1 << 5),
  RD_Option_Exports    = (1 << 6),
  RD_Option_Disasm     = (1 << 7),
  RD_Option_Rawdata    = (1 << 8),
  RD_Option_Tls        = (1 << 9),
  RD_Option_Codeview   = (1 << 10),
  RD_Option_Symbols    = (1 << 11),
  RD_Option_Relocs     = (1 << 12),
  RD_Option_Exceptions = (1 << 13),
  RD_Option_LoadConfig = (1 << 14),
  RD_Option_Resources  = (1 << 15),
  RD_Option_LongNames  = (1 << 16),
};

typedef struct RD_Marker
{
  U64     off;
  String8 string;
} RD_Marker;

typedef struct RD_MarkerArray
{
  U64        count;
  RD_Marker *v;
} RD_MarkerArray;

typedef struct MarkerNode
{
  struct MarkerNode *next;
  RD_Marker          v;
} RD_MarkerNode;

typedef struct RD_MarkerList
{
  U64            count;
  RD_MarkerNode *first;
  RD_MarkerNode *last;
} RD_MarkerList;

typedef struct RD_DisasmResult
{
  String8 text;
  U64     size;
} RD_DisasmResult;

////////////////////////////////

//- Markers

internal RD_MarkerArray * rd_section_markers_from_coff_symbol_table(Arena *arena, String8 raw_data, U64 string_table_off, U64 section_count, COFF_Symbol32Array symbols);

//- Disasm

internal RD_DisasmResult rd_disasm_next_instruction(Arena *arena, Arch arch, U64 addr, String8 raw_code);
internal void            rd_format_disasm(Arena *arena, String8List *out, String8 indent, Arch arch, U64 image_base, U64 sect_off, U64 marker_count, RD_Marker *markers, String8 raw_code);

//- Raw Data

internal String8 rd_format_hex_array(Arena *arena, U8 *ptr, U64 size);
internal void    rd_format_raw_data(Arena *arena, String8List *out, String8 indent, U64 bytes_per_row, U64 marker_count, RD_Marker *markers, String8 raw_data);

//- CodeView

internal void cv_format_binary_annots(Arena *arena, String8List *out, String8 indent, CV_Arch arch, String8 raw_data);
internal void cv_format_lvar_addr_range(Arena *arena, String8List *out, String8 indent, CV_LvarAddrRange range);
internal void cv_format_lvar_addr_gap(Arena *arena, String8List *out, String8 indent, String8 raw_data);
internal void cv_format_lvar_attr(Arena *arena, String8List *out, String8 indent, CV_LocalVarAttr attr);
internal void cv_format_symbol(Arena *arena, String8List *out, String8 indent, CV_Arch arch, U32 type, String8 raw_symbol);
internal U64  cv_format_leaf(Arena *arena, String8List *out, String8 indent, CV_LeafKind kind, String8 raw_leaf);
internal void cv_format_debug_t(Arena *arena, String8List *out, String8 indent, CV_DebugT debug_t);
internal void cv_format_symbols_c13(Arena *arena, String8List *out, String8 indent, String8 raw_data);
internal void cv_format_lines_c13(Arena *arena, String8List *out, String8 indent, String8 raw_lines);
internal void cv_format_file_checksums(Arena *arena, String8List *out, String8 indent, String8 raw_chksums);
internal void cv_format_string_table(Arena *arena, String8List *out, String8 indent, String8 raw_strtab);
internal void cv_format_inlinee_lines(Arena *arena, String8List *out, String8 indent, String8 raw_data);
internal void cv_format_symbols_section(Arena *arena, String8List *out, String8 indent, String8 raw_ss);

//- COFF

internal void coff_format_archive_member_header(Arena *arena, String8List *out, String8 indent, COFF_ArchiveMemberHeader header, String8 long_names);
internal void coff_format_section_table(Arena *arena, String8List *out, String8 indent, String8 raw_data, U64 string_table_off, COFF_Symbol32Array symbols, U64 sect_count, COFF_SectionHeader *sect_headers);
internal void coff_disasm_sections(Arena *arena, String8List *out, String8 indent, String8 raw_data, COFF_MachineType machine, U64 image_base, B32 is_obj, RD_MarkerArray *section_markers, U64 section_count, COFF_SectionHeader *sections);
internal void coff_raw_data_sections(Arena *arena, String8List *out, String8 indent, String8 raw_data, B32 is_obj, RD_MarkerArray *section_markers, U64 section_count, COFF_SectionHeader *sections);
internal void coff_format_relocs(Arena *arena, String8List *out, String8 indent, String8 raw_data, U64 string_table_off, COFF_MachineType machine, U64 sect_count, COFF_SectionHeader *sect_headers, COFF_Symbol32Array symbols);
internal void coff_format_symbol_table(Arena *arena, String8List *out, String8 indent, String8 raw_data, B32 is_big_obj, U64 string_table_off, COFF_Symbol32Array symbols);
internal void coff_format_big_obj_header(Arena *arena, String8List *out, String8 indent, COFF_HeaderBigObj *header);
internal void coff_format_header(Arena *arena, String8List *out, String8 indent, COFF_Header *header);
internal void coff_format_import(Arena *arena, String8List *out, String8 indent, COFF_ImportHeader *header);
internal void coff_format_big_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts);
internal void coff_format_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts);
internal void coff_format_archive(Arena *arena, String8List *out, String8 indent, String8 raw_archive, RD_Option opts);

//- MSVC CRT

internal void mscrt_format_eh_handler_type32(Arena *arena, String8List *out, String8 indent, MSCRT_EhHandlerType32 *handler);

//- PE

internal void pe_format_data_directory_ranges(Arena *arena, String8List *out, String8 indent, U64 count, PE_DataDirectory *dirs);
internal void pe_format_optional_header32(Arena *arena, String8List *out, String8 indent, PE_OptionalHeader32 *opt_header, PE_DataDirectory *dirs);
internal void pe_format_optional_header32plus(Arena *arena, String8List *out, String8 indent, PE_OptionalHeader32Plus *opt_header, PE_DataDirectory *dirs);
internal void pe_format_load_config32(Arena *arena, String8List *out, String8 indent, PE_LoadConfig32 *lc);
internal void pe_format_load_config64(Arena *arena, String8List *out, String8 indent, PE_LoadConfig64 *lc);
internal void pe_format_tls(Arena *arena, String8List *out, String8 indent, PE_ParsedTLS tls);
internal void pe_format_debug_directory(Arena *arena, String8List *out, String8 indent, String8 raw_data, String8 raw_dir);
internal void pe_format_export_table(Arena *arena, String8List *out, String8 indent, PE_ParsedExportTable exptab);
internal void pe_format_static_import_table(Arena *arena, String8List *out, String8 indent, U64 image_base, PE_ParsedStaticImportTable imptab);
internal void pe_format_delay_import_table(Arena *arena, String8List *out, String8 indent, U64 image_base, PE_ParsedDelayImportTable imptab);
internal void pe_format_resources(Arena *arena, String8List *out, String8 indent, PE_ResourceDir *root);
internal void pe_format_exceptions_x8664(Arena *arena, String8List *out, String8 indent, U64 section_count, COFF_SectionHeader *sections, String8 raw_data, Rng1U64 except_frange);
internal void pe_format_exceptions(Arena *arena, String8List *out, String8 indent, COFF_MachineType machine, U64 section_count, COFF_SectionHeader *sections, String8 raw_data, Rng1U64 except_frange);
internal void pe_format_base_relocs(Arena *arena, String8List *out, String8 indent, COFF_MachineType machine, U64 image_base, U64 section_count, COFF_SectionHeader *sections, String8 raw_data, Rng1U64 base_reloc_franges);
internal void pe_format(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts);

////////////////////////////////

internal void format_preamble(Arena *arena, String8List *out, String8 indent, String8 input_path, String8 raw_data);

#endif // RADDUMP_H
