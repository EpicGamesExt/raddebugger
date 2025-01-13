#define BUILD_CONSOLE_INTERFACE 1

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "pdb/pdb_stringize.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "pdb/pdb_stringize.c"

////////////////////////////////

#include "linker/base_ext/base_blake3.h"
#include "linker/base_ext/base_blake3.c"
#include "third_party/md5/md5.c"
#include "third_party/md5/md5.h"
#include "third_party/xxHash/xxhash.c"
#include "third_party/xxHash/xxhash.h"

#include "linker/base_ext/base_inc.h"
#include "linker/path_ext/path.h"
#include "linker/hash_table.h"
#include "linker/thread_pool/thread_pool.h"
#include "linker/codeview_ext/codeview.h"

#include "linker/base_ext/base_inc.c"
#include "linker/path_ext/path.c"
#include "linker/hash_table.c"
#include "linker/thread_pool/thread_pool.c"
#include "linker/codeview_ext/codeview.c"

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
      CV_InlineRangeKind range_kind = 0;
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
      S32 line_delta = cv_inline_annot_signed_from_unsigned_operand(code_offset_and_line_offset >> 4);
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
  String8   info_data = msf_data_from_stream(msf, PDB_FixedStream_Info);
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
  MSF_StreamNumber strtbl_sn   = named_streams->sn[PDB_NamedStream_StringTable];
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
  MSF_StreamNumber        section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
  String8                 section_data   = msf_data_from_stream(msf, section_stream);
  COFF_SectionHeaderArray sections       = pdb_coff_section_array_from_data(arena, section_data);

  // find comp units
  String8            comp_units_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
  PDB_CompUnitArray *comp_units      = pdb_comp_unit_array_from_data(arena, comp_units_data);

  if (single_comp_unit_mode) {
    if (single_comp_unit_idx >= comp_units->count) {
      fprintf(stderr, "comp unit idx %llu is out of bounds, PDB has %llu comp unit(s)\n", single_comp_unit_idx, comp_units->count);
      return;
    }
  }

#if 0
  // print run info
  DateTime now_time_universal = os_now_universal_time();
  DateTime now_time_local     = os_local_time_from_universal_time(&now_time_universal);
  String8  now_time_str       = push_date_time_string(arena, &now_time_local);
  fprintf(stdout, "Time: %.*s\n", str8_varg(now_time_str));
  fprintf(stdout, "File: %.*s\n", str8_varg(pdb_name));
  fprintf(stdout, "Size: %llu (bytes)\n", pdb_data.size);
  fprintf(stdout, "\n");
#endif

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

    // parse $$
    CV_DebugS debug_s = cv_parse_debug_s_c13(arena, c13_data);

    // find $$FILE_CKSMS
    String8 file_chksms = cv_file_chksms_from_debug_s(debug_s);

    // parse $$INLINEE_LINES 
    String8List                  ss_inlinee_lines     = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_InlineeLines);
    CV_C13InlineeLinesParsedList inlinee_lines_parsed = cv_c13_inlinee_lines_from_sub_sections(arena, ss_inlinee_lines);

    // parse $$LINES
    U64           c13_lines_count = 0;
    CV_LineArray *c13_lines       = 0;
    {
      String8List raw_lines_list = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_Lines);

      for (String8Node *raw_lines_node = raw_lines_list.first; raw_lines_node != 0; raw_lines_node = raw_lines_node->next) {
        Temp temp = temp_begin(arena);
        CV_C13LinesHeaderList parsed_list = cv_c13_lines_from_sub_sections(temp.arena, raw_lines_node->string, rng_1u64(0, raw_lines_node->string.size));
        c13_lines_count += parsed_list.count;
        temp_end(temp);
      }

      c13_lines = push_array_no_zero(arena, CV_LineArray, c13_lines_count);

      U64 c13_lines_idx = 0;
      for (String8Node *raw_lines_node = raw_lines_list.first; raw_lines_node != 0; raw_lines_node = raw_lines_node->next) {
        String8               raw_lines   = raw_lines_node->string;
        CV_C13LinesHeaderList parsed_list = cv_c13_lines_from_sub_sections(arena, raw_lines, rng_1u64(0, raw_lines.size));

        for(CV_C13LinesHeaderNode *header_node = parsed_list.first; header_node != 0; header_node = header_node->next) {
          if (0 < header_node->v.sec_idx && header_node->v.sec_idx <= sections.count) {
            Assert(c13_lines_idx < c13_lines_count);
            U64 sec_voff = sections.v[header_node->v.sec_idx - 1].voff;
            c13_lines[c13_lines_idx++] = cv_c13_line_array_from_data(arena, raw_lines, sec_voff, header_node->v);
          } else {
            Assert(!"error: out of bounds section index"); 
          }
        }
      }
    }

    String8List                  raw_inlinee_lines = cv_sub_section_from_debug_s(debug_s, CV_C13SubSectionKind_InlineeLines);
    CV_C13InlineeLinesParsedList inlinee_lines     = cv_c13_inlinee_lines_from_sub_sections(arena, raw_inlinee_lines);

    CV_InlineeLinesAccel *inlinee_lines_accel = cv_c13_make_inlinee_lines_accel(arena, inlinee_lines);
    CV_LinesAccel        *lines_accel         = cv_c13_make_lines_accel(arena, c13_lines_count, c13_lines);

    fprintf(stdout, "[%llu] %.*s\n", comp_unit_idx, str8_varg(comp_unit->obj_name));

    U64     scope_level = 0;
    U64     parent_voff = 0;
    String8 parent_name = str8_lit("???");

    CV_SymbolList symbol_list = {0};
    cv_parse_symbol_sub_section(arena, &symbol_list, 0, symbol_data, CV_SymbolAlign);

    for (CV_SymbolNode *symbol_n = symbol_list.first; symbol_n != 0; symbol_n = symbol_n->next) {
      CV_Symbol  symbol  = symbol_n->data;
      void      *rec_raw = symbol.data.str;
      void      *rec_opl = symbol.data.str + symbol.data.size;

      if (symbol.kind == CV_SymKind_LPROC32 || symbol.kind == CV_SymKind_GPROC32) {
        CV_SymProc32 *proc32 = rec_raw;
        parent_voff = sections.v[proc32->sec-1].voff + proc32->off;
        parent_name = str8_cstring_capped(proc32+1, rec_opl);

        scope_level += 1;
      } else if (symbol.kind == CV_SymKind_BLOCK32) {
        scope_level += 1;
      } else if (symbol.kind == CV_SymKind_END) {
        if (parent_voff) {
          Assert(scope_level > 0);
          scope_level -= 1;
          if (scope_level == 0) {
            parent_voff = 0;
            parent_name = str8_lit("???");
          }
        }
      } else if (symbol.kind == CV_SymKind_INLINESITE) {
        CV_SymInlineSite            *inline_site         = rec_raw;
        String8                      binary_annots       = str8_skip(symbol.data, sizeof(*inline_site));
        CV_C13InlineeLinesParsed    *inlinee_parsed      = cv_c13_inlinee_lines_accel_find(inlinee_lines_accel, inline_site->inlinee);
        CV_InlineBinaryAnnotsParsed  binary_annots_parse = cv_c13_parse_inline_binary_annots(arena, parent_voff, inlinee_parsed, binary_annots);

        
        String8 inlinee_name = str8_lit("???");
        if (ipi->itype_first <= inline_site->inlinee && inline_site->inlinee < ipi->itype_opl) {
          CV_RecRange inlinee_rec = ipi_leaf_parsed->leaf_ranges.ranges[inline_site->inlinee - ipi->itype_first];
          void *leaf_raw = ipi_leaf_data.str + inlinee_rec.off + sizeof(CV_LeafKind);
          void *leaf_opl = ipi_leaf_data.str + inlinee_rec.off + sizeof(CV_LeafKind) + inlinee_rec.hdr.size;
          if (inlinee_rec.hdr.kind == CV_LeafKind_MFUNC_ID) {
            CV_LeafMFuncId *mfunc_id = leaf_raw;
            inlinee_name = str8_cstring_capped(mfunc_id + 1, leaf_opl);
          } else if (inlinee_rec.hdr.kind == CV_LeafKind_FUNC_ID) {
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
                (symbol.offset + sizeof(symbol.kind)),
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
          CV_LineArray lines = binary_annots_parse.lines[lines_idx];

          CV_C13Checksum checksum = {0};
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
        U64 range_idx = 0;
        for (Rng1U64Node *range_n = binary_annots_parse.code_ranges.first; range_n != 0; range_n = range_n->next, ++range_idx) {
          char *pre  = (range_idx % 4) == 0 ? "\t\t\t" : "\t";
          char *post = (range_idx % 4) == 3 || ((range_idx + 1) == binary_annots_parse.code_ranges.count) ? "\n" : "";
          fprintf(stdout, "%s%08llX-%08llX%s", pre, base_addr + range_n->v.min, base_addr + range_n->v.max, post);
        }
        fprintf(stdout, "\n");
      }
    }
  }
}


