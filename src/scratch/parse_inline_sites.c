#define BUILD_CONSOLE_INTERFACE 1

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"

internal void
print_inline_binary_annotations(String8 binary_annots)
{
  U32 code_offset = 0;
  S32 line_offset = 0;

  for (U64 cursor = 0; cursor < binary_annots.size; ) {
    U64 op_offset = cursor;
    CV_InlineBinaryAnnotation op = CV_InlineBinaryAnnotation_Null;
    cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &op);

    fprintf(stdout, "\t\t[%04llX] ", op_offset);
    switch (op) {
    case CV_InlineBinaryAnnotation_Null: {
      fprintf(stdout, "End");
      cursor = binary_annots.size;
    } break;
    case CV_InlineBinaryAnnotation_CodeOffset: {
      U32 value = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &value);
      code_offset += value;
      fprintf(stdout, "CodeOffset: 0x%X; Code 0x%X", value, code_offset);
    } break;
    case CV_InlineBinaryAnnotation_ChangeCodeOffsetBase: {
      U32 delta;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &delta);
      code_offset += delta;
      fprintf(stdout, "ChangeCodeOffsetBase: 0x%X; Code 0x%X", delta, code_offset);
    } break;
    case CV_InlineBinaryAnnotation_ChangeCodeOffset: {
      U32 delta = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &delta);
      code_offset += delta;
      fprintf(stdout, "ChangeCodeOffset: 0x%X; Code 0x%X", delta, code_offset);
    } break;
    case CV_InlineBinaryAnnotation_ChangeCodeLength: {
      U32 delta = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &delta);
      code_offset += delta;
      fprintf(stdout, "ChangeCodeLength: 0x%X; Code End 0x%X", delta, code_offset);
    } break;
    case CV_InlineBinaryAnnotation_ChangeFile: {
      U32 file_id = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &file_id);
      fprintf(stdout, "ChangeFile: 0x%X", file_id);
    } break;
    case CV_InlineBinaryAnnotation_ChangeLineOffset: {
      S32 delta = 0;
      cursor += cv_decode_inline_annot_s32(binary_annots, cursor, &delta);
      line_offset += delta;
      fprintf(stdout, "ChangeLineOffset: %d; Line %d", delta, line_offset);
    } break;
    case CV_InlineBinaryAnnotation_ChangeLineEndDelta: {
      S32 end_delta = 0;
      cursor += cv_decode_inline_annot_s32(binary_annots, cursor, &end_delta);
      line_offset += end_delta;
      fprintf(stdout, "ChangeLineEndDelta: %d; Line %d", end_delta, line_offset);
    } break;
    case CV_InlineBinaryAnnotation_ChangeRangeKind: {
      U32 range_kind = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &range_kind);
      String8 range_kind_str = cv_string_from_inline_range_kind(range_kind);
      fprintf(stdout, "ChangeRangeKind: %.*s (%u)", str8_varg(range_kind_str), range_kind);
    } break;
    case CV_InlineBinaryAnnotation_ChangeColumnStart: {
      S32 delta = 0;
      cursor += cv_decode_inline_annot_s32(binary_annots, cursor, &delta);
      fprintf(stdout, "ChangeColumnStart: %d", delta);
    } break;
    case CV_InlineBinaryAnnotation_ChangeCodeOffsetAndLineOffset: {
      U32 code_offset_and_line_offset = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &code_offset_and_line_offset);
      S32 line_delta = cv_inline_annot_convert_to_signed_operand(code_offset_and_line_offset >> 4);
      U32 code_delta = code_offset_and_line_offset & 0xF; 
      line_offset += line_delta;
      code_offset += code_delta;
      fprintf(stdout, "ChnageCodeOffsetAndLineOffset: 0x%X %d; Code 0x%X Line %d", code_delta, line_delta, code_offset, line_offset);
    } break;
    case CV_InlineBinaryAnnotation_ChangeCodeLengthAndCodeOffset: {
      U32 code_length_delta = 0;
      U32 code_offset_delta = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &code_length_delta);
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &code_offset_delta);
      code_offset += code_offset_delta;
      fprintf(stdout, "ChangeCodeLengthAndCodeOffset: %u 0x%X; Code 0x%X Code End 0x%X", code_length_delta, code_offset_delta, code_offset, code_offset + code_length_delta);
      code_offset += code_length_delta;
    } break;
    case CV_InlineBinaryAnnotation_ChangeColumnEnd: {
      U32 column_end = 0;
      cursor += cv_decode_inline_annot_u32(binary_annots, cursor, &column_end);
      fprintf(stdout, "ChangeColumnEnd: %u", column_end);
    } break;
    default: {
      fprintf(stdout, "Unknown Inline Binary Annotation Op Code: 0x%X", op);
    } break;
    }
    fprintf(stdout, "\n");
  }
}

internal void
entry_point(CmdLine *cmdl)
{
  Arena *arena = arena_alloc();

  B32 do_help = cmd_line_has_flag(cmdl, str8_lit("help")) ||
                cmd_line_has_flag(cmdl, str8_lit("h")) ||
                (cmdl->inputs.node_count == 0 && cmdl->options.count == 0);
  if (do_help) {
    fprintf(stdout, 
            "Parse Inline Sites\n"
            "\t-pdb:<file_path>\n"
            "\t-comp_unit=<int>\n"
            "\t-base_addr=<int>\n"
            "\t-help\n");
    return;
  }

  // -comp_unit
  U64 single_comp_unit_idx = max_U64;
  B32 single_comp_unit_mode = cmd_line_has_argument(cmdl, str8_lit("comp_unit"));
  if (single_comp_unit_mode) {
    String8 comp_unit_str = cmd_line_string(cmdl, str8_lit("comp_unit"));
    if (!try_u64_from_str8_c_rules(comp_unit_str, &single_comp_unit_idx)) {
      fprintf(stderr, "ERROR: unable to parse -comp_unit=%.*s\n", str8_varg(comp_unit_str));
      return;
    }
  }

  // -base_addr
  U64 base_addr = 0;
  String8 base_str = cmd_line_string(cmdl, str8_lit("base_addr"));
  try_u64_from_str8_c_rules(base_str, &base_addr);

  // -pdb
  String8 pdb_name;
  if (cmd_line_has_argument(cmdl, str8_lit("pdb"))) { 
    pdb_name = cmd_line_string(cmdl, str8_lit("pdb"));
    if (pdb_name.size == 0) {
      fprintf(stderr, "ERROR: missing -pdb:<path>\n");
      return;
    }
  } else {
    if (cmdl->inputs.node_count == 1) {
      pdb_name = cmdl->inputs.first->string;
    } else if (cmdl->inputs.node_count == 0) {
      fprintf(stderr, "ERROR: no input PDB!\n");
      return;
    } else if (cmdl->inputs.node_count > 1) {
      fprintf(stderr, "ERROR: too many inputs\n");
      return;
    }
  }

  // read PDB from disk
  String8 pdb_data = os_data_from_file_path(arena, pdb_name);
  if (pdb_data.size == 0) {
    fprintf(stderr, "ERROR: unable to load %.*s from disk\n", str8_varg(pdb_name));
    return;
  }

  // parse msf
  MSF_Parsed *msf = msf_parsed_from_data(arena, pdb_data);
  if (!msf) {
    fprintf(stderr, "ERROR: unable to parse MSF\n");
    return;
  }

  // find dbi
  String8        dbi_data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
  PDB_DbiParsed *dbi      = pdb_dbi_from_data(arena, dbi_data);
  if (!dbi) {
    fprintf(stderr, "ERROR: unable to parse DBI\n");
    return;
  }

  // find info stream
  String8   info_data = msf_data_from_stream(msf, PDB_FixedStream_PdbInfo);
  PDB_Info *info      = pdb_info_from_data(arena, info_data);
  if (!info) {
    fprintf(stderr, "ERROR: unable to parse INFO\n");
  }

  // parse named streams
  PDB_NamedStreamTable *named_streams = pdb_named_stream_table_from_info(arena, info);
  if (!named_streams) {
    fprintf(stderr, "ERROR: unable to parse named streams\n");
    return;
  }

  // find string table
  MSF_StreamNumber strtbl_sn   = named_streams->sn[PDB_NamedStream_STRTABLE];
  String8          strtbl_data = msf_data_from_stream(msf, strtbl_sn);
  PDB_Strtbl      *strtbl      = pdb_strtbl_from_data(arena, strtbl_data);
  if (!strtbl) {
    fprintf(stderr, "ERROR: unable to parse string table\n");
    return;
  }

  // find IPI
  String8        ipi_data        = msf_data_from_stream(msf, PDB_FixedStream_Ipi);
  PDB_TpiParsed *ipi             = pdb_tpi_from_data(arena, ipi_data);
  String8        ipi_leaf_data   = pdb_leaf_data_from_tpi(ipi);
  CV_LeafParsed *ipi_leaf_parsed = cv_leaf_from_data(arena, ipi_leaf_data, ipi->itype_first);

  // find sections
  MSF_StreamNumber      section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
  String8               section_data   = msf_data_from_stream(msf, section_stream);
  PDB_CoffSectionArray *sections       = pdb_coff_section_array_from_data(arena, section_data);

  // find comp units
  String8            comp_units_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
  PDB_CompUnitArray *comp_units      = pdb_comp_unit_array_from_data(arena, comp_units_data);

  if (single_comp_unit_mode) {
    if (single_comp_unit_idx >= comp_units->count) {
      fprintf(stderr, "comp unit idx %llu is out of bounds, PDB has %llu comp unit(s)\n", single_comp_unit_idx, comp_units->count);
      return;
    }
  }

  // print run info
  DateTime now_time_universal = os_now_universal_time();
  DateTime now_time_local     = os_local_time_from_universal_time(&now_time_universal);
  String8  now_time_str       = push_date_time_string(arena, &now_time_local);
  fprintf(stdout, "Time: %.*s\n", str8_varg(now_time_str));
  fprintf(stdout, "File: %.*s\n", str8_varg(pdb_name));
  fprintf(stdout, "Size: %llu (bytes)\n", pdb_data.size);
  fprintf(stdout, "\n");

  // prepare iterator
  U64 comp_unit_idx;
  U64 comp_unit_count;
  if (single_comp_unit_mode) {
    comp_unit_idx   = single_comp_unit_idx;
    comp_unit_count = single_comp_unit_idx + 1;
  } else {
    comp_unit_idx   = 0;
    comp_unit_count = comp_units->count;
  }

  for (; comp_unit_idx < comp_unit_count; ++comp_unit_idx) {
    PDB_CompUnit *comp_unit     = comp_units->units[comp_unit_idx];
    String8       symbol_data   = pdb_data_from_unit_range(msf, comp_unit, PDB_DbiCompUnitRange_Symbols);
    String8       c13_data      = pdb_data_from_unit_range(msf, comp_unit, PDB_DbiCompUnitRange_C13);
    CV_SymParsed  symbol_parsed = cv_sym_from_data(arena, symbol_data, 4);

    // parse $$
    CV_C13SubSectionList c13_ss_list = cv_c13_sub_section_list_from_data(arena, c13_data, 4);
    CV_C13Parsed         c13_parsed  = cv_c13_parsed_from_list(&c13_ss_list);

    // find $$FILE_CKSMS
    String8 file_chksms = cv_c13_file_chksms_from_sub_sections(c13_data, &c13_parsed);

    // parse $$INLINEE_LINES 
    CV_C13SubSectionList ss_inlinee_lines = c13_parsed.v[CV_C13_SubSectionIdxKind_InlineeLines];
    CV_C13InlineeLinesParsedList inlinee_lines_parsed = cv_c13_inlinee_lines_from_sub_sections(arena, c13_data, ss_inlinee_lines);

    // parse $$LINES
    U64              c13_lines_count;
    CV_C13LineArray *c13_lines;
    {
      CV_C13SubSectionList ss_lines = c13_parsed.v[CV_C13_SubSectionIdxKind_Lines];

      c13_lines_count = 0;
      CV_C13LinesParsedList *parsed_lines = push_array_no_zero(arena, CV_C13LinesParsedList, ss_lines.count);
      {
        U64 ss_idx = 0;
        for(CV_C13SubSectionNode *ss_node = ss_lines.first; ss_node != 0; ss_node = ss_node->next, ss_idx += 1)
        {
          parsed_lines[ss_idx] = cv_c13_lines_from_sub_sections(arena, c13_data, ss_node->range);
          c13_lines_count += parsed_lines[ss_idx].count;
        }
      }

      c13_lines = push_array_no_zero(arena, CV_C13LineArray, c13_lines_count);
      for(U64 ss_idx = 0, c13_lines_idx = 0; ss_idx < ss_lines.count; ss_idx += 1)
      {
        for(CV_C13LinesParsedNode *lines_n = parsed_lines[ss_idx].first; lines_n != 0; lines_n = lines_n->next)
        {
          if(0 < lines_n->v.sec_idx && lines_n->v.sec_idx <= sections->count)
          {
            Assert(c13_lines_idx < c13_lines_count);
            U64 sec_voff = sections->sections[lines_n->v.sec_idx - 1].voff;
            c13_lines[c13_lines_idx] = cv_c13_line_array_from_data(arena, c13_data, sec_voff, lines_n->v);
            c13_lines_idx += 1;
          }
          else
          {
            Assert(!"error: out of bounds section index"); 
          }
        }
      }
    }

    CV_C13InlineeLinesAccel *inlinee_lines_accel = cv_c13_make_inlinee_lines_accel(arena, inlinee_lines_parsed);
    CV_C13LinesAccel        *lines_accel         = cv_c13_make_lines_accel(arena, c13_lines_count, c13_lines);

    fprintf(stdout, "[%llu] %.*s\n", comp_unit_idx, str8_varg(comp_unit->obj_name));

    U64     scope_level = 0;
    U64     parent_voff = 0;
    String8 parent_name = str8_lit("???");

    for (U64 rec_idx = 0; rec_idx < symbol_parsed.sym_ranges.count; ++rec_idx) {
      CV_RecRange rec_range = symbol_parsed.sym_ranges.ranges[rec_idx];
      void       *rec_raw   = symbol_data.str + rec_range.off + sizeof(CV_SymKind);
      void       *rec_opl   = symbol_data.str + rec_range.off + sizeof(CV_SymKind) + rec_range.hdr.size;

      if (rec_range.hdr.kind == CV_SymKind_LPROC32 || 
          rec_range.hdr.kind == CV_SymKind_GPROC32) {
        CV_SymProc32 *proc32 = rec_raw;
        parent_voff = sections->sections[proc32->sec-1].voff + proc32->off;
        parent_name = str8_cstring_capped(proc32+1, rec_opl);

        scope_level += 1;
      } else if (rec_range.hdr.kind == CV_SymKind_BLOCK32) {
        scope_level += 1;
      } else if (rec_range.hdr.kind == CV_SymKind_END) {
        if (parent_voff) {
          Assert(scope_level > 0);
          scope_level -= 1;
          if (scope_level == 0) {
            parent_voff = 0;
            parent_name = str8_lit("???");
          }
        }
      } else if (rec_range.hdr.kind == CV_SymKind_INLINESITE) {
        CV_SymInlineSite            *inline_site         = rec_raw;
        String8                      binary_annots       = str8((U8*)(inline_site + 1), rec_range.hdr.size - sizeof(rec_range.hdr.kind) - sizeof(*inline_site));
        CV_C13InlineeLinesParsed    *inlinee_parsed      = cv_c13_inlinee_lines_accel_find(inlinee_lines_accel, inline_site->inlinee);
        CV_InlineBinaryAnnotsParsed  binary_annots_parse = cv_c13_parse_inline_binary_annots(arena, parent_voff, inlinee_parsed, binary_annots);

        
        String8 inlinee_name = str8_lit("???");
        if (ipi->itype_first <= inline_site->inlinee && inline_site->inlinee < ipi->itype_opl) {
          CV_RecRange inlinee_rec = ipi_leaf_parsed->leaf_ranges.ranges[inline_site->inlinee - ipi->itype_first];
          void *leaf_raw = ipi_leaf_data.str + inlinee_rec.off + sizeof(CV_LeafKind);
          void *leaf_opl = ipi_leaf_data.str + inlinee_rec.off + sizeof(CV_LeafKind) + inlinee_rec.hdr.size;
          if (inlinee_rec.hdr.kind == CV_LeafIDKind_MFUNC_ID) {
            CV_LeafMFuncId *mfunc_id = leaf_raw;
            inlinee_name = str8_cstring_capped(mfunc_id + 1, leaf_opl);
          } else if (inlinee_rec.hdr.kind == CV_LeafIDKind_FUNC_ID) {
            CV_LeafFuncId *func_id = leaf_raw;
            inlinee_name = str8_cstring_capped(func_id + 1, leaf_opl);
          }
        }


        String8 first_ln  = str8_lit("???");
        String8 file_off  = str8_lit("???");
        if (inlinee_parsed) {
          first_ln  = push_str8f(arena, "%u", inlinee_parsed->first_source_ln);
          file_off  = push_str8f(arena, "0x%X", inlinee_parsed->file_off);
        }

        fprintf(stdout, "\tInline site @ %06llX, Parent VOFF: 0x%llX (%.*s), Inlinee: 0x%X (%.*s), First LN: %.*s, File Off: %.*s\n",
                (rec_range.off + sizeof(rec_range.hdr.kind)),
                parent_voff,
                str8_varg(parent_name),
                inline_site->inlinee,
                str8_varg(inlinee_name),
                str8_varg(first_ln),
                str8_varg(file_off));

        fprintf(stdout, "\t\tOpcodes:\n");
        print_inline_binary_annotations(binary_annots);
        fprintf(stdout, "\n");

        fprintf(stdout, "\t\tLines:\n");
        for (U64 lines_idx = 0; lines_idx < binary_annots_parse.lines_count; lines_idx += 1) {
          CV_C13LineArray lines = binary_annots_parse.lines[lines_idx];

          CV_C13_Checksum checksum = {0};
          str8_deserial_read_struct(file_chksms, lines.file_off, &checksum);

          String8 file_name = pdb_strtbl_string_from_off(strtbl, checksum.name_off);

          fprintf(stdout, "\t\t%.*s\n", str8_varg(file_name));
          for (U64 line_idx = 0; line_idx < lines.line_count; ++line_idx) {
            char *pre  = (line_idx % 4) == 0 ? "\t\t\t" : "\t";
            char *post = (line_idx % 4) == 3 || ((line_idx + 1) == lines.line_count) ? "\n" : "";
            fprintf(stdout, "%s%08llX %04u%s", pre, base_addr + lines.voffs[line_idx], lines.line_nums[line_idx], post);
          }
        }
        fprintf(stdout, "\n");

        fprintf(stdout, "\t\tCode Ranges:\n");
        for (U64 range_idx = 0; range_idx < binary_annots_parse.code_range_count; ++range_idx) {
          char *pre  = (range_idx % 4) == 0 ? "\t\t\t" : "\t";
          char *post = (range_idx % 4) == 3 || ((range_idx + 1) == binary_annots_parse.code_range_count) ? "\n" : "";
          fprintf(stdout, "%s%08llX-%08llX%s", pre, base_addr + binary_annots_parse.code_ranges[range_idx].min, base_addr + binary_annots_parse.code_ranges[range_idx].max, post);
        }
        fprintf(stdout, "\n");
      }
    }
  }
}


