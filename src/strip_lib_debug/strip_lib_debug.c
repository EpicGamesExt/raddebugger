// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define BUILD_TITLE "Epic Games Tools (R) Lib Strip Debug"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
// Headers

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"

////////////////////////////////
// Implementations

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"

internal void
sld_main(CmdLine *cmdl)
{
  B32 do_help = cmd_line_has_flag(cmdl, str8_lit("help")) ||
                cmd_line_has_flag(cmdl, str8_lit("h"))    ||
                cmd_line_has_flag(cmdl, str8_lit("?"))    ||
                cmdl->argc == 1;
  if (do_help) {    fprintf(stderr, "--- Help ---------------------------------------------------------------------\n");
    fprintf(stderr, " %s\n\n", BUILD_TITLE_STRING_LITERAL);
    fprintf(stderr, " Usage: strip_lib_debug [Options]\n\n");
    fprintf(stderr, " Options:\n");
    fprintf(stderr, "  -in:<path>  Path to input lib file\n");
    fprintf(stderr, "  -out:<path> Path to output lib file\n");
    os_abort(0);
  }

  Temp scratch = scratch_begin(0,0);

  String8 in_lib_path  = cmd_line_string(cmdl, str8_lit("in"));
  String8 out_lib_path = cmd_line_string(cmdl, str8_lit("out"));

  if (in_lib_path.size == 0) {
    fprintf(stderr, "ERROR: please provide an input path via -in:<path>\n");
    os_abort(1);
  }
  if (out_lib_path.size == 0) {
    fprintf(stderr, "ERROR: please provide an output path via -out:<path>\n");
    os_abort(1);
  }

  String8 in_lib = os_data_from_file_path(scratch.arena, in_lib_path);
  if (in_lib.size == 0) {
    fprintf(stderr, "ERROR: unable to read file %.*s\n", str8_varg(in_lib_path));
    os_abort(1);
  }

  if (!coff_is_regular_archive(in_lib)) {
    fprintf(stderr, "ERROR: input lib is not COFF archive\n");
    os_abort(1);
  }

  // read & parse lib
  String8           out_lib = push_str8_copy(scratch.arena, in_lib);
  COFF_ArchiveParse parse   = coff_archive_parse_from_data(out_lib);

  // was parse successful?
  if (parse.error.size) {
    fprintf(stderr, "ERROR: %.*s: %.*s\n", str8_varg(in_lib_path), str8_varg(parse.error));
    os_abort(1);
  }

  // convert big endian offsets
  U32  member_offsets_count = parse.first_member.symbol_count;
  U32 *member_offsets       = push_array(scratch.arena, U32, parse.first_member.member_offset_count);
  for (U32 offset_idx = 0; offset_idx < member_offsets_count; offset_idx += 1) {
    member_offsets[offset_idx] = from_be_u32(parse.first_member.member_offsets[offset_idx]);
  }

  // fixup sections
  for (U64 member_idx = 0; member_idx < member_offsets_count; member_idx += 1) {
    COFF_ParsedArchiveMemberHeader member_header = {0};
    coff_parse_archive_member_header(out_lib, member_offsets[member_idx], &member_header);
    String8       member_data = str8_substr(out_lib, member_header.data_range);
    COFF_DataType member_type = coff_data_type_from_data(member_data);
    if (member_type == COFF_DataType_BigObj || member_type == COFF_DataType_Obj) {
      COFF_FileHeaderInfo  file_header_info = coff_file_header_info_from_data(member_data);
      COFF_SectionHeader  *section_table    = (COFF_SectionHeader *)str8_substr(member_data, file_header_info.section_table_range).str;
      String8              string_table     = str8_substr(member_data, file_header_info.string_table_range);
      for (U64 sect_idx = 0; sect_idx < file_header_info.section_count_no_null; sect_idx += 1) {
        COFF_SectionHeader *sect_header = &section_table[sect_idx];
        String8             name        = coff_name_from_section_header(string_table, sect_header);
        if (str8_match(str8_lit(".debug$S"), name, 0) || str8_match(str8_lit(".debug$T"), name, 0)) {
          sect_header->flags = COFF_SectionFlag_LnkRemove;
          MemorySet(sect_header->name, 'x', sizeof(sect_header->name));
        }
      }
    }
  }

  // wirte modified library
  if (!os_write_data_to_file_path(out_lib_path, out_lib)) {
    fprintf(stderr, "ERROR: unable to write output file to %.*s\n", str8_varg(out_lib_path));
    os_abort(1);
  }

  scratch_end(scratch);
}

internal void
entry_point(CmdLine *cmdl)
{
  sld_main(cmdl);
}

