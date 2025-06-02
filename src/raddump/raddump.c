// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

// TODO:
//
// Code View:
//  [x] Test .debug$S
//  [x] Test .debug$T
//
// Test dumper on following sections:
//  [x] .debug_abbrev
//  [ ] .debug_aranges
//  [ ] .debug_frame
//  [x] .debug_info
//  [x] .debug_line
//  [ ] .debug_loc
//  [ ] .debug_macinfo
//  [ ] .debug_pubnames
//  [ ] .debug_pubtypes
//  [x] .debug_str
//  [x] .debug_ranges
//  [ ] .debug_addr
//  [ ] .debug_loclists
//  [ ] .debug_rnglists
//  [ ] .debug_stroffsets
//  [ ] .debug_linestr
//  [ ] .debug_names
//
// [ ] port ELF dumper from internal repo
// [ ] port COFF/PE resource dumper from internal repo
// [x] hook up RDI symbolizer

internal void
rd_stderr(char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 f = push_str8fv(scratch.arena, fmt, args);
  fprintf(stderr, "%.*s\n", str8_varg(f));
  va_end(args);
  scratch_end(scratch);
}

internal RDI_Parsed *
rd_rdi_from_pe(Arena *arena, String8 pe_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // make command line for converter
  String8List cmdl_string = {0};
  str8_list_pushf(scratch.arena, &cmdl_string, "-pe:%S", pe_path);
  CmdLine cmdl = cmd_line_from_string_list(scratch.arena, cmdl_string);
  
  // run converter
  String8 raw_rdi = rc_rdi_from_cmd_line(scratch.arena, &cmdl);
  
  // load RDI
  RDI_Parsed *rdi = 0;
  if (raw_rdi.size) {
    rdi = push_array(arena, RDI_Parsed, 1);
    
    // TODO: check guid
    RDI_ParseStatus parse_status = rdi_parse(raw_rdi.str, raw_rdi.size, rdi);
    
    String8 parse_status_string = str8_zero();
    if (parse_status == RDI_ParseStatus_Good) {
    } else if (parse_status == RDI_ParseStatus_HeaderDoesNotMatch) {
      parse_status_string = str8_lit("Header does not match");
    } else if (parse_status == RDI_ParseStatus_UnsupportedVersionNumber) {
      parse_status_string = str8_lit("Unsupported version number");
    } else if (parse_status == RDI_ParseStatus_InvalidDataSecionLayout) {
      parse_status_string = str8_lit("Invalid data section layout");
    } else if (parse_status == RDI_ParseStatus_MissingRequiredSection) {
      parse_status_string = str8_lit("Missing required section");
    } else {
      parse_status_string = push_str8f(scratch.arena, "unknown parse status code: %u", parse_status);
    }
    
    if (parse_status_string.size) {
      rdi = 0;
      rd_errorf("RDI parse status(%u): %S", parse_status, parse_status_string);
    }
  }
  
  scratch_end(scratch);
  return rdi;
}

internal String8
rd_proc_name_from_voff(RDI_Parsed *rdi, U64 voff)
{
  RDI_Scope     *scope = rdi_scope_from_voff(rdi, voff);
  RDI_Procedure *proc  = rdi_procedure_from_scope(rdi, scope);
  String8        name  = {0};
  name.str = rdi_string_from_idx(rdi, proc->name_string_idx, &name.size);
  return name;
}

internal String8
rd_format_proc_line(Arena *arena, RDI_Parsed *rdi, U64 voff)
{
  RDI_Scope     *scope = rdi_scope_from_voff(rdi, voff);
  RDI_Procedure *proc  = rdi_procedure_from_scope(rdi, scope);
  String8        name  = {0};
  name.str = rdi_string_from_idx(rdi, proc->name_string_idx, &name.size);
  U64            rel   = voff - scope->voff_range_first;
  String8 result = str8_zero();
  if (name.size) {
    if (rel) {
      result = push_str8f(arena, "%S+%llx", name, rel);
    } else {
      result = push_str8f(arena, "%S", name);
    }
  }
  return result;
}

internal String8
rd_path_from_file_path_node_idx(Arena *arena, RDI_Parsed *rdi, U32 file_path_node_idx, PathStyle style)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List parts = {0};
  for(RDI_FilePathNode *fpn = rdi_element_from_name_idx(rdi, FilePathNodes, file_path_node_idx);
      fpn != rdi_element_from_name_idx(rdi, FilePathNodes, 0);
      fpn = rdi_element_from_name_idx(rdi, FilePathNodes, fpn->parent_path_node)) {
    String8 p;
    p.str = rdi_string_from_idx(rdi, fpn->name_string_idx, &p.size);
    str8_list_push_front(scratch.arena, &parts, p);
  }
  String8 path = str8_path_list_join_by_style(arena, &parts, style);
  scratch_end(scratch);
  return path;
}

internal RD_Line
rd_line_from_voff(Arena *arena, RDI_Parsed *rdi, U64 voff, PathStyle path_style)
{
  RDI_Line        line      = rdi_line_from_voff(rdi, voff);
  RDI_SourceFile *src_file  = rdi_source_file_from_line(rdi, &line);
  String8         file_path = rd_path_from_file_path_node_idx(arena, rdi, src_file->file_path_node_idx, path_style);
  
  RD_Line result   = {0};
  result.file_path = file_path;
  result.line_num  = line.line_num;
  
  return result;
}

internal String8
rd_format_line_from_voff(Arena *arena, RDI_Parsed *rdi, U64 voff, PathStyle path_style)
{
  Temp scratch = scratch_begin(&arena, 1);
  RD_Line line   = rd_line_from_voff(scratch.arena, rdi, voff, path_style);
  String8 result = push_str8f(arena, "%S:%u", line.file_path, line.line_num);
  scratch_end(scratch);
  return result;
}

internal B32
rd_is_rdi(String8 raw_data)
{
  B32 is_rdi = 0;
  if (raw_data.size > 8) {
    U64 *magic = (U64 *)raw_data.str;
    is_rdi = *magic == RDI_MAGIC_CONSTANT;
  }
  return is_rdi;
}

internal String8
rd_string_from_reg_off(Arena *arena, String8 reg_str, S64 reg_off)
{
  String8 result;
  if (reg_off > 0) {
    result = push_str8f(arena, "%S%+lld", reg_str, reg_off);
  } else {
    result = push_str8f(arena, "%S", reg_str);
  }
  return result;
}

internal String8
rd_string_from_flags(Arena *arena, String8List list, U64 remaining_flags)
{
  String8 result;
  if (list.node_count == 0 && remaining_flags == 0) {
    result = str8_lit("0");
  } else {
    Temp scratch = scratch_begin(&arena, 1);
    if (remaining_flags != 0) {
      str8_list_pushf(scratch.arena, &list, "Unknown flags: %#llx", remaining_flags);
    }
    result = str8_list_join(arena, &list, &(StringJoin){.sep = str8_lit(", ") });
    scratch_end(scratch);
  }
  return result;
}

internal String8
rd_string_from_array_u32(Arena *arena, U32 *v, U64 count)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  for (U64 i = 0; i < count; ++i) {
    str8_list_pushf(scratch.arena, &list, "%u", v[i]);
  }
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return result;
}

internal String8
rd_string_from_hex_u8(Arena *arena, U8 *v, U64 count)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  for (U64 i = 0; i < count; ++i) {
    str8_list_pushf(scratch.arena, &list, "%#x", v[i]);
  }
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return result;
}

internal String8
rd_string_from_array_hex_u32(Arena *arena, U32 *v, U64 count)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  for (U64 i = 0; i < count; ++i) {
    str8_list_pushf(scratch.arena, &list, "%#x", v[i]);
  }
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return result;
}

internal String8
rd_string_from_array_hex_u64(Arena *arena, U64 *v, U64 count)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  for (U64 i = 0; i < count; ++i) {
    str8_list_pushf(scratch.arena, &list, "%#llx", v[i]);
  }
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return result;
}

internal String8
rd_string_from_range_array_u64_hex(Arena *arena, U64 *v, U64 count)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  for (U64 i = 0; i+2 <= count; i += 2) {
    str8_list_pushf(scratch.arena, &list, "[%#llx, %#llx)", v[i+0], v[i+1]);
  }
  String8 result = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return result;
}

internal void
rd_format_preamble(Arena *arena, String8List *out, String8 indent, String8 input_path, String8 raw_data)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 input_type_string = str8_lit("???");
  if (coff_is_regular_archive(raw_data)) {
    input_type_string = str8_lit("Archive");
  } else if (coff_is_thin_archive(raw_data)) {
    input_type_string = str8_lit("Thin Archive");
  } else if (coff_is_big_obj(raw_data)) {
    input_type_string = str8_lit("Big Obj");
  } else if (coff_is_obj(raw_data)) {
    input_type_string = str8_lit("Obj");
  } else if (pe_check_magic(raw_data)) {
    input_type_string = str8_lit("COFF/PE");
  } else if (rd_is_rdi(raw_data)) {
    input_type_string = str8_lit("RDI");
  } else if (elf_check_magic(raw_data)) {
    U8 sig[ELF_Identifier_Max] = {0};
    str8_deserial_read(raw_data, 0, &sig[0], sizeof(sig), 1);
    input_type_string = push_str8f(scratch.arena, "ELF (Class: %S)", elf_string_from_class(scratch.arena, sig[ELF_Identifier_Class]));
  }
  
  DateTime universal_dt = os_now_universal_time();
  DateTime local_dt     = os_local_time_from_universal(&universal_dt);
  String8  time         = push_date_time_string(scratch.arena, &local_dt);
  String8  full_path    = os_full_path_from_path(scratch.arena, input_path);
  rd_printf("# %S, [%S] %S", time, input_type_string, full_path);
  
  scratch_end(scratch);
}

internal int
rd_marker_is_before(void *a, void *b)
{
  return u64_is_before(&((RD_Marker*)a)->off, &((RD_Marker*)b)->off);
}

internal int
rd_range_min_is_before(void *raw_a, void *raw_b)
{
  Rng1U64 *a = raw_a;
  Rng1U64 *b = raw_b;
  return a->min < b->min;
}

internal U64
rd_range_bsearch(Rng1U64 *ranges, U64 count, U64 x)
{
  if (count > 0 && ranges[0].min <= x && x < ranges[count - 1].min) {
    U64 first = 0;
    U64 opl   = count;
    for (;;) {
      U64 mid = (first + opl)/2;
      if (ranges[mid].min < x) {
        first = mid;
      } else if (ranges[mid].min > x) {
        opl = mid;
      } else {
        first = mid;
        break;
      }
      if (opl - first <= 1) {
        break;
      }
    }
    return first;
  }
  return max_U64;
}

internal RD_MarkerArray *
rd_section_markers_from_rdi(Arena *arena, RDI_Parsed *rdi)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64                sect_count = 0;
  RDI_BinarySection *sect_lo    = rdi_table_from_name(rdi, BinarySections, &sect_count);
  
  Rng1U64 *sect_vranges = push_array(scratch.arena, Rng1U64, sect_count);
  for (U64 i = 0; i < sect_count; ++i) {
    sect_vranges[i].min = sect_lo[i].voff_first;
    sect_vranges[i].max = i;
  }
  radsort(sect_vranges, sect_count, rd_range_min_is_before);
  
  U64            proc_count = 0;
  RDI_Procedure *proc_lo    = rdi_table_from_name(rdi, Procedures, &proc_count);
  
  RD_MarkerList *markers = push_array(scratch.arena, RD_MarkerList, sect_count);
  for (U64 i = 0; i < proc_count; ++i) {
    RDI_Procedure *proc         = proc_lo + i;
    U64            proc_voff_lo = rdi_first_voff_from_procedure(rdi, proc);
    U64            proc_voff_hi = rdi_opl_voff_from_procedure(rdi, proc);
    
    U64 sect_range_idx = rd_range_bsearch(sect_vranges, sect_count, proc_voff_lo);
    if (sect_range_idx < sect_count) {
      Rng1U64 sect_range = sect_vranges[sect_range_idx];
      U64     sect_idx   = sect_range.max;
      
      String8 name = str8_zero();
      name.str = rdi_name_from_procedure(rdi, proc, &name.size);
      
      RD_MarkerNode *n = push_array(scratch.arena, RD_MarkerNode, 1);
      n->v.off         = proc_voff_lo - sect_range.min;
      n->v.string      = name;
      
      RD_MarkerList *list = &markers[sect_idx];
      SLLQueuePush(list->first, list->last, n);
      ++list->count;
    }
  }
  
  // lists -> arrays
  RD_MarkerArray *result = push_array(arena, RD_MarkerArray, sect_count);
  for (U64 i = 0; i < sect_count; ++i) {
    result[i].count = 0;
    result[i].v     = push_array(arena, RD_Marker, markers[i].count);
    for (RD_MarkerNode *n = markers[i].first; n != 0; n = n->next) {
      result[i].v[result[i].count++] = n->v;
    }
  }
  
  // sort arrays
  for (U64 i = 0; i < sect_count; ++i) {
    radsort(result[i].v, result[i].count, rd_marker_is_before);
  }
  
  scratch_end(scratch);
  return result;
}

internal RD_MarkerArray *
rd_section_markers_from_coff_symbol_table(Arena *arena, String8 string_table, U64 section_count, COFF_Symbol32Array symbols)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // extract markers from symbol table
  RD_MarkerList *markers = push_array(scratch.arena, RD_MarkerList, section_count+1);
  for (U64 symbol_idx = 0; symbol_idx < symbols.count; ++symbol_idx) {
    COFF_Symbol32 *symbol = &symbols.v[symbol_idx];
    
    COFF_SymbolValueInterpType interp = coff_interp_symbol(symbol->section_number, symbol->value, symbol->storage_class);
    B32 is_marker = interp == COFF_SymbolValueInterp_Regular &&
      symbol->aux_symbol_count == 0 &&
    (symbol->storage_class == COFF_SymStorageClass_External || symbol->storage_class == COFF_SymStorageClass_Static);
    
    if (is_marker) {
      String8 name = coff_read_symbol_name(string_table, &symbol->name);
      
      RD_MarkerNode *n = push_array(scratch.arena, RD_MarkerNode, 1);
      n->v.off         = symbol->value;
      n->v.string      = name;
      
      RD_MarkerList *list = &markers[symbol->section_number];
      SLLQueuePush(list->first, list->last, n);
      ++list->count;
    }
    
    symbol_idx += symbol->aux_symbol_count;
  }
  
  // lists -> arrays
  RD_MarkerArray *result = push_array(arena, RD_MarkerArray, section_count);
  for (U64 i = 0; i < section_count; ++i) {
    result[i].count = 0;
    result[i].v     = push_array(arena, RD_Marker, markers[i].count);
    for (RD_MarkerNode *n = markers[i].first; n != 0; n = n->next) {
      result[i].v[result[i].count++] = n->v;
    }
  }
  
  // sort arrays
  for (U64 i = 0; i < section_count; ++i) {
    radsort(result[i].v, result[i].count, rd_marker_is_before);
  }
  
  scratch_end(scratch);
  return result;
}

internal RD_DisasmResult
rd_disasm_next_instruction(Arena *arena, Arch arch, U64 addr, String8 raw_code)
{
  RD_DisasmResult result = {0};
  
  switch (arch) {
    case Arch_Null: break;
    
    case Arch_x64:
    case Arch_x86: {
      ZydisMachineMode             machine_mode = bit_size_from_arch(arch) == 32 ? ZYDIS_MACHINE_MODE_LEGACY_32 : ZYDIS_MACHINE_MODE_LONG_64;
      ZydisDisassembledInstruction inst         = {0};
      ZyanStatus                   status       = ZydisDisassemble(machine_mode, addr, raw_code.str, raw_code.size, &inst, ZYDIS_FORMATTER_STYLE_INTEL);
      
      String8 text = str8_cstring_capped(inst.text, inst.text+sizeof(inst.text));
      result.text = push_str8_copy(arena, text);
      result.size = inst.info.length;
    } break;
    
    default: NotImplemented;
  }
  
  return result;
}

internal void
rd_print_disasm(Arena            *arena,
                String8List      *out,
                String8           indent,
                Arch              arch,
                U64               image_base,
                U64               sect_off,
                U64               marker_count,
                RD_Marker        *markers,
                String8           raw_code)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64   bytes_buffer_max = 256;
  char *bytes_buffer     = push_array(scratch.arena, char, bytes_buffer_max);
  
  U64     decode_off    = 0;
  U64     marker_cursor = 0;
  String8 to_decode     = raw_code;
  
  for (; to_decode.size > 0; ) {
    Temp temp = temp_begin(scratch.arena);
    
    // decode instruction
    U64             addr          = image_base + sect_off + decode_off;
    RD_DisasmResult disasm_result = rd_disasm_next_instruction(temp.arena, arch, addr, to_decode);
    
    // format instruction bytes
    String8 bytes;
    {
      U64 bytes_size = 0;
      for (U64 i = 0; i < disasm_result.size; ++i) {
        bytes_size += raddbg_snprintf(bytes_buffer + bytes_size, bytes_buffer_max-bytes_size, "%s%02x", i > 0 ? " " : "", to_decode.str[i]);
      }
      bytes = str8((U8*)bytes_buffer, bytes_size);
    }
    
    // print address marker
    if (marker_cursor < marker_count) {
      RD_Marker *m = &markers[marker_cursor];
      // NOTE: markers must be sorted on address
      if (decode_off <= m->off && m->off < decode_off + disasm_result.size) {
        if (m->off != decode_off) {
          U64 off = m->off - decode_off;
          rd_printf("; %S+%#llx", m->string, addr);
        } else {
          rd_printf("; %S", m->string);
        }
        marker_cursor += 1;
      }
    }
    
    // print final line
    rd_printf("%#08x: %-32S %S", addr, bytes, disasm_result.text);
    
    // advance
    to_decode = str8_skip(to_decode, disasm_result.size);
    decode_off += disasm_result.size;
    
    temp_end(temp);
  }
  
  scratch_end(scratch);
}

internal String8
rd_format_hex_array(Arena *arena, U8 *ptr, U64 size)
{
  U64   buf_max  = 32 + size * 8;
  char *buf      = push_array(arena, char, buf_max);
  U64   buf_size = 0;
  
  buf_size += raddbg_snprintf(buf+buf_size, buf_max-buf_size, "{ ");
  for (U64 i = 0; i < size; ++i) {
    buf_size += raddbg_snprintf(buf+buf_size, buf_max-buf_size, "%s0x%02x", i>0 ? ", " : "", ptr[i]);
  }
  buf_size += raddbg_snprintf(buf+buf_size, buf_max-buf_size, " }");
  
  buf[buf_size] = '\0';
  
  String8 result = str8((U8*)buf, buf_size);
  return result;
}

internal void
rd_print_raw_data(Arena       *arena,
                  String8List *out,
                  String8      indent,
                  U64          bytes_per_row,
                  U64          marker_count,
                  RD_Marker   *markers,
                  String8      raw_data)
{
  AssertAlways(bytes_per_row > 0);
  
  char temp_buffer[1024];
  
  String8 to_format = raw_data;
  for (; to_format.size > 0; ) {
    String8 raw_row = str8_prefix(to_format, bytes_per_row);
    
    U64 temp_cursor = 0;
    
    // offset
    U64 offset = (U64)(raw_row.str-raw_data.str);
    temp_cursor += raddbg_snprintf(temp_buffer+temp_cursor, sizeof(temp_buffer)-temp_cursor, "%#08llx: ", offset);
    
    // hex
    for (U64 i = 0; i < raw_row.size; ++i) {
      U8 b = raw_row.str[i];
      temp_cursor += raddbg_snprintf(temp_buffer+temp_cursor, sizeof(temp_buffer)-temp_cursor, "%s%02x", i>0 ? " " : "", b);
    }
    U64 hex_indent_size = (bytes_per_row - raw_row.size) * 3;
    MemorySet(temp_buffer+temp_cursor, ' ', hex_indent_size);
    temp_cursor += hex_indent_size;
    
    temp_cursor += raddbg_snprintf(temp_buffer+temp_cursor, sizeof(temp_buffer) - temp_cursor, " ");
    
    // ascii
    for (U64 i = 0; i < raw_row.size; ++i) {
      U8 b = raw_row.str[i];
      U8 c = b;
      if (c < ' ' || c > '~') {
        c = '.';
      }
      temp_cursor += raddbg_snprintf(temp_buffer+temp_cursor, sizeof(temp_buffer)-temp_cursor, "%c", c);
    }
    
    rd_printf("%.*s", temp_cursor, temp_buffer);
    
    // advance
    to_format = str8_skip(to_format, bytes_per_row);
  }
}

internal String8
str8_from_rdi_string_idx(RDI_Parsed *rdi, U32 idx)
{
  String8 result = str8_zero();
  result.str = rdi_string_from_idx(rdi, idx, &result.size);
  return result;
}

internal String8
rdi_string_from_data_section_kind(Arena *arena, RDI_SectionKind v)
{
  String8 result;
  switch (v) {
    default: result = push_str8f(arena, "<invalid RDI_SectionKind %u>", v); break;
#define X(name, lower, type) case RDI_SectionKind_##name:{result = str8_lit(#name);} break;
    RDI_SectionKind_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_arch(Arena *arena, RDI_Arch v)
{
  String8 result;
  switch (v) {
    default:{ result = push_str8f(arena, "<invalid RDI_Arch %u", v); } break;
#define X(name) case RDI_Arch_##name:{result = str8_lit(#name);} break;
    RDI_Arch_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_language(Arena *arena, RDI_Language v)
{
  String8 result;
  switch (v) {
    default:{ result = push_str8f(arena, "<invalid RDI_Language %u ", v); } break;
#define X(name) case RDI_Language_##name:{result = str8_lit(#name);} break;
    RDI_Language_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_local_kind(Arena *arena, RDI_LocalKind v)
{
  String8 result;
  switch(v)
  {
    default: { result = push_str8f(arena, "<invalid RDI_LocalKind %u>", v); } break;
#define X(name) case RDI_LocalKind_##name:{ result = str8_lit(#name); } break;
    RDI_LocalKind_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_member_kind(Arena *arena, RDI_MemberKind v)
{
  String8 result;
  switch (v) {
    default: { result = push_str8f(arena, "<invalid RDI_MemberKind %u>", v); } break;
#define X(name) case RDI_MemberKind_##name: { result = str8_lit(#name); } break;
    RDI_MemberKind_XList
#undef X
  }
  return result;
}

internal String8
rdi_string_from_reg_code_x86(U64 reg_code)
{
#define X(name, value) case RDI_RegCodeX86_##name: return str8_lit(#name);
  switch (reg_code) {
    RDI_RegCodeX86_XList
  }
#undef X
  return str8_lit("");
}

internal String8
rdi_string_from_reg_code_x64(U64 reg_code)
{
#define X(name, value) case RDI_RegCodeX64_##name: return str8_lit(#name);
  switch (reg_code) {
    RDI_RegCodeX64_XList
  }
#undef X
  return str8_lit("");
}

internal String8
rdi_string_from_reg_code(Arena *arena, RDI_Arch arch, U64 reg_code)
{
  switch (arch) {
    case RDI_Arch_NULL: break;
    case RDI_Arch_X86: return rdi_string_from_reg_code_x86(reg_code);
    case RDI_Arch_X64: return rdi_string_from_reg_code_x64(reg_code);
    default: InvalidPath;
  }
  return push_str8f(arena, "??? (%llu)", reg_code);
}

internal String8
rdi_string_from_eval_op(Arena *arena, RDI_EvalOp op)
{
  switch (op) {
    case RDI_EvalOp_Stop:            return str8_lit("Stop");
    case RDI_EvalOp_Noop:            return str8_lit("Noop");
    case RDI_EvalOp_Cond:            return str8_lit("Cond");
    case RDI_EvalOp_Skip:            return str8_lit("Skip");
    case RDI_EvalOp_MemRead:         return str8_lit("MemRead");
    case RDI_EvalOp_RegRead:         return str8_lit("RegRead");
    case RDI_EvalOp_RegReadDyn:      return str8_lit("RegReadDyn");
    case RDI_EvalOp_FrameOff:        return str8_lit("FrameOff");
    case RDI_EvalOp_ModuleOff:       return str8_lit("ModuleOff");
    case RDI_EvalOp_TLSOff:          return str8_lit("TLSOff");
    case RDI_EvalOp_ObjectOff:       return str8_lit("ObjectOff");
    case RDI_EvalOp_CFA:             return str8_lit("CFA");
    case RDI_EvalOp_ConstU8:         return str8_lit("ConstU8");
    case RDI_EvalOp_ConstU16:        return str8_lit("ConstU16");
    case RDI_EvalOp_ConstU32:        return str8_lit("ConstU32");
    case RDI_EvalOp_ConstU64:        return str8_lit("ConstU64");
    case RDI_EvalOp_ConstU128:       return str8_lit("ConstU128");
    case RDI_EvalOp_ConstString:     return str8_lit("ConstString");
    case RDI_EvalOp_Abs:             return str8_lit("Abs");
    case RDI_EvalOp_Neg:             return str8_lit("Neg");
    case RDI_EvalOp_Add:             return str8_lit("Add");
    case RDI_EvalOp_Sub:             return str8_lit("Sub");
    case RDI_EvalOp_Mul:             return str8_lit("Mul");
    case RDI_EvalOp_Div:             return str8_lit("Div");
    case RDI_EvalOp_Mod:             return str8_lit("Mod");
    case RDI_EvalOp_LShift:          return str8_lit("LShift");
    case RDI_EvalOp_RShift:          return str8_lit("RShift");
    case RDI_EvalOp_BitAnd:          return str8_lit("BitAnd");
    case RDI_EvalOp_BitOr:           return str8_lit("BitOr");
    case RDI_EvalOp_BitXor:          return str8_lit("BitXor");
    case RDI_EvalOp_BitNot:          return str8_lit("BitNot");
    case RDI_EvalOp_LogAnd:          return str8_lit("LogAnd");
    case RDI_EvalOp_LogOr:           return str8_lit("LogOr");
    case RDI_EvalOp_LogNot:          return str8_lit("LogNot");
    case RDI_EvalOp_EqEq:            return str8_lit("EqEq");
    case RDI_EvalOp_NtEq:            return str8_lit("NtEq");
    case RDI_EvalOp_LsEq:            return str8_lit("LsEq");
    case RDI_EvalOp_GrEq:            return str8_lit("GrEq");
    case RDI_EvalOp_Less:            return str8_lit("Less");
    case RDI_EvalOp_Grtr:            return str8_lit("Grtr");
    case RDI_EvalOp_Trunc:           return str8_lit("Trunc");
    case RDI_EvalOp_TruncSigned:     return str8_lit("TruncSigned");
    case RDI_EvalOp_Convert:         return str8_lit("Convert");
    case RDI_EvalOp_Pick:            return str8_lit("Pick");
    case RDI_EvalOp_Pop:             return str8_lit("Pop");
    case RDI_EvalOp_Insert:          return str8_lit("Insert");
    case RDI_EvalOp_ValueRead:       return str8_lit("ValueRead");
    case RDI_EvalOp_ByteSwap:        return str8_lit("ByteSwap");
    case RDI_EvalOp_CallSiteValue:   return str8_lit("CallSiteValue");
    case RDI_EvalOp_PartialValue:    return str8_lit("PartialValue");
    case RDI_EvalOp_PartialValueBit: return str8_lit("PartialValueBit");
  }
  return push_str8f(arena, "%#x", op);
}

internal String8
rdi_string_from_eval_type_group(Arena *arena, RDI_EvalTypeGroup x)
{
  switch (x) {
    case RDI_EvalTypeGroup_Other: return str8_lit("Other");
    case RDI_EvalTypeGroup_U:     return str8_lit("U");
    case RDI_EvalTypeGroup_S:     return str8_lit("S");
    case RDI_EvalTypeGroup_F32:   return str8_lit("F32");
    case RDI_EvalTypeGroup_F64:   return str8_lit("F64");
  }
  return push_str8f(arena, "%#x", x);
}

internal String8
rdi_string_from_binary_section_flags(Arena *arena, RDI_BinarySectionFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if (flags & RDI_BinarySectionFlag_##name) { flags &= ~RDI_BinarySectionFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_BinarySectionFlags_XList;
#undef X
  String8 result = rd_string_from_flags(arena, list, flags);
  scratch_end(scratch);
  return result;
}

internal String8
rdi_string_from_type_modifier_flags(Arena *arena, RDI_TypeModifierFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if(flags & RDI_TypeModifierFlag_##name) { flags &= ~RDI_TypeModifierFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_TypeModifierFlags_XList;
#undef X
  String8 result = rd_string_from_flags(arena, list, flags);
  scratch_end(scratch);
  return result;
}

internal String8
rdi_string_from_udt_flags(Arena *arena, RDI_UDTFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if (flags & RDI_UDTFlag_##name) { flags &= ~RDI_UDTFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_UDTFlags_XList;
#undef X
  String8 result = rd_string_from_flags(arena, list, flags);
  scratch_end(scratch);
  return result;
}

internal String8
rdi_string_from_link_flags(Arena *arena, RDI_LinkFlags flags)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
#define X(name) if (flags & RDI_LinkFlag_##name) { flags &= ~RDI_LinkFlag_##name; str8_list_push(scratch.arena, &list, str8_lit(#name)); }
  RDI_LinkFlags_XList;
#undef X
  String8 result = rd_string_from_flags(arena, list, flags);
  scratch_end(scratch);
  return result;
}

internal void
rdi_print_data_sections(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi)
{
  Temp scratch = scratch_begin(&arena, 1);
  for(U64 idx = 0; idx < rdi->sections_count; idx += 1) {
    RDI_SectionKind  kind     = (RDI_SectionKind)idx;
    RDI_Section     *section  = &rdi->sections[idx];
    String8          kind_str = rdi_string_from_data_section_kind(scratch.arena, kind);
    rd_printf("data_section[%5llu] = {%#08llx, %7u, %7u}, %S", idx, section->off, section->encoded_size, section->unpacked_size, kind_str);
  }
  scratch_end(scratch);
}

internal void
rdi_print_top_level_info(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_TopLevelInfo *tli)
{
  Temp scratch = scratch_begin(&arena, 1);
  rd_printf("arch         =%S",      rdi_string_from_arch(scratch.arena, tli->arch));
  rd_printf("exe_name     ='%S'",    str8_from_rdi_string_idx(rdi, tli->exe_name_string_idx));
  rd_printf("voff_max     =%#08llx", tli->voff_max);
  rd_printf("producer_name='%S'",    str8_from_rdi_string_idx(rdi, tli->producer_name_string_idx));
  scratch_end(scratch);
}

internal void
rdi_print_binary_section(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_BinarySection *bin_section)
{
  Temp scratch = scratch_begin(&arena, 1);
  rd_printf("name      ='%S'",  str8_from_rdi_string_idx(rdi, bin_section->name_string_idx));
  rd_printf("flags     =%S",    rdi_string_from_binary_section_flags(scratch.arena, bin_section->flags));
  rd_printf("voff_first=%#08x", bin_section->voff_first);
  rd_printf("voff_opl  =%#08x", bin_section->voff_opl);
  rd_printf("foff_first=%#08x", bin_section->foff_first);
  rd_printf("foff_opl  =%#08x", bin_section->foff_opl);
  scratch_end(scratch);
}

internal void
rdi_print_file_path(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_FilePathNode *file_path)
{
  U64               file_path_count = 0;
  RDI_FilePathNode *file_path_array = rdi_table_from_name(rdi, FilePathNodes, &file_path_count);
  
  String8 name     = str8_from_rdi_string_idx(rdi, file_path->name_string_idx);
  U64     this_idx = (U64)(file_path - file_path_array);
  
  if (file_path->source_file_idx == 0) {
    rd_printf("[%llu] '%S'", this_idx, name);
  } else {
    rd_printf("[%llu] '%S'; source_file=%u", this_idx, name, file_path->source_file_idx);
  }
  
  for (U32 child = file_path->first_child; child != 0; ) {
    // get node for child
    RDI_FilePathNode *child_node = 0;
    if (child < file_path_count) {
      child_node = file_path_array + child;
    }
    if (child_node == 0) {
      break;
    }
    
    // stringize child
    rd_indent();
    rdi_print_file_path(arena, out, indent, rdi, child_node);
    rd_unindent();
    
    // increment iterator
    child = child_node->next_sibling;
  }
}

internal void
rdi_print_source_file(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_SourceFile *source_file)
{
  rd_printf("{ file_path_node_idx= ");
  rd_printf("file_path_node_idx= %u",   source_file->file_path_node_idx);
  rd_printf("path=               '%S'", str8_from_rdi_string_idx(rdi, source_file->normal_full_path_string_idx));
  rd_printf("source_line_map=    %u",   source_file->source_line_map_idx);
}

internal void
rdi_print_line_table(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_LineTable *line_table)
{
  RDI_ParsedLineTable parsed_line_table = {0};
  rdi_parsed_from_line_table(rdi, line_table, &parsed_line_table);
  
  rd_printf("lines:");
  for (U32 i = 0; i < parsed_line_table.count; i += 1) {
    U64         first = parsed_line_table.voffs[i];
    U64         opl   = parsed_line_table.voffs[i + 1];
    RDI_Line   *line  = parsed_line_table.lines + i;
    RDI_Column *col   = 0;
    if (i < parsed_line_table.col_count) {
      col = parsed_line_table.cols + i;
    }
    if (col == 0) {
      rd_printf("[0x%08llx,0x%08llx) file=%u; line=%u", first, opl, line->file_idx, line->line_num);
    } else {
      rd_printf("[0x%08llx,0x%08llx) file=%u; line=%u; columns=[%u,%u)", first, opl, line->file_idx, line->line_num, col->col_first, col->col_opl);
    }
  }
}

internal void
rdi_print_source_line_map(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_SourceLineMap *map)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  RDI_ParsedSourceLineMap line_map = {0};
  rdi_parsed_from_source_line_map(rdi, map, &line_map);
  
  rd_printf("source lines:");
  for (U32 line_num_idx = 0; line_num_idx < line_map.count; ++line_num_idx) {
    Temp temp = temp_begin(scratch.arena);
    
    String8List list = {0};
    U32 voff_lo = line_map.ranges[line_num_idx];
    U32 voff_hi = ClampTop(line_map.ranges[line_num_idx + 1], line_map.voff_count);
    for (U64 voff_idx = voff_lo; voff_idx < voff_hi; ++voff_idx) {
      str8_list_pushf(temp.arena, &list, "%#llx", line_map.voffs[voff_idx]);
    }
    
    String8 voffs = str8_list_join(temp.arena, &list, &(StringJoin){.sep=str8_lit(", ")});
    rd_printf("%u: %S", line_map.nums[line_num_idx], voffs);
    
    temp_end(temp);
  }
  
  scratch_end(scratch);
}

internal void
rdi_print_unit(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_Unit *unit)
{
  Temp scratch = scratch_begin(&arena, 1);
  rd_printf("unit_name        ='%S'", str8_from_rdi_string_idx(rdi, unit->unit_name_string_idx));
  rd_printf("compiler_name    ='%S'", str8_from_rdi_string_idx(rdi, unit->compiler_name_string_idx));
  rd_printf("source_file_path =%u",   unit->source_file_path_node);
  rd_printf("object_file_path =%u",   unit->object_file_path_node);
  rd_printf("archive_file_path=%u",   unit->archive_file_path_node);
  rd_printf("build_path       =%u",   unit->build_path_node);
  rd_printf("language         =%S",   rdi_string_from_language(scratch.arena, unit->language));
  rd_printf("line_table_idx   =%u",   unit->line_table_idx);
  scratch_end(scratch);
}

internal void
rdi_print_type_node(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_TypeNode *type)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 type_kind_str = {0};
  type_kind_str.str = rdi_string_from_type_kind(type->kind, &type_kind_str.size);
  
  rd_printf("kind                    =%S", type_kind_str);
  if (type->kind == RDI_TypeKind_Modifier) {
    rd_printf("flags                   =%S", rdi_string_from_type_modifier_flags(scratch.arena, type->flags));
  } else {
    if (type->flags != 0) {
      rd_printf("flags=%#x (missing stringizer path)", type->flags);
    }
  }
  rd_printf("byte_size               =%u", type->byte_size);
  if (RDI_TypeKind_FirstBuiltIn <= type->kind && type->kind <= RDI_TypeKind_LastBuiltIn) {
    rd_printf("built_in.name           ='%S'", str8_from_rdi_string_idx(rdi, type->built_in.name_string_idx));
  } else if (type->kind == RDI_TypeKind_Array) {
    rd_printf("constructed.direct_type =%u", type->constructed.direct_type_idx);
    rd_printf("constructed.array_count =%u", type->constructed.count);
  } else if (type->kind == RDI_TypeKind_Function) {
    U32  param_idx_count = 0;
    U32 *param_idx_array = rdi_idx_run_from_first_count(rdi, type->constructed.param_idx_run_first, type->constructed.count, &param_idx_count);
    String8 param_idx_str = rd_string_from_array_u32(scratch.arena, param_idx_array, param_idx_count);
    rd_printf("constructed.params      =%S", param_idx_str);
    rd_printf("return type             =%u", type->constructed.direct_type_idx);
  } else if (type->kind == RDI_TypeKind_Method) {
    U32  param_idx_count = 0;
    U32 *param_idx_array = rdi_idx_run_from_first_count(rdi, type->constructed.param_idx_run_first, type->constructed.count, &param_idx_count);
    String8 this_type_str = str8_lit("\?\?\?");
    if (param_idx_count > 0) {
      this_type_str = push_str8f(scratch.arena, "%u", param_idx_array[0]);
      param_idx_count -= 1;
      param_idx_array += 1;
    }
    String8 param_idx_str = rd_string_from_array_u32(scratch.arena, param_idx_array, param_idx_count);
    rd_printf("constructed.this_type   =%S", this_type_str);
    rd_printf("constructed.params      =%S", param_idx_str);
    rd_printf("return type             =%u", type->constructed.direct_type_idx);
  } else if (RDI_TypeKind_FirstConstructed <= type->kind && type->kind <= RDI_TypeKind_LastConstructed) {
    
    rd_printf("constructed.direct_type =%u", type->constructed.direct_type_idx);
  } else if (RDI_TypeKind_FirstUserDefined <= type->kind && type->kind <= RDI_TypeKind_LastUserDefined){
    rd_printf("user_defined.name       ='%S'", str8_from_rdi_string_idx(rdi, type->user_defined.name_string_idx));
    rd_printf("user_defined.direct_type=%u",   type->user_defined.direct_type_idx);
    rd_printf("user_defined.udt        =%u",   type->user_defined.udt_idx);
  } else if (type->kind == RDI_TypeKind_Bitfield) {
    rd_printf("bitfield.off            =%u", type->bitfield.off);
    rd_printf("bitfield.size           =%u", type->bitfield.size);
  }
  
  scratch_end(scratch);
}

internal void
rdi_print_udt(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_UDT *udt)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64             member_count      = 0;
  U64             enum_member_count = 0;
  RDI_Member     *member_array      = rdi_table_from_name(rdi, Members,     &member_count);
  RDI_EnumMember *enum_member_array = rdi_table_from_name(rdi, EnumMembers, &enum_member_count);
  
  String8 flags_str = rdi_string_from_udt_flags(scratch.arena, udt->flags);
  
  rd_printf("self_type=%u", udt->self_type_idx);
  rd_printf("flags    =%S", flags_str);
  if (udt->file_idx != 0) {
    rd_printf("loc      ={file=%u; line=%u; col=%u}", udt->file_idx, udt->line, udt->col);
  }
  
  // enum members
  if (udt->flags & RDI_UDTFlag_EnumMembers) {
    U32 member_hi = ClampTop(udt->member_first + udt->member_count, enum_member_count);
    U32 member_lo = ClampTop(udt->member_first, member_hi);
    if (member_lo < member_hi) {
      rd_printf("members={");
      rd_indent();
      RDI_EnumMember *enum_member = enum_member_array + member_lo;
      for (U32 i = member_lo; i < member_hi; ++i, ++enum_member) {
        rd_printf("{ %llu, '%S' }", enum_member->val, str8_from_rdi_string_idx(rdi, enum_member->name_string_idx));
      }
      rd_unindent();
      rd_printf("}");
    }
  }
  // field members
  else {
    U32 member_hi = ClampTop(udt->member_first + udt->member_count, member_count);
    U32 member_lo = ClampTop(udt->member_first, member_hi);
    if (member_lo < member_hi) {
      rd_printf("members={");
      rd_indent();
      RDI_Member *member = member_array + member_lo;
      for (U32 i = member_lo; i < member_hi; ++i, ++member) {
        String8 kind_str = rdi_string_from_member_kind(scratch.arena, member->kind);
        String8 name_str = str8_from_rdi_string_idx(rdi, member->name_string_idx);
        rd_printf("{ kind=%S, type=%u, off=%u, name='%S' }", kind_str, member->type_idx, member->off, name_str);
      }
      rd_unindent();
      rd_printf("}");
    }
  }
  scratch_end(scratch);
}

internal String8
rdi_string_from_bytecode(Arena *arena, RDI_Arch arch, String8 bc)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8List fmt = {0};
  for (U64 cursor = 0; cursor < bc.size; ) {
    RDI_EvalOp op = RDI_EvalOp_Stop;
    cursor += str8_deserial_read_struct(bc, cursor, &op);
    
    U16 ctrlbits = rdi_eval_op_ctrlbits_table[op];
    U32 imm_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
    
    String8 imm = {0};
    cursor += str8_deserial_read_block(bc, cursor, imm_size, &imm);
    if (imm.size != imm_size) {
      str8_list_pushf(scratch.arena, &fmt, "(ERROR: not enough bytes to read immediate)");
      break;
    }
    
    String8 imm_fmt = {0};
    switch (op) {
      case RDI_EvalOp_Stop: goto exit;
      case RDI_EvalOp_Noop: break;
      case RDI_EvalOp_Cond: break;
      case RDI_EvalOp_Skip:  {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U16 *)imm.str);
      } break;
      case RDI_EvalOp_MemRead: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U8 *)imm.str);
      } break;
      case RDI_EvalOp_RegRead: {
        U32         regread   = *(U32 *)imm.str;
        RDI_RegCode reg_code  = Extract8(regread, 0);
        U8          byte_size = Extract8(regread, 1);
        U8          byte_off  = Extract8(regread, 2);
        String8     reg_str   = rdi_string_from_reg_code(scratch.arena, arch, reg_code);
        imm_fmt = push_str8f(scratch.arena, "%S, Size: %u", rd_string_from_reg_off(scratch.arena, reg_str, byte_off), byte_size);
      } break;
      case RDI_EvalOp_RegReadDyn: break;
      case RDI_EvalOp_FrameOff: {
        imm_fmt = push_str8f(scratch.arena, "%+lld", *(S64 *)imm.str);
      } break;
      case RDI_EvalOp_ModuleOff: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U32 *)imm.str);
      } break;
      case RDI_EvalOp_TLSOff: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U32 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU8: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U8 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU16: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U16 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU32: {
        imm_fmt = push_str8f(scratch.arena, "%u", *(U32 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU64: {
        imm_fmt = push_str8f(scratch.arena, "%llu", *(U64 *)imm.str);
      } break;
      case RDI_EvalOp_ConstU128: {
        imm_fmt = push_str8f(scratch.arena, "Lo: %llu, Hi: %llu", *(U64 *)imm.str, *((U64 *)imm.str + 1));
      } break;
      case RDI_EvalOp_ConstString: {
        U8      size   = *(U8 *)imm.str;
        String8 string = {0};
        cursor += str8_deserial_read_block(bc, cursor, size, &string);
        
        imm_fmt = push_str8f(scratch.arena, "(%u) \"%S\"", size, string);
      } break;
      case RDI_EvalOp_Abs:
      case RDI_EvalOp_Neg: 
      case RDI_EvalOp_Add:
      case RDI_EvalOp_Sub:
      case RDI_EvalOp_Mul:
      case RDI_EvalOp_Div:
      case RDI_EvalOp_Mod:
      case RDI_EvalOp_LShift:
      case RDI_EvalOp_RShift:
      case RDI_EvalOp_BitAnd:
      case RDI_EvalOp_BitOr:
      case RDI_EvalOp_BitXor:
      case RDI_EvalOp_BitNot:
      case RDI_EvalOp_LogAnd:
      case RDI_EvalOp_LogOr:
      case RDI_EvalOp_LogNot:
      case RDI_EvalOp_EqEq:
      case RDI_EvalOp_NtEq:
      case RDI_EvalOp_LsEq:
      case RDI_EvalOp_GrEq:
      case RDI_EvalOp_Less:
      case RDI_EvalOp_Grtr: {
        U8 eval_type_group = *(U8 *)imm.str;
        imm_fmt = rdi_string_from_eval_type_group(scratch.arena, eval_type_group);
      } break;
      case RDI_EvalOp_Trunc:
      case RDI_EvalOp_TruncSigned: {
        U8 trunc = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", trunc);
      } break;
      case RDI_EvalOp_Convert: {
        U16 convert = *(U16 *)imm.str;
        U8 in  = Extract8(convert, 0);
        U8 out = Extract8(convert, 1);
        String8 in_str  = rdi_string_from_eval_type_group(scratch.arena, in);
        String8 out_str = rdi_string_from_eval_type_group(scratch.arena, out);
        imm_fmt = push_str8f(scratch.arena, "in: %S out: %S", in_str, out_str);
      } break;
      case RDI_EvalOp_Pick: {
        U8 pick = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", pick);
      } break;
      case RDI_EvalOp_Pop: break;
      case RDI_EvalOp_Insert: {
        U8 insert = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", insert);
      } break;
      case RDI_EvalOp_ValueRead: {
        U8 bytes_to_read = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", bytes_to_read);
      } break;
      case RDI_EvalOp_ByteSwap: {
        U8 byte_size = *(U8 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", byte_size);
      } break;
      case RDI_EvalOp_CallSiteValue: {
        U32     call_site_bc_size = *(U32 *)imm.str;
        String8 call_site_bc      = {0};
        cursor += str8_deserial_read_block(bc, cursor, call_site_bc_size, &call_site_bc);
        
        String8 call_site_str = rdi_string_from_bytecode(scratch.arena, arch, call_site_bc);
        imm_fmt = push_str8f(scratch.arena, "%S", call_site_str);
      } break;
      case RDI_EvalOp_PartialValue: {
        U32 partial_value_size = *(U32 *)imm.str;
        imm_fmt = push_str8f(scratch.arena, "%u", partial_value_size);
      } break;
      case RDI_EvalOp_PartialValueBit: {
        U64 partial_value = *(U64 *)imm.str;
        U32 bit_size = Extract32(partial_value, 0);
        U32 bit_off  = Extract32(partial_value, 1);
        imm_fmt = push_str8f(scratch.arena, "Off: %u, Size: %u", bit_size, bit_off);
      } break;
    }
    
    String8 op_str = rdi_string_from_eval_op(scratch.arena, op);
    if (imm_fmt.size) {
      str8_list_pushf(scratch.arena, &fmt, "RDI_EvalOp_%S(%S)", op_str, imm_fmt);
    } else {
      str8_list_pushf(scratch.arena, &fmt, "RDI_EvalOp_%S", op_str);
    }
  }
  exit:;
  
  String8 result = str8_list_join(arena, &fmt, &(StringJoin){.sep = str8_lit(", ")});
  
  scratch_end(scratch);
  return result;
}

internal void
rdi_print_locations(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_Arch arch, U64 block_lo, U64 block_hi)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64                location_block_count = 0;
  U64                location_data_size   = 0;
  RDI_LocationBlock *location_block_array = rdi_table_from_name(rdi, LocationBlocks, &location_block_count);
  RDI_U8            *location_data        = rdi_table_from_name(rdi, LocationData,   &location_data_size);
  
  block_lo = ClampTop(block_lo, location_block_count);
  block_hi = ClampTop(block_hi, location_block_count);
  
  for (U32 block_idx = block_lo; block_idx < block_hi; ++block_idx) {
    RDI_LocationBlock *block_ptr = &location_block_array[block_idx];
    
    String8List fmt = {0};
    
    if (block_ptr->scope_off_first == 0 && block_ptr->scope_off_opl == max_U32) {
      str8_list_pushf(scratch.arena, &fmt, "*always*:");
    } else {
      str8_list_pushf(scratch.arena, &fmt, "[%#08x, %#08x):", block_ptr->scope_off_first, block_ptr->scope_off_opl);
    }
    
    if (block_ptr->location_data_off >= location_data_size) {
      str8_list_pushf(scratch.arena, &fmt, "<bad-location-data-offset %x>", block_ptr->location_data_off);
    } else {
      U8               *loc_data_opl = location_data + location_data_size;
      U8               *loc_base_ptr = location_data + block_ptr->location_data_off;
      RDI_LocationKind  kind         = *(RDI_LocationKind*)loc_base_ptr;
      switch (kind) {
        default: {
          str8_list_pushf(scratch.arena, &fmt, "\?\?\?: %u", kind);
        } break;
        
        case RDI_LocationKind_AddrBytecodeStream: {
          String8 bc     = str8_range(loc_base_ptr + 1, loc_data_opl);
          String8 bc_str = rdi_string_from_bytecode(scratch.arena, arch, bc);
          str8_list_pushf(scratch.arena, &fmt, "AddrBytecodeStream(%S)", bc_str);
        } break;
        
        case RDI_LocationKind_ValBytecodeStream: {
          String8 bc     = str8_range(loc_base_ptr + 1, loc_data_opl);
          String8 bc_str = rdi_string_from_bytecode(scratch.arena, arch, bc);
          str8_list_pushf(scratch.arena, &fmt, "ValBytecodeStream(%S)", bc_str);
        } break;
        
        case RDI_LocationKind_AddrRegPlusU16: {
          if (loc_base_ptr + sizeof(RDI_LocationRegPlusU16) > loc_data_opl) {
            str8_list_pushf(scratch.arena, &fmt, "AddrRegPlusU16(\?\?\?)");
          } else {
            RDI_LocationRegPlusU16 *loc = (RDI_LocationRegPlusU16*)loc_base_ptr;
            str8_list_pushf(scratch.arena, &fmt, "AddrRegPlusU16(reg: %S, off: %u)", rdi_string_from_reg_code(scratch.arena, arch, loc->reg_code), loc->offset);
          }
        } break;
        
        case RDI_LocationKind_AddrAddrRegPlusU16: {
          if (loc_base_ptr + sizeof(RDI_LocationRegPlusU16) > loc_data_opl){
            str8_list_pushf(scratch.arena, &fmt, "AddrAddrRegPlusU16(\?\?\?)");
          } else {
            RDI_LocationRegPlusU16 *loc = (RDI_LocationRegPlusU16 *)loc_base_ptr;
            str8_list_pushf(scratch.arena, &fmt, "AddrAddrRegisterPlusU16(reg: %S, off: %u)", rdi_string_from_reg_code(scratch.arena, arch, loc->reg_code), loc->offset);
          }
        } break;
        
        case RDI_LocationKind_ValReg: {
          if (loc_base_ptr + sizeof(RDI_LocationReg) > loc_data_opl) {
            str8_list_pushf(scratch.arena, &fmt, "ValReg(\?\?\?)");
          } else {
            RDI_LocationReg *loc = (RDI_LocationReg*)loc_base_ptr;
            str8_list_pushf(scratch.arena, &fmt, "ValReg(reg: %S)", rdi_string_from_reg_code(scratch.arena, arch, loc->reg_code));
          }
        } break;
      }
    }
    
    String8 print_string = str8_list_join(scratch.arena, &fmt, &(StringJoin){.sep=str8_lit(" ")});
    rd_printf("%S", print_string);
  }
  
  scratch_end(scratch);
}

internal void
rdi_print_global_variable(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_GlobalVariable *gvar)
{
  Temp scratch = scratch_begin(&arena, 1);
  rd_printf("name         ='%S'",  str8_from_rdi_string_idx(rdi, gvar->name_string_idx));
  rd_printf("link_flags   =%S",    rdi_string_from_link_flags(scratch.arena, gvar->link_flags));
  rd_printf("voff         =%#08x", gvar->voff);
  rd_printf("type_idx     =%u",    gvar->type_idx);
  rd_printf("container_idx=%u",    gvar->container_idx);
  scratch_end(scratch);
}

internal void
rdi_print_thread_variable(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_ThreadVariable *tvar)
{
  Temp scratch = scratch_begin(&arena, 1);
  rd_printf("name         ='%S'",  str8_from_rdi_string_idx(rdi, tvar->name_string_idx));
  rd_printf("link_flags   =%S",    rdi_string_from_link_flags(scratch.arena, tvar->link_flags));
  rd_printf("tls_off      =%#08x", tvar->tls_off);
  rd_printf("type_idx     =%u",    tvar->type_idx);
  rd_printf("container_idx=%u",    tvar->container_idx);
  scratch_end(scratch);
}

internal void
rdi_print_constant(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_Constant *constant)
{
  rd_printf("name     ='%S'", str8_from_rdi_string_idx(rdi, constant->name_string_idx));
  rd_printf("type_idx ='%u'", constant->type_idx);
}

internal void
rdi_print_procedure(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_Procedure *proc, RDI_Arch arch)
{
  Temp scratch = scratch_begin(&arena, 1);
  rd_printf("name           ='%S'", str8_from_rdi_string_idx(rdi, proc->name_string_idx));
  rd_printf("link_name      ='%S'", str8_from_rdi_string_idx(rdi, proc->link_name_string_idx));
  rd_printf("link_flags     =%S",   rdi_string_from_link_flags(scratch.arena, proc->link_flags));
  rd_printf("type_idx       =%u",   proc->type_idx);
  rd_printf("root_scope_idx =%u",   proc->root_scope_idx);
  rd_printf("container_idx  =%u",   proc->container_idx);
  rd_printf("frame_base (first=%u, opl=%u)", proc->frame_base_location_first, proc->frame_base_location_opl);
  rd_indent();
  rdi_print_locations(arena, out, indent, rdi, arch, proc->frame_base_location_first, proc->frame_base_location_opl);
  rd_unindent();
  
  scratch_end(scratch);
}

internal void
rdi_print_scope(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RDI_Scope *scope, RDI_Arch arch)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64                scope_count          = 0;
  U64                scope_voff_count     = 0;
  U64                local_count          = 0;
  U64                proc_count           = 0;
  U64                inline_site_count    = 0;
  RDI_Scope         *scope_array          = rdi_table_from_name(rdi, Scopes,         &scope_count);
  U64               *scope_voff_array     = rdi_table_from_name(rdi, ScopeVOffData,  &scope_voff_count);
  RDI_Local         *local_array          = rdi_table_from_name(rdi, Locals,         &local_count);
  RDI_Procedure     *proc_array           = rdi_table_from_name(rdi, Procedures,     &proc_count);
  RDI_InlineSite    *inline_site_array    = rdi_table_from_name(rdi, InlineSites,    &inline_site_count);
  
  U32      voff_range_lo    = ClampTop(scope->voff_range_first, scope_voff_count);
  U32      voff_range_hi    = ClampTop(scope->voff_range_opl,   scope_voff_count);
  U32      voff_range_count = (voff_range_hi - voff_range_lo);
  U64     *voff_ptr         = scope_voff_array + voff_range_lo;
  String8  voff_str         = rd_string_from_range_array_u64_hex(scratch.arena, voff_ptr, voff_range_count);
  
  U64 scope_idx = (U64)(scope - scope_array);
  rd_printf("scope[%llu]", scope_idx);
  rd_indent();
  
  String8 proc_name = str8_lit("???");
  if (scope->proc_idx < proc_count) {
    RDI_Procedure *proc = &proc_array[scope->proc_idx];
    proc_name           = str8_from_rdi_string_idx(rdi, proc->name_string_idx);
  }
  
  String8 inline_site_name = str8_lit("");
  if (scope->inline_site_idx != 0) {
    if (scope->inline_site_idx < inline_site_count) {
      RDI_InlineSite *inline_site = &inline_site_array[scope->inline_site_idx];
      inline_site_name = str8_from_rdi_string_idx(rdi, inline_site->name_string_idx);
    } else {
      inline_site_name = str8_lit("???");
    }
    inline_site_name = push_str8f(scratch.arena, " '%S'", inline_site_name);
  }
  
  rd_printf("proc_idx              =%u '%S'", scope->proc_idx, proc_name);
  rd_printf("first_child_scope_idx =%u",      scope->first_child_scope_idx);
  rd_printf("next_sibling_scope_idx=%u",      scope->next_sibling_scope_idx);
  rd_printf("inline_site_idx       =%u%S",    scope->inline_site_idx, inline_site_name);
  rd_printf("voff_ranges           =%S",      voff_str);
  
  // local_array
  {
    U32 local_lo = ClampTop(scope->local_first, local_count);
    U32 local_hi = ClampTop(local_lo + scope->local_count, local_count);
    if (local_lo < local_hi) {
      for (U32 local_idx = local_lo; local_idx < local_hi; ++local_idx) {
        RDI_Local *local_ptr = &local_array[local_idx];
        
        rd_printf("local[%u]", local_idx);
        rd_indent();
        rd_printf("kind    =%S",   rdi_string_from_local_kind(scratch.arena, local_ptr->kind));
        rd_printf("name    ='%S'", str8_from_rdi_string_idx(rdi, local_ptr->name_string_idx));
        rd_printf("type_idx=%u",   local_ptr->type_idx);
        
        if (local_ptr->location_first < local_ptr->location_opl) {
          rd_printf("locations:");
          rd_indent();
          rdi_print_locations(arena, out, indent, rdi, arch, local_ptr->location_first, local_ptr->location_opl);
          rd_unindent();
        }
        rd_unindent();
      }
    }
  }
  
  for (U32 child_scope_idx = scope->first_child_scope_idx; child_scope_idx != 0; ) {
    if (child_scope_idx >= scope_count) {
      rd_errorf("child scope index (%u) is out of bounds", child_scope_idx);
      break;
    }
    
    rd_indent();
    RDI_Scope *child_scope = scope_array + child_scope_idx;
    rdi_print_scope(arena, out, indent, rdi, child_scope, arch);
    rd_unindent();
    
    // advance to next child
    child_scope_idx = child_scope->next_sibling_scope_idx;
  }
  
  rd_unindent();
  rd_printf("[/%llu]", scope_idx);
  
  scratch_end(scratch);
}

internal void
rdi_print_inline_site(Arena          *arena,
                      String8List    *out,
                      String8         indent,
                      RDI_Parsed     *rdi,
                      U64             idx,
                      RDI_InlineSite *inline_site)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 inline_site_idx = push_str8f(scratch.arena, "inline_site[%u]",      idx);
  String8 type_idx        = push_str8f(scratch.arena, "type_idx = %u,",       inline_site->type_idx);
  String8 owner_type_idx  = push_str8f(scratch.arena, "owner_type_idx = %u,", inline_site->owner_type_idx);
  String8 line_table_idx  = push_str8f(scratch.arena, "line_table_idx = %u,", inline_site->line_table_idx);
  rd_printf("%-20S = { %-25S %-25S %-25S name = '%-20S' }",
            inline_site_idx,
            type_idx,
            owner_type_idx,
            line_table_idx,
            str8_from_rdi_string_idx(rdi, inline_site->name_string_idx));
  scratch_end(scratch);
}

internal void
rdi_print_vmap_entry(Arena *arena, String8List *out, String8 indent, RDI_VMapEntry *v)
{
  rd_printf("%#llx: %llu", v->voff, v->idx);
}

internal void
rdi_print(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, RD_Option opts)
{
  RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
  
  if (opts & RD_Option_RdiDataSections) {
    rd_printf("# DATA SECTIONS");
    rd_indent();
    rdi_print_data_sections(arena, out, indent, rdi);
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiTopLevelInfo) {
    rd_printf("# TOP LEVEL INFO");
    rd_indent();
    rdi_print_top_level_info(arena, out, indent, rdi, tli);
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiBinarySections) {
    U64 count;
    RDI_BinarySection *v = rdi_table_from_name(rdi, BinarySections, &count);
    rd_printf("# BINARY SECTIONS");
    rd_indent();
    for (U64 idx = 0; idx < count; ++idx) {
      rd_printf("section[%llu]:", idx);
      rd_indent();
      rdi_print_binary_section(arena, out, indent, rdi, &v[idx]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiFilePaths) {
    U64                file_path_count = 0;
    RDI_FilePathNode *file_path_array = rdi_table_from_name(rdi, FilePathNodes, &file_path_count);
    rd_printf("# FILE PATHS");
    rd_indent();
    for (U64 i = 0; i < file_path_count; ++i) {
      if (file_path_array[i].parent_path_node == 0) {
        rdi_print_file_path(arena, out, indent, rdi, &file_path_array[i]);
      }
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiSourceFiles) {
    U64             source_file_count = 0;
    RDI_SourceFile *source_file_array = rdi_table_from_name(rdi, SourceFiles, &source_file_count);
    rd_printf("# SOURCE FILES");
    rd_indent();
    for (U64 i = 0; i < source_file_count; ++i) {
      RDI_SourceFile *source_file = &source_file_array[i];
      rd_printf("source_file[%4llu] = { file_path_node_idx = %4u, source_line_map = %4u, path = '%S' }",
                i,
                source_file->file_path_node_idx,
                source_file->source_line_map_idx,
                str8_from_rdi_string_idx(rdi, source_file->normal_full_path_string_idx));
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiUnits) {
    U64       unit_count = 0;
    RDI_Unit *unit_array = rdi_table_from_name(rdi, Units, &unit_count);
    rd_printf("# UNITS");
    rd_indent();
    for (U64 i = 0; i < unit_count; ++i) {
      rd_printf("unit[%llu]:", i);
      rd_indent();
      rdi_print_unit(arena, out, indent, rdi, &unit_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiUnitVMap) {
    U64            vmap_count = 0;
    RDI_VMapEntry *vmap_array = rdi_table_from_name(rdi, UnitVMap, &vmap_count);
    rd_printf("# UNIT VMAP");
    rd_indent();
    for (U64 i = 0; i < vmap_count; ++i) {
      rdi_print_vmap_entry(arena, out, indent, &vmap_array[i]);
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiLineTables) {
    U64            line_table_count = 0;
    RDI_LineTable *line_table_array = rdi_table_from_name(rdi, LineTables, &line_table_count);
    rd_printf("# LINE TABLES");
    rd_indent();
    for (U64 i = 0; i < line_table_count; ++i) {
      rd_printf("line_table[%llu]", i);
      rd_indent();
      rdi_print_line_table(arena, out, indent, rdi, &line_table_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiSourceLineMaps) {
    U64                source_line_map_count = 0;
    RDI_SourceLineMap *source_line_map_array = rdi_table_from_name(rdi, SourceLineMaps, &source_line_map_count);
    rd_printf("# SOURCE LINE MAPS");
    rd_indent();
    for (U64 i = 0; i < source_line_map_count; ++i) {
      rd_printf("source_line_map[%llu]:", i);
      rd_indent();
      rdi_print_source_line_map(arena, out, indent, rdi, &source_line_map_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiTypeNodes) {
    U64           type_node_count = 0;
    RDI_TypeNode *type_node_array = rdi_table_from_name(rdi, TypeNodes, &type_node_count);
    rd_printf("# TYPE NODES");
    rd_indent();
    for (U64 i = 0; i < type_node_count; ++i) {
      rd_printf("type[%llu]", i);
      rd_indent();
      rdi_print_type_node(arena, out, indent, rdi, &type_node_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiUserDefinedTypes) {
    U64      udt_count = 0;
    RDI_UDT *udt_array = rdi_table_from_name(rdi, UDTs, &udt_count);
    rd_printf("# UDTS");
    rd_indent();
    for (U64 i = 0; i < udt_count; ++i) {
      rd_printf("udt[%u]:", i);
      rd_indent();
      rdi_print_udt(arena, out, indent, rdi, &udt_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiGlobalVars) {
    U64                 gvar_count = 0;
    RDI_GlobalVariable *gvar_array = rdi_table_from_name(rdi, GlobalVariables, &gvar_count);
    rd_printf("# GLOBAL VARIABLES");
    rd_indent();
    for (U64 i = 0; i < gvar_count; ++i) {
      rd_printf("global_variable[%llu]:", i);
      rd_indent();
      rdi_print_global_variable(arena, out, indent, rdi, &gvar_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiGlobalVarsVMap) {
    U64            vmap_count = 0;
    RDI_VMapEntry *vmap_array = rdi_table_from_name(rdi, GlobalVMap, &vmap_count);
    rd_printf("# GLOBAL VMAP");
    rd_indent();
    for (U64 i = 0; i < vmap_count; ++i) {
      rdi_print_vmap_entry(arena, out, indent, &vmap_array[i]);
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiThreadVars) {
    U64                 tvar_count = 0;
    RDI_ThreadVariable *tvar_array = rdi_table_from_name(rdi, ThreadVariables, &tvar_count);
    rd_printf("# THREAD VARIABLES");
    rd_indent();
    for (U64 i = 0; i < tvar_count; ++i) {
      rd_printf("thread_variable[%llu]:", i);
      rd_indent();
      rdi_print_thread_variable(arena, out, indent, rdi, &tvar_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiConstants) {
    U64           constants_count = 0;
    RDI_Constant *constants_array = rdi_table_from_name(rdi, Constants, &constants_count);
    rd_printf("# CONSTANTS");
    rd_indent();
    for (U64 i = 0; i < constants_count; ++i) {
      rd_printf("constant[%llu]:", i);
      rd_indent();
      rdi_print_constant(arena, out, indent, rdi, &constants_array[i]);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiProcedures) {
    U64            proc_count = 0;
    RDI_Procedure *proc_array = rdi_table_from_name(rdi, Procedures, &proc_count);
    rd_printf("# PROCEDURES");
    rd_indent();
    for (U64 i = 0; i < proc_count; ++i) {
      rd_printf("procedure[%llu]:", i);
      rd_indent();
      rdi_print_procedure(arena, out, indent, rdi, &proc_array[i], tli->arch);
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiScopes) {
    U64        scope_count = 0;
    RDI_Scope *scope_array = rdi_table_from_name(rdi, Scopes, &scope_count);
    rd_printf("# SCOPES");
    rd_indent();
    for (U64 i = 0; i < scope_count; ++i) {
      if (scope_array[i].parent_scope_idx == 0) {
        rdi_print_scope(arena, out, indent, rdi, &scope_array[i], tli->arch);
      }
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiScopeVMap) {
    U64            vmap_count = 0;
    RDI_VMapEntry *vmap_array = rdi_table_from_name(rdi, ScopeVMap, &vmap_count);
    rd_printf("# SCOPE VMAP");
    rd_indent();
    for (U64 i = 0; i < vmap_count; ++i) {
      rdi_print_vmap_entry(arena, out, indent, &vmap_array[i]);
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiInlineSites) {
    U64             inline_site_count = 0;
    RDI_InlineSite *inline_site_array = rdi_table_from_name(rdi, InlineSites, &inline_site_count);
    rd_printf("# INLINE SITES");
    rd_indent();
    for (U64 i = 0; i < inline_site_count; ++i) {
      rdi_print_inline_site(arena, out, indent, rdi, i, &inline_site_array[i]);
    }
    rd_unindent();
    rd_newline();
  }
  if (opts & RD_Option_RdiNameMaps) {
    Temp scratch = scratch_begin(&arena, 1);
    U64          name_map_count = 0;
    RDI_NameMap *name_map_array = rdi_table_from_name(rdi, NameMaps, &name_map_count);
    rd_printf("# NAME MAPS");
    rd_indent();
    for (U64 i = 0; i < name_map_count; ++i) {
      if (i > 0) {
        rd_newline();
      }
      RDI_ParsedNameMap name_map = {0};
      rdi_parsed_from_name_map(rdi, &name_map_array[i], &name_map);
      rd_printf("name_map[%S]",rdi_string_from_name_map_kind(i));
      rd_indent();
      for (U64 bucket_idx = 0; bucket_idx < name_map.bucket_count; ++bucket_idx) {
        if (name_map.buckets[bucket_idx].node_count == 0) {
          continue;
        }
        rd_printf("bucket[%llu]:", bucket_idx);
        rd_indent();
        RDI_NameMapNode *node_ptr = name_map.nodes + name_map.buckets[bucket_idx].first_node;
        RDI_NameMapNode *node_opl = node_ptr + name_map.buckets[bucket_idx].node_count;
        for (; node_ptr < node_opl; ++node_ptr) {
          Temp temp = temp_begin(scratch.arena);
          String8 str     = str8_from_rdi_string_idx(rdi, node_ptr->string_idx);
          String8 indices;
          if (node_ptr->match_count == 1) {
            indices = push_str8f(temp.arena, "%u", node_ptr->match_idx_or_idx_run_first);
          } else {
            U32  idx_count = 0;
            U32 *idx_array = rdi_idx_run_from_first_count(rdi, node_ptr->match_idx_or_idx_run_first, node_ptr->match_count, &idx_count);
            indices = rd_string_from_array_u32(temp.arena, idx_array, idx_count);
          }
          rd_printf("match \"%S\": %S", str, indices);
          temp_end(temp);
        }
        rd_unindent();
      }
      rd_unindent();
    }
    rd_unindent();
    rd_newline();
    scratch_end(scratch);
  }
  if (opts & RD_Option_RdiStrings) {
    U64  stridx_count = 0;
    U32 *stridx_array = rdi_table_from_name(rdi, StringTable, &stridx_count);
    rd_printf("# STRINGS");
    rd_indent();
    for (U64 i = 0; i < stridx_count; ++i) {
      rd_printf("string[%llu]: \"%S\"", i, str8_from_rdi_string_idx(rdi, i));
    }
    rd_unindent();
    rd_newline();
  }
}

internal String8
dw_string_from_reg_off(Arena *arena, Arch arch, U64 reg_idx, S64 reg_off)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 reg_str = dw_string_from_register(scratch.arena, arch, reg_idx);
  String8 result = rd_string_from_reg_off(arena, reg_str, reg_off);
  scratch_end(scratch);
  return result;
}

B32 is_global_var = 0;

internal String8List
dw_string_list_from_expression(Arena *arena, String8 raw_data, U64 cu_base, U64 address_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List result = {0};
  for (U64 cursor = 0; cursor < raw_data.size; ) {
    U8 op = 0;
    cursor += str8_deserial_read_struct(raw_data, cursor, &op);
    
    String8 op_value   = str8_zero();
    U64     size_param = 0;
    B32     is_signed  = 0;
    switch (op) {
      case DW_ExprOp_Lit0:  case DW_ExprOp_Lit1:  case DW_ExprOp_Lit2:
      case DW_ExprOp_Lit3:  case DW_ExprOp_Lit4:  case DW_ExprOp_Lit5:
      case DW_ExprOp_Lit6:  case DW_ExprOp_Lit7:  case DW_ExprOp_Lit8:
      case DW_ExprOp_Lit9:  case DW_ExprOp_Lit10: case DW_ExprOp_Lit11:
      case DW_ExprOp_Lit12: case DW_ExprOp_Lit13: case DW_ExprOp_Lit14:
      case DW_ExprOp_Lit15: case DW_ExprOp_Lit16: case DW_ExprOp_Lit17:
      case DW_ExprOp_Lit18: case DW_ExprOp_Lit19: case DW_ExprOp_Lit20:
      case DW_ExprOp_Lit21: case DW_ExprOp_Lit22: case DW_ExprOp_Lit23:
      case DW_ExprOp_Lit24: case DW_ExprOp_Lit25: case DW_ExprOp_Lit26:
      case DW_ExprOp_Lit27: case DW_ExprOp_Lit28: case DW_ExprOp_Lit29:
      case DW_ExprOp_Lit30: case DW_ExprOp_Lit31: {
        U64 x = op - DW_ExprOp_Lit0;
        op_value = push_str8f(scratch.arena, "%llu", x);
      } break;
      
      case DW_ExprOp_Const1U:size_param = 1;                goto const_n;
      case DW_ExprOp_Const2U:size_param = 2;                goto const_n;
      case DW_ExprOp_Const4U:size_param = 4;                goto const_n;
      case DW_ExprOp_Const8U:size_param = 8;                goto const_n;
      case DW_ExprOp_Const1S:size_param = 1; is_signed = 1; goto const_n;
      case DW_ExprOp_Const2S:size_param = 2; is_signed = 1; goto const_n;
      case DW_ExprOp_Const4S:size_param = 4; is_signed = 1; goto const_n;
      case DW_ExprOp_Const8S:size_param = 8; is_signed = 1; goto const_n;
      const_n:
      {
        if (is_signed) {
          S64 x = 0;
          cursor += str8_deserial_read(raw_data, cursor, &x, size_param, 1);
          x = extend_sign64(x, size_param);
          op_value = push_str8f(scratch.arena, "%lld", x);
        } else {
          U64 x = 0;
          cursor += str8_deserial_read(raw_data, cursor, &x, size_param, 1);
          op_value = push_str8f(scratch.arena, "%llu", x);
        }
      } break;
      
      case DW_ExprOp_Addr: {
        U64 addr = 0;
        cursor += str8_deserial_read(raw_data, cursor, &addr, address_size, 1);
        op_value = push_str8f(scratch.arena, "%#llx", addr);
      } break;
      
      case DW_ExprOp_ConstU: {
        U64 x = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%llu", x);
      } break;
      
      case DW_ExprOp_ConstS: {
        S64 x = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%lld", x);
      } break;
      
      case DW_ExprOp_Reg0:  case DW_ExprOp_Reg1:  case DW_ExprOp_Reg2:
      case DW_ExprOp_Reg3:  case DW_ExprOp_Reg4:  case DW_ExprOp_Reg5:
      case DW_ExprOp_Reg6:  case DW_ExprOp_Reg7:  case DW_ExprOp_Reg8:
      case DW_ExprOp_Reg9:  case DW_ExprOp_Reg10: case DW_ExprOp_Reg11:
      case DW_ExprOp_Reg12: case DW_ExprOp_Reg13: case DW_ExprOp_Reg14:
      case DW_ExprOp_Reg15: case DW_ExprOp_Reg16: case DW_ExprOp_Reg17:
      case DW_ExprOp_Reg18: case DW_ExprOp_Reg19: case DW_ExprOp_Reg20:
      case DW_ExprOp_Reg21: case DW_ExprOp_Reg22: case DW_ExprOp_Reg23:
      case DW_ExprOp_Reg24: case DW_ExprOp_Reg25: case DW_ExprOp_Reg26:
      case DW_ExprOp_Reg27: case DW_ExprOp_Reg28: case DW_ExprOp_Reg29:
      case DW_ExprOp_Reg30: case DW_ExprOp_Reg31: {
        U64 reg_idx = op - DW_ExprOp_Reg0;
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, 0);
      } break;
      
      case DW_ExprOp_RegX: {
        U64 reg_idx = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, 0);
      } break;
      
      case DW_ExprOp_ImplicitValue: {
        U64 value_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &value_size);
        Rng1U64 value_range = rng_1u64(cursor, cursor + value_size);
        String8 value_data  = str8_substr(raw_data, value_range);
        cursor += value_size;
        
        String8 value_str = rd_string_from_hex_u8(scratch.arena, value_data.str, value_data.size);
        op_value = push_str8f(scratch.arena, "{ %S }", value_str);
      } break;
      
      case DW_ExprOp_Piece: {
        U64 size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &size);
        op_value = push_str8f(scratch.arena, "%u", size);
      } break;
      
      case DW_ExprOp_BitPiece: {
        U64 bit_size = 0, bit_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &bit_size);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &bit_off);
        op_value = push_str8f(scratch.arena, "bit size %llu, bit offset %llu", bit_size, bit_off);
      } break;
      
      case DW_ExprOp_Pick: {
        U8 stack_idx = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &stack_idx);
        op_value = push_str8f(scratch.arena, "stack index %u", stack_idx);
      } break;
      
      case DW_ExprOp_PlusUConst: {
        U64 addend = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &addend);
        op_value = push_str8f(arena, "addend %llu", addend);
      } break;
      
      case DW_ExprOp_Skip: {
        S16 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%+d bytes", x);
      } break;
      
      case DW_ExprOp_Bra: {
        S16 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%+d", x);
      } break;
      
      case DW_ExprOp_BReg0:  case DW_ExprOp_BReg1:  case DW_ExprOp_BReg2: 
      case DW_ExprOp_BReg3:  case DW_ExprOp_BReg4:  case DW_ExprOp_BReg5: 
      case DW_ExprOp_BReg6:  case DW_ExprOp_BReg7:  case DW_ExprOp_BReg8:  
      case DW_ExprOp_BReg9:  case DW_ExprOp_BReg10: case DW_ExprOp_BReg11: 
      case DW_ExprOp_BReg12: case DW_ExprOp_BReg13: case DW_ExprOp_BReg14: 
      case DW_ExprOp_BReg15: case DW_ExprOp_BReg16: case DW_ExprOp_BReg17: 
      case DW_ExprOp_BReg18: case DW_ExprOp_BReg19: case DW_ExprOp_BReg20: 
      case DW_ExprOp_BReg21: case DW_ExprOp_BReg22: case DW_ExprOp_BReg23: 
      case DW_ExprOp_BReg24: case DW_ExprOp_BReg25: case DW_ExprOp_BReg26: 
      case DW_ExprOp_BReg27: case DW_ExprOp_BReg28: case DW_ExprOp_BReg29: 
      case DW_ExprOp_BReg30: case DW_ExprOp_BReg31: {
        U64 reg_idx = op - DW_ExprOp_BReg0;
        S64 reg_off = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off);
      } break;
      
      case DW_ExprOp_FBReg: {
        S64 reg_off = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        op_value = push_str8f(scratch.arena, "offset %lld", reg_off);
      } break;
      
      case DW_ExprOp_BRegX: {
        U64 reg_idx = 0;
        S64 reg_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        op_value = dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off);
      } break;
      
      case DW_ExprOp_XDerefSize:
      case DW_ExprOp_DerefSize: {
        U8 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%u", x);
      } break;
      
      case DW_ExprOp_Call2: {
        U16 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(scratch.arena, "%u", x);
      } break;
      case DW_ExprOp_Call4: {
        U32 x = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &x);
        op_value = push_str8f(arena, "%u", x);
      } break;
      case DW_ExprOp_CallRef: {
        U64 x = 0;
        cursor += str8_deserial_read_dwarf_uint(raw_data, cursor, format, &x);
        op_value = push_str8f(scratch.arena, "%llu", x);
      } break;
      case DW_ExprOp_ImplicitPointer:
      case DW_ExprOp_GNU_ImplicitPointer: {
        U64 info_off = 0;
        cursor += str8_deserial_read_dwarf_uint(raw_data, cursor, format, &info_off);
        S64 ptr = 0;
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &ptr);
        
        op_value = push_str8f(scratch.arena, ".debug_info+%#llx, ptr %llx", info_off, ptr);
      } break;
      case DW_ExprOp_Convert:
      case DW_ExprOp_GNU_Convert: {
        U64 type_cu_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        op_value = push_str8f(scratch.arena, "TypeCuOff %#llx", cu_base + type_cu_off);
      } break;
      case DW_ExprOp_GNU_ParameterRef: {
        // TODO: always 4 bytes?
        U32 cu_off = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &cu_off);
        op_value = push_str8f(scratch.arena, "CuOff %#x", cu_base + cu_off);
      } break;
      case DW_ExprOp_DerefType:
      case DW_ExprOp_GNU_DerefType: {
        U8  deref_size  = 0;
        U64 type_cu_off = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &deref_size);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        op_value = push_str8f(scratch.arena, "%#x, TypeCuOff %#llx", deref_size, cu_base + type_cu_off);
      } break;
      case DW_ExprOp_ConstType:
      case DW_ExprOp_GNU_ConstType: {
        U64 type_cu_off      = 0;
        U8  const_value_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        cursor += str8_deserial_read_struct(raw_data, cursor, &const_value_size);
        Rng1U64 const_value_range = rng_1u64(cursor, cursor + const_value_size);
        String8 const_value_data  = str8_substr(raw_data, const_value_range);
        String8 const_value_str   = rd_string_from_hex_u8(scratch.arena, const_value_data.str, const_value_data.size);
        op_value = push_str8f(scratch.arena, "TypeCuOff %#llx, Const Value { %S }", cu_base + type_cu_off, const_value_str);
        cursor += const_value_size;
      } break;
      case DW_ExprOp_RegvalType:
      case DW_ExprOp_GNU_RegvalType: {
        U64 reg_idx = 0, type_cu_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &type_cu_off);
        op_value = push_str8f(scratch.arena, "%S, TypeCuOff %#llx", dw_string_from_register(scratch.arena, arch, reg_idx), cu_base + type_cu_off);
      } break;
      case DW_ExprOp_EntryValue:
      case DW_ExprOp_GNU_EntryValue: {
        U64 block_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &block_size);
        Rng1U64     block_range = rng_1u64(cursor, cursor + block_size);
        String8     block_data  = str8_substr(raw_data, block_range);
        String8List block_expr  = dw_string_list_from_expression(scratch.arena, block_data, cu_base, address_size, arch, ver, ext, format);
        op_value = str8_list_join(scratch.arena, &block_expr, &(StringJoin){.pre = str8_lit("{ "), .sep = str8_lit(","), .post = str8_lit(" }")});
        cursor += block_size;
      } break;
      case DW_ExprOp_Addrx: {
        U64 addr = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &addr);
        op_value = push_str8f(scratch.arena, "%#llx", addr);
      } break;
      
      case DW_ExprOp_CallFrameCfa:
      case DW_ExprOp_FormTlsAddress:
      case DW_ExprOp_PushObjectAddress:
      case DW_ExprOp_Nop:
      case DW_ExprOp_Eq:
      case DW_ExprOp_Ge:
      case DW_ExprOp_Gt:
      case DW_ExprOp_Le:
      case DW_ExprOp_Lt:
      case DW_ExprOp_Ne:
      case DW_ExprOp_Shl:
      case DW_ExprOp_Shr:
      case DW_ExprOp_Shra:
      case DW_ExprOp_Xor:
      case DW_ExprOp_XDeref:
      case DW_ExprOp_Abs:
      case DW_ExprOp_And:
      case DW_ExprOp_Div:
      case DW_ExprOp_Minus:
      case DW_ExprOp_Mod:
      case DW_ExprOp_Mul:
      case DW_ExprOp_Neg:
      case DW_ExprOp_Not:
      case DW_ExprOp_Or:
      case DW_ExprOp_Plus:
      case DW_ExprOp_Rot:
      case DW_ExprOp_Swap:
      case DW_ExprOp_Deref:
      case DW_ExprOp_Dup:
      case DW_ExprOp_Drop:
      case DW_ExprOp_Over:
      case DW_ExprOp_StackValue: {
        // no operands
      } break;
    }
    
    String8 opcode_str = dw_string_from_expr_op(scratch.arena, ver, ext, op);
    if (op_value.size == 0) {
      str8_list_pushf(arena, &result, "DW_OP_%S", opcode_str);
    } else {
      str8_list_pushf(arena, &result, "DW_OP_%S = %S", opcode_str, op_value);
    }
  }
  scratch_end(scratch);
  return result;
}

internal String8
dw_format_expression_single_line(Arena *arena, String8 raw_data, U64 cu_base, U64 address_size, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format)
{
  Temp        scratch    = scratch_begin(&arena, 1);
  String8List list       = dw_string_list_from_expression(scratch.arena, raw_data, cu_base, address_size, arch, ver, ext, format);
  String8     expression = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return expression;
}

internal void
dw_print_cfi_program(Arena *arena, String8List *out, String8 indent, String8 raw_data, DW_CIEUnpacked *cie, DW_EhPtrCtx *ptr_ctx, Arch arch, DW_Version ver, DW_Ext ext, DW_Format format)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64 address_bit_size = bit_size_from_arch(arch);
  U64 address_size     = address_bit_size / 8;
  
  for (U64 cursor = 0; cursor < raw_data.size; /* empty */) {
    DW_CFA opcode = 0;
    cursor += str8_deserial_read_struct(raw_data, cursor, &opcode);
    
    U64 operand = 0;
    if ((opcode & DW_CFAMask_OpcodeHi) != 0) {
      operand = opcode & DW_CFAMask_Operand;
      opcode  = opcode & DW_CFAMask_OpcodeHi;
    }
    
    switch (opcode) {
      case DW_CFA_Nop: {
        rd_printf("DW_CFA_nop");
      } break;
      case DW_CFA_SetLoc: {
        U64 address = 0;
        switch (arch) {
          case Arch_x64: cursor += dw_unwind_parse_pointer_x64(raw_data.str, rng_1u64(0,raw_data.size), ptr_ctx, cie->addr_encoding, cursor, &address); break;
          
          default: NotImplemented; break;
        }
        rd_printf("DW_CFA_set_loc: %#llx", address);
      } break;
      case DW_CFA_AdvanceLoc1: {
        U8 delta = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &delta);
        
        rd_printf("DW_CFA_advance_loc1: %+u", delta * cie->code_align_factor);
      } break;
      case DW_CFA_AdvanceLoc2: {
        U16 delta = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &delta);
        
        rd_printf("DW_CFA_advance_loc2: %+u", delta * cie->code_align_factor);
      } break;
      case DW_CFA_AdvanceLoc4: {
        U32 delta = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &delta);
        
        rd_printf("DW_CFA_advance_loc4: %+u", delta * cie->code_align_factor);
      } break;
      case DW_CFA_OffsetExt: {
        U64 reg_idx = 0, reg_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_off);
        
        rd_printf("DW_CFA_offset_extended: %S register %llu (%S), offset %+llu", 
                  dw_string_from_reg_off(scratch.arena, arch, reg_idx, (S64)reg_off * cie->data_align_factor));
      } break;
      case DW_CFA_RestoreExt: {
        rd_printf("DW_CFA_restore_extended");
      } break;
      case DW_CFA_Undefined: {
        U64 reg = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg);
        
        rd_printf("DW_CFA_undefined: %llu", reg);
      } break;
      case DW_CFA_SameValue: {
        U64 reg = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg);
        
        rd_printf("DW_CFA_same_value: %S", dw_string_from_register(scratch.arena, arch, reg));
      } break;
      case DW_CFA_Register: {
        U64 reg_idx = 0, reg_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_off);
        
        rd_printf("DW_CFA_register: %S", dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off));
      } break;
      case DW_CFA_RememberState: {
        rd_printf("DW_CFA_remember_state");
      } break;
      case DW_CFA_RestoreState: {
        rd_printf("DW_CFA_restore_state");
      } break;
      case DW_CFA_DefCfa: {
        U64 reg_idx = 0, reg_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_off);
        
        rd_printf("DW_CFA_def_cfa: %S", dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off));
      } break;
      case DW_CFA_DefCfaRegister: {
        U64 reg = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg);
        
        rd_printf("DW_CFA_register: %llu (%S)", 
                  reg, 
                  dw_string_from_register(arena, arch, reg));
      } break;
      case DW_CFA_DefCfaOffset: {
        U64 offset = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &offset);
        
        rd_printf("DW_CFA_def_cfa_offset: %llu", offset);
      } break;
      case DW_CFA_DefCfaExpr: {
        U64 block_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &block_size);
        String8 raw_expr = str8_substr(raw_data, rng_1u64(cursor, cursor + block_size));
        cursor += block_size;
        
        rd_printf("DW_CFA_def_cfa_expression: %S",
                  dw_format_expression_single_line(scratch.arena, raw_expr, 0, address_size, arch, ver, ext, format));
      } break;
      case DW_CFA_Expr: {
        U64 reg = 0, block_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &block_size);
        String8 raw_expr = str8_substr(raw_data, rng_1u64(cursor, cursor + block_size));
        cursor += block_size;
        
        rd_printf("DW_CFA_expression: %S, expression %S", 
                  dw_string_from_register(scratch.arena, arch, reg), 
                  dw_format_expression_single_line(scratch.arena, raw_expr, 0, address_size, arch, ver, ext, format));
      } break;
      case DW_CFA_OffsetExtSf: {
        U64 reg_idx = 0;
        S64 reg_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        
        rd_printf("DW_CFA_offset_ext_sf: %S", dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off * cie->data_align_factor));
      } break;
      case DW_CFA_DefCfaSf: {
        U64 reg_idx = 0;
        S64 reg_off = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &reg_idx);
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &reg_off);
        
        rd_printf("DW_CFA_def_cfa_sf: %S", dw_string_from_reg_off(scratch.arena, arch, reg_idx, reg_off * cie->data_align_factor));
      } break;
      case DW_CFA_ValOffset: {
        U64 val = 0, offset = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &val);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &offset);
        
        rd_printf("DW_CFA_val_offset: value %llu, offset %+llu", val, offset);
      } break;
      case DW_CFA_ValOffsetSf: {
        U64 val    = 0;
        S64 offset = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &val);
        cursor += str8_deserial_read_sleb128(raw_data, cursor, &offset);
        
        rd_printf("DW_CFA_val_offset_sf: value %llu, offset %+lld", val, offset);
      } break;
      case DW_CFA_ValExpr: {
        U64 val        = 0;
        U64 block_size = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &val);
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &block_size);
        String8 raw_expr = str8_substr(raw_data, rng_1u64(cursor, cursor + block_size));
        cursor += block_size;
        
        rd_printf("DW_CFA_val_expr: value %+llu, expression %S",
                  val,
                  dw_format_expression_single_line(scratch.arena, raw_expr, 0, address_size, arch, ver, ext, format));
      } break;
      case DW_CFA_AdvanceLoc: {
        rd_printf("DW_CFA_advance_loc: %+llu", operand);
      } break;
      case DW_CFA_Offset: {
        U64 offset = 0;
        cursor += str8_deserial_read_uleb128(raw_data, cursor, &offset);
        S64 v = (S64)offset * cie->data_align_factor;
        
        rd_printf("DW_CFA_offset: %S", dw_string_from_reg_off(scratch.arena, arch, operand, v));
      } break;
      case DW_CFA_Restore: {
        rd_printf("DW_CFA_restore: %S", operand, dw_string_from_register(scratch.arena, arch, operand));
      } break;
      default: {
        rd_errorf("unknown CFI opcode %u", opcode);
      } break;
    }
  }
  
  scratch_end(scratch);
}

internal String8
dw_format_eh_ptr_enc(Arena *arena, DW_EhPtrEnc enc)
{
  U8 type = enc & DW_EhPtrEnc_TypeMask;
  String8 type_str = str8_lit("NULL");
  switch (type) {
    case DW_EhPtrEnc_Ptr:     type_str = str8_lit("PTR");      break;
    case DW_EhPtrEnc_ULEB128: type_str = str8_lit("ULEB128");  break;
    case DW_EhPtrEnc_UData2:  type_str = str8_lit("UDATA2");   break;
    case DW_EhPtrEnc_UData4:  type_str = str8_lit("UDATA4");   break;
    case DW_EhPtrEnc_UData8:  type_str = str8_lit("UDATA8");   break;
    case DW_EhPtrEnc_Signed:  type_str = str8_lit("SIGNED");   break;
    case DW_EhPtrEnc_SLEB128: type_str = str8_lit("SLEB128");  break;
    case DW_EhPtrEnc_SData2:  type_str = str8_lit("SDATA2");   break;
    case DW_EhPtrEnc_SData4:  type_str = str8_lit("SDATA4");   break;
    case DW_EhPtrEnc_SData8:  type_str = str8_lit("SDATA8");   break;
  }
  U8 modifier = enc & DW_EhPtrEnc_ModifyMask;
  String8 modifier_str = str8_lit("NULL");
  switch (modifier) {
    case DW_EhPtrEnc_PcRel:   modifier_str = str8_lit("PCREL");   break;
    case DW_EhPtrEnc_TextRel: modifier_str = str8_lit("TEXTREL"); break;
    case DW_EhPtrEnc_DataRel: modifier_str = str8_lit("DATAREL"); break;
    case DW_EhPtrEnc_FuncRel: modifier_str = str8_lit("FUNCREL"); break;
  }
  String8 indir_str = str8_lit("");
  if (enc & DW_EhPtrEnc_Indirect) {
    indir_str = str8_lit("(INDIRECT)");
  }
  return push_str8f(arena, "Type: %S, Modifier: %S %S", type_str, modifier_str, indir_str);
}

internal void
dw_print_eh_frame(Arena *arena, String8List *out, String8 indent, String8 raw_eh_frame, Arch arch, DW_Version ver, DW_Ext ext, DW_EhPtrCtx *ptr_ctx)
{
  Temp scratch = scratch_begin(&arena, 1);
  DW_CIEUnpacked cie = {0};
  
  for (U64 cursor = 0; cursor < raw_eh_frame.size; ) {
    U64 header_offset = cursor;
    
    U64 length = 0; // doesn't include bytes for size
    cursor += dw_based_range_read_length(raw_eh_frame.str, rng_1u64(0,raw_eh_frame.size), cursor, &length);
    
    if (length == 0) {
      break; // encountered exit marker
    }
    
    U64 entry_start = cursor;
    U64 entry_end   = cursor + length;
    
    U32 entry_id = 0; // always 4-bytes, even when length is encoded as 64-bit integer
    cursor += str8_deserial_read_struct(raw_eh_frame, cursor, &entry_id);
    
    // TODO: fix the freaking DW_EhPtrEnc_PCREL encoding.
    // it assumes "frame_base" points to the first byte of .eh_frame
    // but here base is start of ELF and we use range to select .eh_frame
    // bytes to read, which breaks parsing.
    String8 raw_frame = str8_substr(raw_eh_frame, rng_1u64(cursor, cursor + length - sizeof(entry_id)));
    Rng1U64 cfi_range = rng_1u64(0,0);
    
    // CIE
    if (entry_id == 0) {
      dw_unwind_parse_cie_x64(raw_frame.str, rng_1u64(0,raw_frame.size), ptr_ctx, 0, &cie);
      cfi_range = cie.cfi_range;
      
      rd_printf("CIE @ 0x%X, Length %u", header_offset, length);
      rd_indent();
      rd_printf("LSDA Encoding:           %S",    dw_format_eh_ptr_enc(scratch.arena, cie.lsda_encoding));
      rd_printf("Address Encoding:        %S",    dw_format_eh_ptr_enc(scratch.arena, cie.addr_encoding));
      rd_printf("Augmentation:            %S",    cie.augmentation);
      rd_printf("Code Align Factor:       %llu",  cie.code_align_factor);
      rd_printf("Data Align Factor:       %lld",  cie.data_align_factor);
      rd_printf("Return Address Register: %u",    cie.ret_addr_reg);
      rd_printf("Handler IP:              %#llx", cie.handler_ip);
      rd_unindent();
    }
    // FDE
    else {
      DW_FDEUnpacked fde = {0};
      dw_unwind_parse_fde_x64(raw_eh_frame.str, rng_1u64(0,raw_eh_frame.size), ptr_ctx, &cie, 0, &fde);
      cfi_range = fde.cfi_range;
      
      // calc parent CIE offset
      AssertAlways(entry_start >= entry_id);
      U64 cie_offset = entry_start - entry_id; NotImplemented; // TODO: syms_safe_sub_u64(range.min + entry_start, entry_id);
      
      rd_printf("FDE @ %#llx, Length %u, Parent CIE @ %#llx", header_offset, length, cie_offset);
      rd_indent();
      rd_printf("IP Range: %#llx-%#llx", fde.ip_voff_range.min, fde.ip_voff_range.max);
      rd_printf("LSDA IP:  %#llx",       fde.lsda_ip);
      rd_unindent();
    }
    
    // print CFI program
    rd_printf("CFI Program:");
    rd_indent();
    
    DW_Format format  = DW_FormatFromSize(length);
    String8   raw_cfi = str8_substr(raw_eh_frame, cfi_range);
    dw_print_cfi_program(scratch.arena, out, indent, raw_cfi, &cie, ptr_ctx, arch, ver, ext, format);
    
    rd_unindent();
    rd_newline();
    
    // advance to next entry
    cursor = entry_end;
  }
  
  scratch_end(scratch);
}

internal void
dw_print_debug_info(Arena *arena, String8List *out, String8 indent, DW_Input *input, DW_ListUnitInput lu_input, Arch arch, B32 relaxed)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  Rng1U64List      cu_ranges = dw_unit_ranges_from_data(scratch.arena, input->sec[DW_Section_Info].data);
  
  if (cu_ranges.count > 0) {
    rd_printf("# %S", input->sec[DW_Section_Info].name);
    rd_indent();
  }
  
  U64 comp_idx = 0;
  for (Rng1U64Node *cu_range_n = cu_ranges.first; cu_range_n != 0; cu_range_n = cu_range_n->next, ++comp_idx) {
    Temp comp_temp = temp_begin(scratch.arena);
    
    U64         cu_base  = cu_range_n->v.min;
    Rng1U64     cu_range = cu_range_n->v;
    DW_CompUnit cu       = dw_cu_from_info_off(comp_temp.arena, input, lu_input, cu_range.min, relaxed);
    
    String8 cu_dir    = dw_string_from_attrib  (input, &cu, cu.tag, DW_Attrib_CompDir );
    String8 cu_name   = dw_string_from_attrib  (input, &cu, cu.tag, DW_Attrib_Name    );
    String8 stmt_list = dw_line_ptr_from_attrib(input, &cu, cu.tag, DW_Attrib_StmtList);
    
    DW_LineVMHeader line_vm   = {0};
    dw_read_line_vm_header(comp_temp.arena, stmt_list, 0, input, cu_dir, cu_name, cu.address_size, cu.str_offsets_lu, &line_vm);
    
    // print comp info
    rd_printf("Compilation Unit #%u", comp_idx);
    rd_indent();
    rd_printf("Version:       %u",               cu.version);
    rd_printf("Address Size:  %llu",             cu.address_size);
    rd_printf("Abbrev Offset: %#llx",            cu.abbrev_off);
    rd_printf("Info Range:    %#llx-%#llx (%M)", cu.info_range.min, cu.info_range.max, dim_1u64(cu.info_range));
    rd_newline();
    
    // prase tags
    U32 tag_depth = 0;
    for (U64 info_off = cu.first_tag_info_off; info_off < cu.info_range.max; ) {
      Temp tag_temp = temp_begin(scratch.arena);
      
      U64    tag_info_off = info_off;
      DW_Tag tag          = {0};
      info_off += dw_read_tag_cu(tag_temp.arena, input, &cu, tag_info_off, &tag);
      
      String8 tag_str = dw_string_from_tag_kind(tag_temp.arena, tag.kind);
      rd_printf("<%x><%llx> DW_Tag_%S (Abbrev Number: %llu)", tag_depth, tag_info_off, tag_str, tag.abbrev_id);
      rd_indent();
      
      // parse attributes
      for (DW_AttribNode *attrib_n = tag.attribs.first; attrib_n != 0; attrib_n = attrib_n->next) {
        Temp attrib_temp = temp_begin(tag_temp.arena);
        
        DW_Attrib *attrib = &attrib_n->v;
        
        String8List attrib_list = {0};
        
        // attribute .debug_info offset
        str8_list_pushf(attrib_temp.arena, &attrib_list, "<%llx> ", attrib->info_off);
        
        // attribute kind
        String8 attrib_kind_str = dw_string_from_attrib_kind(attrib_temp.arena, cu.version, cu.ext, attrib->attrib_kind);
        if (attrib_kind_str.size == 0) {
          if (relaxed) {
            attrib_kind_str = dw_string_from_attrib_kind(attrib_temp.arena, DW_Version_Last, cu.ext, attrib->attrib_kind);
          }
        }
        if (attrib_kind_str.size == 0) {
          str8_list_pushf(attrib_temp.arena, &attrib_list, "Unknown(%#x) ", attrib->attrib_kind);
        } else {
          str8_list_pushf(attrib_temp.arena, &attrib_list, "DW_Attrib_%-20S ", attrib_kind_str);
        }
        
        // form kind
        String8 form_kind_str = dw_string_from_form_kind(scratch.arena, cu.version, attrib->form_kind);
        str8_list_pushf(attrib_temp.arena, &attrib_list, "DW_Form_%-15S", form_kind_str);
        
        DW_AttribClass value_class = dw_value_class_from_attrib(&cu, attrib);
        switch (value_class) {
          default: {
            str8_list_pushf(attrib_temp.arena, &attrib_list, "ERROR: unknown value class");
          } break;
          case DW_AttribClass_Undefined: {
            str8_list_pushf(attrib_temp.arena, &attrib_list, "ERROR: undefined value class");
          } break;
          case DW_AttribClass_Address: {
            U64 address = dw_address_from_attrib_ptr(input, &cu, attrib);
            str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", address);
          } break;
          case DW_AttribClass_Block: {
            String8 block     = dw_block_from_attrib_ptr(input, &cu, attrib);
            String8 block_str = rd_string_from_hex_u8(attrib_temp.arena, block.str, block.size);
            str8_list_pushf(attrib_temp.arena, &attrib_list, "%S", block_str);
          } break;
          case DW_AttribClass_Const: {
            U64 constant = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", constant);
          } break;
          case DW_AttribClass_ExprLoc: {
            String8 exprloc     = dw_exprloc_from_attrib_ptr(input, &cu, attrib);
            String8 exprloc_str = dw_format_expression_single_line(attrib_temp.arena, exprloc, cu_base, cu.address_size, arch, cu.version, cu.ext, cu.format);
            str8_list_push(attrib_temp.arena, &attrib_list, exprloc_str);
          } break;
          case DW_AttribClass_Flag: {
            B32 flag = dw_flag_from_attrib_ptr(input, &cu, attrib);
            str8_list_pushf(attrib_temp.arena, &attrib_list, "%llu (%s)", flag, flag == 0 ? "false" : "true");
          } break;
          case DW_AttribClass_LinePtr: {
            if (attrib->form_kind == DW_Form_SecOffset) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "ERROR: unexpected form %S", dw_string_from_form_kind(attrib_temp.arena, cu.version, attrib->form_kind));
            }
          } break;
          case DW_AttribClass_LocListPtr: {
            if (attrib->form_kind == DW_Form_SecOffset) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "ERROR: unexpected form %S", dw_string_from_form_kind(attrib_temp.arena, cu.version, attrib->form_kind));
            }
          } break;
          case DW_AttribClass_MacPtr: {
            if (attrib->form_kind == DW_Form_SecOffset) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "\?\?\?");
            }
          } break;
          case DW_AttribClass_RngListPtr: {
            if (attrib->form_kind == DW_Form_SecOffset) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "\?\?\?");
            }
          } break;
          case DW_AttribClass_RngList: {
            if (attrib->form_kind == DW_Form_SecOffset) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "\?\?\?");
            }
          } break;
          case DW_AttribClass_Reference: {
            if (attrib->form_kind == DW_Form_Ref1 ||
                attrib->form_kind == DW_Form_Ref2 ||
                attrib->form_kind == DW_Form_Ref4 ||
                attrib->form_kind == DW_Form_Ref8 ||
                attrib->form_kind == DW_Form_RefUData) {
              U64 info_off = cu.info_range.min + attrib->form.ref;
              str8_list_pushf(attrib_temp.arena, &attrib_list, "<%llx>", info_off);
              if (!contains_1u64(cu.info_range, attrib->form.ref)) {
                str8_list_pushf(attrib_temp.arena, &attrib_list, "(ERROR: out of bounds reference)");
              }
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.ref);
            }
          } break;
          case DW_AttribClass_String: {
            if (attrib->form_kind == DW_Form_Strp) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            }
            String8 string = dw_string_from_attrib_ptr(input, &cu, attrib);
            str8_list_pushf(attrib_temp.arena, &attrib_list, "(%S)", string);
          } break;
          case DW_AttribClass_StrOffsetsPtr: {
            if (attrib->form_kind == DW_Form_SecOffset) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "ERROR: unexpected form %S", dw_string_from_form_kind(attrib_temp.arena, cu.version, attrib->form_kind));
            }
          } break;
          case DW_AttribClass_AddrPtr: {
            if (attrib->form_kind == DW_Form_SecOffset) {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "%#llx", attrib->form.sec_offset);
            } else {
              str8_list_pushf(attrib_temp.arena, &attrib_list, "ERROR: unexpected form %S", dw_string_from_form_kind(attrib_temp.arena, cu.version, attrib->form_kind));
            }
          } break;
        }
        
        String8 attrib_str = {0};
        switch (attrib->attrib_kind) {
          case DW_Attrib_Language: {
            DW_Language lang = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            attrib_str = dw_string_from_language(attrib_temp.arena, lang);
          } break;
          case DW_Attrib_DeclFile: {
            U64          file_idx = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            DW_LineFile *file     = dw_file_from_attrib_ptr(&cu, &line_vm, attrib);
            attrib_str = str8_lit("\?\?\?");
            if (file) {
              attrib_str = dw_path_from_file(attrib_temp.arena, &line_vm, file);
            }
          } break;
          case DW_Attrib_DeclLine: {
            U64 line = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            attrib_str = push_str8f(attrib_temp.arena, "%llu", line);
          } break;
          case DW_Attrib_Inline: {
            DW_InlKind inl = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            attrib_str = dw_string_from_inl(attrib_temp.arena, inl);
          } break;
          case DW_Attrib_Accessibility: {
            DW_AccessKind access = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            attrib_str = dw_string_from_access_kind(attrib_temp.arena, access);
          } break;
          case DW_Attrib_CallingConvention: {
            DW_CallingConventionKind calling_convetion = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            attrib_str = dw_string_from_calling_convetion(attrib_temp.arena, calling_convetion);
          } break;
          case DW_Attrib_Encoding: {
            DW_ATE encoding = dw_const_u64_from_attrib_ptr(input, &cu, attrib);
            attrib_str = dw_string_from_attrib_type_encoding(attrib_temp.arena, encoding);
          } break;
        }
        
        if (attrib_str.size) {
          str8_list_pushf(attrib_temp.arena, &attrib_list, "(%S)", attrib_str);
        }
        String8 print = str8_list_join(attrib_temp.arena, &attrib_list, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", print);
        
        temp_end(attrib_temp);
      }
      
      B32 is_ender_tag = tag.abbrev_id == 0;
      if (tag.has_children) {
        if (is_ender_tag) {
          rd_errorf("null-tag cannot have children");
        }
        rd_indent();
        tag_depth += 1;
      }
      if (is_ender_tag) {
        if (tag_depth == 0) {
          rd_errorf("malformed data detected, too many null tags");
        } else {
          rd_unindent();
          tag_depth -= 1;
        }
      }
      
      rd_unindent();
      temp_end(tag_temp);
    }
    temp_end(comp_temp);
    
    rd_unindent();
    rd_newline();
  }
  
  if (cu_ranges.count > 0) {
    rd_unindent();
  }
  
  scratch_end(scratch);
}

internal void
dw_print_debug_abbrev(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  DW_Section abbrev     = input->sec[DW_Section_Abbrev];
  String8    raw_abbrev = abbrev.data;
  
  if (raw_abbrev.size) {
    rd_printf("# %S", input->sec[DW_Section_Abbrev].name);
    rd_indent();
  }
  
  for (U64 cursor = 0; cursor < raw_abbrev.size; ) {
    U64 id_off = cursor;
    
    U64 id = 0;
    cursor += str8_deserial_read_uleb128(raw_abbrev, cursor, &id);
    
    if (id == 0) {
      continue; // end of abbrev data for CU
    }
    
    Temp temp = temp_begin(scratch.arena);
    
    U64 tag          = 0;
    U8  has_children = 0;
    cursor += str8_deserial_read_uleb128(raw_abbrev, cursor, &tag);
    cursor += str8_deserial_read_struct(raw_abbrev, cursor, &has_children);
    
    rd_printf("<%llx> %llu DW_Tag_%S %s", id_off, id, dw_string_from_tag_kind(temp.arena, tag), has_children ? "[has children]" : "[no children]");
    rd_indent();
    for (;;) {
      U64 attrib_off = cursor;
      
      U64 attrib_id = 0, form_id = 0;
      cursor += str8_deserial_read_uleb128(raw_abbrev, cursor, &attrib_id);
      cursor += str8_deserial_read_uleb128(raw_abbrev, cursor, &form_id);
      if (attrib_id == 0) {
        break;
      }
      String8 attrib_str = dw_string_from_attrib_kind(temp.arena, DW_Version_Last, DW_Ext_All, attrib_id);
      String8 form_str   = dw_string_from_form_kind(temp.arena, DW_Version_Last, form_id);
      rd_printf("<%llx> DW_Attrib_%-20S DW_Form_%S", attrib_off, attrib_str, form_str);
    }
    rd_unindent();
    
    temp_end(temp);
  }
  
  if (raw_abbrev.size) {
    rd_unindent();
  }
  
  scratch_end(scratch);
}

internal void
dw_print_debug_line(Arena *arena, String8List *out, String8 indent, DW_Input *input, DW_ListUnitInput lu_input, B32 relaxed)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# .debug_line");
  
  Rng1U64List unit_ranges = dw_unit_ranges_from_data(scratch.arena, input->sec[DW_Section_Line].data);
  for (Rng1U64Node *unit_range_n = unit_ranges.first; unit_range_n != 0; unit_range_n = unit_range_n->next) {
    Temp unit_temp = temp_begin(scratch.arena);
    
    String8         unit_data      = str8_substr(input->sec[DW_Section_Line].data, unit_range_n->v);
    String8         cu_dir         = {0};
    String8         cu_name        = {0};
    DW_ListUnit     cu_str_offsets = {0};
    DW_LineVMHeader line_vm        = {0};
    U64             line_vm_size   = dw_read_line_vm_header(unit_temp.arena, unit_data, 0, input, cu_dir, cu_name, line_vm.address_size, &cu_str_offsets, &line_vm);
    
    if (line_vm_size == 0) {
      continue;
    }
    
    {
      rd_printf("Header:", line_vm_size);
      rd_indent();
      String8 opcode_lengths = rd_format_hex_array(unit_temp.arena, line_vm.opcode_lens, line_vm.num_opcode_lens);
      rd_printf("Version:                 %u",    line_vm.version              );
      rd_printf("Line table offset:       %#llx", line_vm.unit_range.min       );
      rd_printf("Line table length:       %llu",  dim_1u64(line_vm.unit_range) );
      rd_printf("Version:                 %u",    line_vm.version              );
      rd_printf("Address size:            %u",    line_vm.address_size         );
      rd_printf("Segment selector size:   %u",    line_vm.segment_selector_size);
      rd_printf("Header length:           %llu",  line_vm.header_length        );
      rd_printf("Min instruction length:  %u",    line_vm.min_inst_len         );
      rd_printf("Max ops for instruction: %u",    line_vm.max_ops_for_inst     );
      rd_printf("Default Is Stmt:         %u",    line_vm.default_is_stmt      );
      rd_printf("Line base:               %d",    line_vm.line_base            );
      rd_printf("Line range:              %u",    line_vm.line_range           );
      rd_printf("Opcode base:             %u",    line_vm.opcode_base          );
      rd_printf("Opcode lengths:          %S",    opcode_lengths               );
      rd_unindent();
      rd_newline();
    }
    
    {
      rd_printf("Directory Table:");
      rd_indent();
      rd_printf("%-4s %-8s", "No.", "String");
      for (U64 dir_idx = 0; dir_idx < line_vm.dir_table.count; ++dir_idx) {
        rd_printf("%-4llu %S", dir_idx, line_vm.dir_table.v[dir_idx]);
      }
      rd_unindent();
      rd_newline();
    }
    
    {
      rd_printf("File Table:");
      rd_indent();
      rd_printf("%-4s %-8s %-8s %-33s %-8s %-8s", "No.", "DirIdx", "Time", "MD5", "Size", "Name");
      for (U64 file_idx = 0; file_idx < line_vm.file_table.count; ++file_idx) {
        DW_LineFile *file = &line_vm.file_table.v[file_idx];
        rd_printf("%-4llu %-8llu %-8llu %016llx-%016llx %-8llu %S",
                  file_idx,
                  file->dir_idx,
                  file->modify_time,
                  file->md5_digest.u64[1],
                  file->md5_digest.u64[0],
                  file->file_size,
                  file->file_name);
      }
      rd_unindent();
      rd_newline();
    }
    
    {
      rd_printf("Opcodes:");
      rd_indent();
      
      String8        opcodes    = str8_skip(unit_data, line_vm_size);
      B32            end_of_seq = 0;
      DW_LineVMState vm_state   = {0};
      dw_line_vm_reset(&vm_state, line_vm.default_is_stmt);
      
      for (U64 cursor = 0; cursor < opcodes.size; ) {
        Temp opcode_temp = temp_begin(unit_temp.arena);
        
        String8List opcode_fmt = {0};
        
        // opcode offset
        str8_list_pushf(opcode_temp.arena, &opcode_fmt, "[%08llx]", cursor);
        
        // parse opcode
        U8 opcode = 0;
        cursor += str8_deserial_read_struct(opcodes, cursor, &opcode);
        
        // push opcode id
        String8 opcode_str = dw_string_from_std_opcode(opcode_temp.arena, opcode);
        str8_list_push(arena, &opcode_fmt, opcode_str);
        
        // format operands
        switch (opcode) {
          default: {
            if (opcode >= line_vm.opcode_base) {
              U32 adjusted_opcode = 0;
              U32 op_advance      = 0;
              S32 line_advance    = 0;
              U64 addr_advance    = 0;
              if (line_vm.line_range > 0 && line_vm.max_ops_for_inst > 0) {
                adjusted_opcode = (U32)(opcode - line_vm.opcode_base);
                op_advance      = adjusted_opcode / line_vm.line_range;
                line_advance    = (S32)line_vm.line_base + ((S32)adjusted_opcode) % (S32)line_vm.line_range;
                addr_advance    = line_vm.min_inst_len * ((vm_state.op_index+op_advance) / line_vm.max_ops_for_inst);
              }
              
              vm_state.address        += addr_advance;
              vm_state.op_index        = (vm_state.op_index + op_advance) % line_vm.max_ops_for_inst;
              vm_state.line            = (U32)((S32)vm_state.line + line_advance);
              vm_state.basic_block     = 0;
              vm_state.prologue_end    = 0;
              vm_state.epilogue_begin  = 0;
              vm_state.discriminator   = 0;
              
              end_of_seq = 0;
              
              str8_list_pushf(opcode_temp.arena, &opcode_fmt, "advance line by %d, advance address by %lld", line_advance, addr_advance);
            } else {
              if (opcode > 0 && opcode <= line_vm.num_opcode_lens) {
                str8_list_pushf(opcode_temp.arena, &opcode_fmt, "skip operands:");
                U64 num_operands = line_vm.opcode_lens[opcode - 1];
                for (U8 i = 0; i < num_operands; i += 1){
                  U64 operand = 0;
                  cursor += str8_deserial_read_uleb128(opcodes, cursor, &operand);
                  str8_list_pushf(opcode_temp.arena, &opcode_fmt, " %llx", operand);
                }
              }
            }
          }break;
          
          case DW_StdOpcode_Copy: {
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "Line = %u, Column = %u, Address = %#llx", vm_state.line, vm_state.column, vm_state.address);
            end_of_seq = 0;
            vm_state.discriminator   = 0;
            vm_state.basic_block     = 0;
            vm_state.prologue_end    = 0;
            vm_state.epilogue_begin  = 0;
          } break;
          case DW_StdOpcode_AdvancePc: {
            U64 advance = 0;
            cursor += str8_deserial_read_uleb128(opcodes, cursor, &advance);
            dw_line_vm_advance(&vm_state, advance, line_vm.min_inst_len, line_vm.max_ops_for_inst);
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "advance %#llx ; current address %#llx", advance, vm_state.address);
          } break;
          
          case DW_StdOpcode_AdvanceLine: {
            S64 advance = 0;
            cursor += str8_deserial_read_sleb128(opcodes, cursor, &advance);
            vm_state.line += advance;
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "advance %lld ; current line %u", advance, vm_state.line);
          } break;
          
          case DW_StdOpcode_SetFile: {
            U64 file_idx = 0;
            cursor += str8_deserial_read_uleb128(opcodes, cursor, &file_idx);
            vm_state.file_index = file_idx;
            
            String8 path = dw_path_from_file_idx(opcode_temp.arena, &line_vm, file_idx);
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu \"%S\"", file_idx, path);
          } break;
          
          case DW_StdOpcode_SetColumn: {
            U64 column = 0;
            cursor += str8_deserial_read_uleb128(opcodes, cursor, &column);
            vm_state.column = column;
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu", column);
          } break;
          
          case DW_StdOpcode_NegateStmt: {
            vm_state.is_stmt = !vm_state.is_stmt;
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "is_stmt = %u", vm_state.is_stmt);
          } break;
          
          case DW_StdOpcode_SetBasicBlock: {
            vm_state.basic_block = 1;
          } break;
          
          case DW_StdOpcode_ConstAddPc: {
            U64 advance = (0xffu - line_vm.opcode_base)/line_vm.line_range;
            dw_line_vm_advance(&vm_state, advance, line_vm.min_inst_len, line_vm.max_ops_for_inst);
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%lld ; address %#llx", advance, vm_state.address);
          }break;
          
          case DW_StdOpcode_FixedAdvancePc: {
            U64 operand = 0;
            cursor += str8_deserial_read_struct(opcodes, cursor, &operand);
            vm_state.address += operand;
            vm_state.op_index = 0;
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu", operand);
          } break;
          
          case DW_StdOpcode_SetPrologueEnd: {
            vm_state.prologue_end = 1;
          } break;
          
          case DW_StdOpcode_SetEpilogueBegin: {
            vm_state.epilogue_begin = 1;
          } break;
          
          case DW_StdOpcode_SetIsa: {
            U64 v = 0;
            cursor += str8_deserial_read_uleb128(opcodes, cursor, &v);
            vm_state.isa = v;
            str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%llu", v);
          } break;
          
          case DW_StdOpcode_ExtendedOpcode: {
            U64 length = 0;
            U8 ext_opcode = 0;
            
            cursor += str8_deserial_read_uleb128(opcodes, cursor, &length);
            U64 opcode_end = cursor + length;
            
            cursor += str8_deserial_read_struct(opcodes, cursor, &ext_opcode);
            
            String8 ext_opcode_str = dw_string_from_ext_opcode(opcode_temp.arena, ext_opcode);
            //str8_list_pushf(opcode_temp.arena, &opcode_fmt, "length: %u", length);
            str8_list_push(opcode_temp.arena, &opcode_fmt, ext_opcode_str);
            switch (ext_opcode) {
              case DW_ExtOpcode_EndSequence: {
                vm_state.end_sequence = 1;
                dw_line_vm_reset(&vm_state, line_vm.default_is_stmt);
                end_of_seq = 1;
              } break;
              case DW_ExtOpcode_SetAddress: {
                U64 address = 0;
                cursor += str8_deserial_read(opcodes, cursor, &address, line_vm.address_size, line_vm.address_size);
                vm_state.address    = address;
                vm_state.op_index   = 0;
                str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%#llx", address);
              } break;
              case DW_ExtOpcode_DefineFile: {
                String8 file_name = {0};
                cursor += str8_deserial_read_cstr(opcodes, cursor, &file_name);
                
                U64 dir_idx = 0, modify_time = 0, file_size = 0;
                cursor += str8_deserial_read_uleb128(opcodes, cursor, &dir_idx);
                cursor += str8_deserial_read_uleb128(opcodes, cursor, &modify_time);
                cursor += str8_deserial_read_uleb128(opcodes, cursor, &file_size);
                str8_list_pushf(opcode_temp.arena, &opcode_fmt, "%S Dir: %llu, Time: %llu, Size: %llu", file_name, dir_idx, modify_time, file_size);
              } break;
              case DW_ExtOpcode_SetDiscriminator: {
                U64 v = 0;
                cursor += str8_deserial_read_uleb128(opcodes, cursor, &v);
                vm_state.discriminator = v;
                str8_list_pushf(arena, &opcode_fmt, "%llu", v);
              } break;
            }
            
            cursor = opcode_end;
          } break;
        }
        
        String8 string = str8_list_join(opcode_temp.arena, &opcode_fmt, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", string);
        
        temp_end(opcode_temp);
      }
      
      rd_unindent();
      rd_newline();
    }
    
    temp_end(unit_temp);
  }
  
  scratch_end(scratch);
}

internal void
dw_print_debug_str(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  String8 data = input->sec[DW_Section_Str].data;
  rd_printf("# %S", input->sec[DW_Section_Str].name);
  rd_indent();
  for (U64 cursor = 0, read_size = 0; cursor < data.size; cursor += read_size) {
    String8 string = {0};
    read_size = str8_deserial_read_cstr(data, cursor, &string);
    rd_printf("[%08llx] { %llu, \"%S\" }", cursor, string.size, string);
  }
  rd_unindent();
}

internal void
dw_print_debug_loc(Arena *arena, String8List *out, String8 indent, DW_Input *input, Arch arch, ImageType image_type, B32 relaxed)
{
#if 0
  DW_Section info = input->sec[DW_Section_Info];
  DW_Section loc  = input->sec[DW_Section_Loc];
  
  if (loc.data.size == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", input->sec[DW_Section_Loc].name);
  rd_indent();
  
  // TODO: warn about overlaps in ranges
  
  Rng1U64List cu_range_list = dw_comp_unit_ranges_from_info(scratch.arena, info);
  
  // parse debug_info for attributes with LOCLIST and store .debug_loc offsets 
  U64List    *loc_lists     = push_array(scratch.arena, U64List,    cu_range_list.count);
  U64        *address_sizes = push_array(scratch.arena, U64,        cu_range_list.count);
  U64        *address_bases = push_array(scratch.arena, U64,        cu_range_list.count);
  U64        *cu_bases      = push_array(scratch.arena, U64,        cu_range_list.count);
  DW_Version *ver_arr       = push_array(scratch.arena, DW_Version, cu_range_list.count);
  DW_Ext     *ext_arr       = push_array(scratch.arena, DW_Ext,     cu_range_list.count);
  
  U64 comp_idx = 0;
  for (Rng1U64Node *cu_range_n = cu_range_list.first; cu_range_n != 0; cu_range_n = cu_range_n->next, ++comp_idx) {
    Temp comp_temp = temp_begin(arena);
    
    Rng1U64     cu_range = cu_range_n->v;
    DW_CompUnit cu       = dw_comp_unit_from_info_off(comp_temp.arena, input, cu_range.min, relaxed);
    
    // store info about comp unit
    address_sizes[comp_idx] = cu.address_size;
    address_bases[comp_idx] = cu.base_addr;
    ver_arr[comp_idx]       = cu.version;
    cu_bases[comp_idx]      = cu_range_n->v.min;
    
    // parse tags
    for (U64 info_off = cu.tags_range.min; info_off < cu.tags_range.max; /* empty */) {
      Temp tag_temp = temp_begin(scratch.arena);
      
      DW_Tag tag = dw_tag_from_info_offset_cu(tag_temp.arena, input, &cu, ext_arr[comp_idx], info_off);
      
      // parse attribs
      for (DW_AttribNode *attrib_node = tag.attribs.first; attrib_node != 0; attrib_node = attrib_node->next) {
        DW_Attrib *attrib         = &attrib_node->v;
        B32        is_sect_offset = attrib->value_class == DW_AttribClass_LocListPtr || (attrib->value_class == DW_AttribClass_LocList && attrib->form_kind == DW_Form_SecOffset);
        B32        is_sect_index  = attrib->value_class == DW_AttribClass_LocList && attrib->form_kind == DW_Form_LocListx;
        if (is_sect_offset) {
          u64_list_push(scratch.arena, &loc_lists[comp_idx], attrib->value.v[0]);
        } else if (is_sect_index) {
          // TODO: support for section indexing
        }
      }
      
      // advance to next tag
      info_off = tag.next_info_off;
      
      temp_end(tag_temp);
    }
    
    temp_end(comp_temp);
  }
  
  void    *base  = dw_base_from_sec(input, DW_Section_Loc);
  Rng1U64  range = dw_range_from_sec(input, DW_Section_Loc);
  
  rd_printf(".debug_loc");
  rd_indent();
  rd_printf("%-8s %-8s %-8s %s", "Offset", "Min", "Max", "Expression");
  for (U32 comp_idx = 0; comp_idx < cu_range_list.count; ++comp_idx) {
    Temp locs_temp = temp_begin(scratch.arena);
    
    DW_Version ver = ver_arr[comp_idx];
    DW_Ext     ext = ext_arr[comp_idx];
    
    U64Array locs = u64_array_from_list(locs_temp.arena, &loc_lists[comp_idx]);
    u64_array_sort(locs.count, locs.v);
    
    U64Array locs_set      = remove_duplicates_u64_array(locs_temp.arena, locs);
    U64      address_size  = address_sizes[comp_idx];
    U64      base_selector = (address_size == 8) ? max_U64 : max_U32;
    
    for (U64 loc_idx = 0; loc_idx < locs_set.count; ++loc_idx) {
      U64 base_address = address_bases[comp_idx];
      for (U64 cursor = locs_set.v[loc_idx]; cursor < dim_1u64(range); /* empty */) {
        Temp range_temp = temp_begin(arena);
        
        String8List list = {0};
        
        // offset
        str8_list_pushf(range_temp.arena, &list, "%08llx", cursor);
        
        // parse entry
        U64 v0 = 0, v1 = 0;
        cursor += dw_based_range_read(base, range, cursor, address_size, &v0);
        cursor += dw_based_range_read(base, range, cursor, address_size, &v1);
        
        B32 is_list_end = v0 == 0 && v1 == 0;
        if (is_list_end) {
          str8_list_pushf(range_temp.arena, &list, "<LIST END>");
        } else if (v0 == base_selector) {
          base_address = v1;
        } else {
          U16 expr_size = 0;
          cursor += dw_based_range_read_struct(base, range, cursor, &expr_size);
          Rng1U64 expr_range = rng_1u64(range.min+cursor, range.min+cursor+expr_size);
          cursor += expr_size;
          
          // format dwarf expression
          B32     is_dwarf64 = (address_size == 8);
          String8 raw_expr   = str8((U8*)base+expr_range.min, dim_1u64(expr_range));
          String8 expression = dw_format_expression_single_line(range_temp.arena, raw_expr, cu_bases[comp_idx], address_size, arch, ver, ext, input->sec[DW_Section_Loc].mode);
          
          // push entry
          U64 min = base_address + v0;
          U64 max = base_address + v1;
          str8_list_pushf(range_temp.arena, &list, "%08llx %08llx %S", min, max, expression);
        }
        
        // print entry
        String8 print = str8_list_join(range_temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", print);
        
        // cleanup temp
        temp_end(range_temp);
        
        // exit check
        if (is_list_end) {
          break;
        }
      }
    }
    
    temp_end(locs_temp);
  }
  rd_unindent();
  
  rd_unindent();
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_ranges(Arena *arena, String8List *out, String8 indent, DW_Input *input, Arch arch, ImageType image_type, B32 relaxed)
{
  NotImplemented;
#if 0
  DW_Section  ranges = input->sec[DW_Section_Ranges];
  void       *base   = dw_base_from_sec(input, DW_Section_Ranges);
  Rng1U64     range  = dw_range_from_sec(input, DW_Section_Ranges);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  Rng1U64List  cu_range_list = dw_comp_unit_ranges_from_info(scratch.arena, sections->v[DW_Section_Info]);
  
  // parse debug_info for attributes with LOCLIST and store .debug_loc offsets 
  U64List *loc_lists     = push_array(scratch.arena, U64List, cu_range_list.count);
  U64     *address_sizes = push_array(scratch.arena, U64,     cu_range_list.count);
  U64     *address_bases = push_array(scratch.arena, U64,     cu_range_list.count);
  
  {
    U64 comp_idx = 0;
    for (Rng1U64Node *cu_range_n = cu_range_list.first; cu_range_n != 0; cu_range_n = cu_range_n->next, ++comp_idx) {
      Rng1U64     cu_range = cu_range_n->v;
      DW_CompUnit cu       = dw_comp_unit_from_info_offset(scratch.arena, sections, cu_range.min, relaxed);
      
      // store info about comp unit
      address_sizes[comp_idx] = cu.address_size;
      address_bases[comp_idx] = cu.base_addr;
      
      // parse tags
      for (U64 info_off = cu.tags_range.min; info_off < cu.tags_range.max; /* empty */) {
        DW_Tag tag = dw_tag_from_info_offset_cu(scratch.arena, sections, &cu, info_off);
        
        // parse attribs
        for (DW_AttribNode *attrib_node = tag.attribs.first; attrib_node != 0; attrib_node = attrib_node->next) {
          DW_Attrib *attrib         = &attrib_node->v;
          B32        is_sect_offset = attrib->value_class == DW_AttribClass_RngListPtr || (attrib->value_class == DW_AttribClass_RngList && attrib->form_kind == DW_Form_SecOffset);
          B32        is_sect_index  = attrib->value_class == DW_AttribClass_RngList && attrib->form_kind == DW_Form_RngListx;
          if (is_sect_offset) {
            u64_list_push(scratch.arena, &loc_lists[comp_idx], attrib->value.v[0]);
          } else if (is_sect_index) {
            // TODO: support for section indexing
          }
        }
        
        info_off = tag.next_info_off;
      }
    }
  }
  
  rd_printf("# %S", sections->v[DW_Section_Ranges].name);
  rd_indent();
  rd_printf("%-8s %-8s %-8s", "Offset", "Min", "Max");
  for (U32 comp_idx = 0; comp_idx < cu_range_list.count; ++comp_idx) {
    U64Array locs = u64_array_from_list(scratch.arena, &loc_lists[comp_idx]);
    u64_array_sort(locs.count, locs.v);
    U64Array locs_set      = remove_duplicates_u64_array(scratch.arena, locs);
    U64      address_size  = address_sizes[comp_idx];
    U64      base_selector = (address_size == 8) ? max_U64 : max_U32;
    
    for (U64 loc_idx = 0; loc_idx < locs_set.count; ++loc_idx) {
      U64 base_address = address_bases[comp_idx];
      for (U64 cursor = locs_set.v[loc_idx]; cursor < dim_1u64(range); /* empty */) {
        Temp range_temp = temp_begin(scratch.arena);
        
        String8List list = {0};
        
        // offset
        str8_list_pushf(range_temp.arena, &list, "%08llx", cursor);
        
        // parse entry
        U64 v0 = 0, v1 = 0;
        cursor += dw_based_range_read(base, range, cursor, address_size, &v0);
        cursor += dw_based_range_read(base, range, cursor, address_size, &v1);
        
        B32 is_list_end = v0 == 0 && v1 == 0;
        if (is_list_end) {
          str8_list_pushf(range_temp.arena, &list, "<LIST END>");
        } else if (v0 == base_selector) {
          base_address = v1;
        } else {
          // push entry
          U64 min = base_address + v0;
          U64 max = base_address + v1;
          str8_list_pushf(range_temp.arena, &list, "%08llx %08llx", min, max);
        }
        
        // print entry
        String8 print = str8_list_join(range_temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", print);
        
        temp_end(range_temp);
        
        // exit check
        if (is_list_end) {
          break;
        }
      }
    }
  }
#endif
}

internal void
dw_print_debug_aranges(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_ARanges);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_ARanges);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_ARanges].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64        unit_length           = 0;
    DW_Version version               = 0;
    U64        debug_info_offset     = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64 unit_opl = cursor + unit_length;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    
    B32 is_dwarf64 = unit_length >= max_U32;
    U64 int_size   = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    cursor += dw_based_range_read(base, range, cursor, int_size, &debug_info_offset);
    
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    
    U64 tuple_size                  = address_size * 2 + segment_selector_size;
    U64 bytes_too_far_past_boundary = cursor % tuple_size;
    if (bytes_too_far_past_boundary > 0) {
      cursor += tuple_size - bytes_too_far_past_boundary;
    }
    
    rd_printf("Unit length:           %llu",  unit_length);
    rd_printf("Version:               %u",    version);
    rd_printf("Debug info offset:     %#llx", debug_info_offset);
    rd_printf("Address size:          %u",    address_size);
    rd_printf("Segment selector size: %u",    segment_selector_size);
    
    if (version != DW_Version_2) {
      rd_warningf("Version value must be 2 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Range");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(arena);
      
      String8List list = {0};
      
      str8_list_pushf(temp.arena, &list, "%08llx", cursor);
      
      U64 segment_selector = 0;
      U64 address          = 0;
      U64 length           = 0;
      cursor += dw_based_range_read(base, range, cursor, segment_selector_size, &segment_selector);
      cursor += dw_based_range_read(base, range, cursor, address_size, &address);
      cursor += dw_based_range_read(base, range, cursor, address_size, &length);
      
      if (segment_selector == 0 && address == 0 && length == 0) {
        str8_list_pushf(temp.arena, &list, "<LIST END>");
      } else {
        if (segment_selector != 0) {
          str8_list_pushf(temp.arena, &list, "%02llu:", segment_selector);
        }
        str8_list_pushf(temp.arena, &list, "%llx-%llx", address, address+length);
      }
      
      String8 print = str8_list_join(temp.arena, &list, &(StringJoin){.sep=str8_lit(" ") });
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_addr(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_Addr);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_Addr);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_Addr].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64        unit_length           = 0;
    DW_Version version               = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    
    U64 unit_offset = cursor;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    
    U64 unit_opl = cursor + unit_length;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    
    U64 tuple_size                  = address_size * 2 + segment_selector_size;
    U64 bytes_too_far_past_boundary = cursor % tuple_size;
    if (bytes_too_far_past_boundary > 0) {
      cursor += tuple_size - bytes_too_far_past_boundary;
    }
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:               %u", version);
    rd_printf("Address size:          %u", address_size);
    rd_printf("Segment selector size: %u", segment_selector_size);
    
    if (version != DW_Version_2) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Address");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(arena);
      
      String8List list = {0};
      
      str8_list_pushf(temp.arena, &list, "%08X", cursor);
      
      U64 segment_selector = 0;
      U64 address          = 0;
      cursor += dw_based_range_read(base, range, cursor, segment_selector_size, &segment_selector);
      cursor += dw_based_range_read(base, range, cursor, address_size, &address);
      
      if (segment_selector == 0 && address == 0) {
        str8_list_pushf(temp.arena, &list, "<LIST END>");
      } else {
        if (segment_selector != 0) {
          str8_list_pushf(temp.arena, &list, "%02u:", segment_selector);
        }
        str8_list_pushf(temp.arena, &list, "%llx", address);
      }
      
      String8 print = str8_list_join(arena, &list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal U64
dw_based_range_read_address(void *base, Rng1U64 range, U64 offset, Rng1U64Array segment_ranges, U8 segment_selector_size, U8 address_size, U64 *address_out)
{
  U64 read_offset = offset;
  
  // read segment
  U64 segment_selector = 0;
  read_offset += dw_based_range_read(base, range, read_offset, segment_selector_size, &segment_selector);
  
  // read address
  U64 address = 0;
  read_offset += dw_based_range_read(base, range, read_offset, address_size, &address);
  
  // apply segment offset
  B32 is_address_segment_relative = segment_selector_size > 0;
  if (is_address_segment_relative) {
    if (segment_selector < segment_ranges.count) {
      address += segment_ranges.v[segment_selector].min;
    } else {
      Assert(!"invalid segment selector");
    }
  }
  
  U64 read_size = (read_offset - offset);
  return read_size;
}

internal void
dw_print_debug_loclists(Arena *arena, String8List *out, String8 indent, DW_Input *input, Rng1U64Array segment_virtual_ranges, Arch arch)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_LocLists);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_LocLists);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_LocLists].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    
    U64        unit_opl              = cursor + unit_length;
    DW_Version version               = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    U32        offset_entry_count    = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &offset_entry_count);
    
    U64 past_header_offset = cursor;
    B32 is_dwarf64         = unit_length > max_U32;
    U64 offset_size        = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:               %u", version);
    rd_printf("Address size:          %u", address_size);
    rd_printf("Segment selector size: %u", segment_selector_size);
    rd_printf("Offset entry count:    %u", offset_entry_count);
    if (version != DW_Version_5) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    if (offset_entry_count > 0) {
      rd_printf("Offsets:");
      rd_indent();
      rd_printf("%-8s %-8s", "Index", "Offset");
      for (U64 offset_idx = 0; offset_idx < offset_entry_count; ++offset_idx) {
        U64 offset = 0;
        cursor += dw_based_range_read(base, range, cursor, offset_size, &offset);
        rd_printf("%-8llu %llx", offset_idx, offset+past_header_offset);
      }
      rd_unindent();
    }
    
    rd_printf("Locations:");
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Location");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(arena);
      
      String8List list = {0};
      
      str8_list_pushf(temp.arena, &list, "%08llx", cursor);
      
      U8 kind = 0;
      cursor += dw_based_range_read_struct(base, range, cursor, &kind);
      str8_list_pushf(temp.arena, &list, "DW_LLE_%S", dw_string_from_loc_list_entry_kind(temp.arena, kind));
      
      B32 has_loc_desc = 0;
      switch (kind) {
        case DW_LocListEntryKind_EndOfList:
        break;
        case DW_LocListEntryKind_DefaultLocation: {
          has_loc_desc = 1;
        } break;
        case DW_LocListEntryKind_BaseAddress: {
          U64 base_address = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &base_address);
          str8_list_pushf(temp.arena, &list, "%llx", base_address);
        } break;
        case DW_LocListEntryKind_StartLength: {
          U64 start  = 0;
          U64 length = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, length);
        } break;
        case DW_LocListEntryKind_StartEnd: {
          U64 start = 0;
          U64 end   = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_address(base, range, cursor, segment_virtual_ranges, segment_selector_size, address_size, &end);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, end);
        } break;
        case DW_LocListEntryKind_BaseAddressX: {
          U64 base_addressx = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &base_addressx);
          str8_list_pushf(temp.arena, &list, "%llx", base_addressx);
        } break;
        case DW_LocListEntryKind_StartXEndX: {
          U64 startx = 0;
          U64 endx   = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &endx);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", startx, endx);
        } break;
        case DW_LocListEntryKind_OffsetPair: {
          U64 a = 0;
          U64 b = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &a);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &b);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", a, b);
          
          U8 expr_length = 0;
          cursor += dw_based_range_read_struct(base, range, cursor, &expr_length); 
          
          String8 raw_expr = str8((U8*)base+cursor, expr_length);
          cursor += expr_length;
          
          // TODO: we need actual cu base to format expression correctly
          NotImplemented;
          String8 expression = dw_format_expression_single_line(temp.arena, raw_expr, 0, address_size, arch, version, DW_Ext_Null, is_dwarf64);
          str8_list_pushf(temp.arena, &list, "(%S)", expression);
        } break;
        case DW_LocListEntryKind_StartXLength: {
          U64 startx = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          U64 length = 0;
          if (version < DW_Version_5) {
            // pre-standard length
            cursor += dw_based_range_read(base, range, cursor, sizeof(U32), &length);
          } else {
            cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          }
        } break;
      }
      
      String8 print = str8_list_join(temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_rnglists(Arena *arena, String8List *out, String8 indent, DW_Input *input, Rng1U64Array segment_ranges)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_RngLists);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_RngLists);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_RngLists].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64        unit_opl              = cursor + unit_length;
    DW_Version version               = 0;
    U8         address_size          = 0;
    U8         segment_selector_size = 0;
    U32        offset_entry_count    = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    cursor += dw_based_range_read_struct(base, range, cursor, &address_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &segment_selector_size);
    cursor += dw_based_range_read_struct(base, range, cursor, &offset_entry_count);
    
    U64 past_header_offset = cursor;
    B32 is_dwarf64         = unit_length > max_U32;
    U64 offset_size        = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:               %u", version);
    rd_printf("Address size:          %u", address_size);
    rd_printf("Segment selector size: %u", segment_selector_size);
    rd_printf("Offset entry count:    %u", offset_entry_count);
    
    if (version != DW_Version_5) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    if (offset_entry_count > 0) {
      rd_printf("Offsets:");
      rd_indent();
      rd_printf("%-8s %-8s", "Index", "Offset");
      for (U64 offset_idx = 0; offset_idx < offset_entry_count; ++offset_idx) {
        U64 offset = 0;
        cursor += dw_based_range_read(base, range, cursor, offset_size, &offset);
        rd_printf("%-8llu %llx", offset_idx, offset+past_header_offset);
      }
      rd_unindent();
    }
    
    rd_printf("Ranges:");
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "Range");
    for (; cursor < unit_opl; ) {
      Temp temp = temp_begin(scratch.arena);
      
      String8List list = {0};
      
      // offset
      str8_list_pushf(temp.arena, &list, "%08llx", cursor);
      
      // opcode mnemonic
      U8 kind = 0;
      cursor += dw_based_range_read_struct(base, range, cursor, &kind);
      str8_list_pushf(temp.arena, &list, "DW_RLE_%S", dw_string_from_rng_list_entry_kind(temp.arena, kind));
      
      // operand
      switch (kind) {
        case DW_RngListEntryKind_EndOfList: {
          // empty
        } break;
        case DW_RngListEntryKind_BaseAddressX:  {
          U64 base_addressx = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &base_addressx);
          str8_list_pushf(temp.arena, &list, "%llx", base_addressx);
        } break;
        case DW_RngListEntryKind_BaseAddress: {
          U64 base_address = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &base_address);
          str8_list_pushf(temp.arena, &list, "%llx", base_address);
        } break;
        case DW_RngListEntryKind_OffsetPair: {
          U64 min = 0;
          U64 max = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &min);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &max);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", min, max);
        } break;
        case DW_RngListEntryKind_StartxLength: {
          U64 startx = 0;
          U64 length = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", startx, length);
        } break;
        case DW_RngListEntryKind_StartxEndx: {
          U64 startx = 0;
          U64 endx   = 0;
          cursor += dw_based_range_read_uleb128(base, range, cursor, &startx);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &endx);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", startx, endx);
        } break;
        case DW_RngListEntryKind_StartEnd: {
          U64 start = 0;
          U64 end = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &end);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, end);
        } break;
        case DW_RngListEntryKind_StartLength: {
          U64 start = 0;
          U64 length = 0;
          cursor += dw_based_range_read_address(base, range, cursor, segment_ranges, segment_selector_size, address_size, &start);
          cursor += dw_based_range_read_uleb128(base, range, cursor, &length);
          str8_list_pushf(temp.arena, &list, "%llx, %llx", start, length);
        } break;
      }
      
      // output row
      String8 print = str8_list_join(temp.arena, &list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", print);
      
      temp_end(temp);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_format_string_table(Arena *arena, String8List *out, String8 indent, DW_Input *input, DW_SectionKind sec)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, sec);
  Rng1U64  range = dw_range_from_sec(sections, sec);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[sec].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64 unit_opl = cursor + unit_length;
    
    DW_Version version = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    
    if (version != DW_Version_2) {
      rd_warningf("Version value must be 2");
    }
    
    B32 is_dwarf64      = unit_length > max_U32;
    U32 sec_offset_size = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    U64 debug_info_offset = 0, debug_info_length = 0;
    cursor += dw_based_range_read(base, range, cursor, sec_offset_size, &debug_info_offset);
    cursor += dw_based_range_read(base, range, cursor, sec_offset_size, &debug_info_length);
    
    rd_printf("Unit @ %#llx, length %llu", unit_offset, unit_length);
    rd_printf("Version:           %u",            version);
    rd_printf("Debug info offset: %#llx",         debug_info_offset);
    rd_printf("Debug info length: %#llx",         debug_info_length);
    
    rd_printf("Entries:");
    rd_indent();
    rd_printf("%-8s %-8s", "Offset", "String");
    for (; cursor < unit_opl; ) {
      U64 info_offset = 0;
      cursor += dw_based_range_read(base, range, cursor, sec_offset_size, &info_offset);
      String8 string = dw_based_range_read_string(base, range, cursor);
      cursor += (string.size + 1);
      
      rd_printf("%08llx %S", info_offset, string);
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_pubnames(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  dw_format_string_table(arena, out, indent, input, DW_Section_PubNames);
}

internal void
dw_print_debug_pubtypes(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  dw_format_string_table(arena, out, indent, input, DW_Section_PubTypes);
}

internal void
dw_print_debug_line_str(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_LineStr);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_LineStr);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_LineStr].name);
  rd_indent();
  
  rd_printf("%-8s %-8s", "Offset", "String");
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 offset = cursor;
    String8 string = dw_based_range_read_string(base, range, cursor);
    cursor += (string.size + 1);
    rd_printf("%08llX %S", offset, string);
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_print_debug_str_offsets(Arena *arena, String8List *out, String8 indent, DW_Input *input)
{
  NotImplemented;
#if 0
  void    *base  = dw_base_from_sec(sections, DW_Section_StrOffsets);
  Rng1U64  range = dw_range_from_sec(sections, DW_Section_StrOffsets);
  
  void    *debug_str_base  = dw_base_from_sec(sections, DW_Section_Str);
  Rng1U64  debug_str_range = dw_range_from_sec(sections, DW_Section_Str);
  
  if (dim_1u64(range) == 0) {
    return;
  }
  
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# %S", sections->v[DW_Section_StrOffsets].name);
  rd_indent();
  for (U64 cursor = 0; cursor < dim_1u64(range); ) {
    U64 unit_offset = cursor;
    
    U64 unit_length = 0;
    cursor += dw_based_range_read_length(base, range, cursor, &unit_length);
    U64 unit_opl = cursor + unit_length;
    
    DW_Version version = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &version);
    if (version != DW_Version_5) {
      rd_warningf("Version value must be 5 (DWARF5 sepc, Feb 13, 2017)");
    }
    
    U16 padding = 0;
    cursor += dw_based_range_read_struct(base, range, cursor, &padding);
    if (padding != 0) {
      rd_warningf("unexpected padding byte");
    }
    
    B32 is_dwarf64  = unit_length > max_U32;
    U32 offset_size = is_dwarf64 ? sizeof(U64) : sizeof(U32);
    
    rd_printf("Unit @ %#llX, length %lld", unit_offset, unit_length);
    rd_printf("Version: %d", version);
    rd_printf("Padding: %d", padding);
    rd_indent();
    rd_printf("%-8s %-8s", "@", "Offset");
    for (; cursor < unit_opl; ) {
      U64 read_at = cursor;
      U64 offset  = 0;
      cursor += dw_based_range_read(base, range, cursor, offset_size, &offset);
      rd_printf("%08llx %08llx", read_at, offset);
      if (dim_1u64(debug_str_range) > 0) {
        String8 string = dw_based_range_read_string(debug_str_base, debug_str_range, offset);
        rd_printf(" %S", string);
      }
      rd_newline();
    }
    rd_unindent();
    rd_newline();
  }
  rd_unindent();
  
  scratch_end(scratch);
#endif
}

internal void
dw_format(Arena *arena, String8List *out, String8 indent, RD_Option opts, DW_Input *input, Arch arch, ImageType image_type)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  Rng1U64Array segment_vranges = {0};
  
  DW_ListUnitInput lu_input = dw_list_unit_input_from_input(scratch.arena, input);
  B32              relaxed  = !!(opts & RD_Option_RelaxDwarfParser);
  
  if (opts & RD_Option_DebugInfo) {
    dw_print_debug_info(arena, out, indent, input, lu_input, arch, relaxed);
  }
  if (opts & RD_Option_DebugAbbrev) {
    dw_print_debug_abbrev(arena, out, indent, input);
  }
  if (opts & RD_Option_DebugLine) {
    dw_print_debug_line(arena, out, indent, input, lu_input, relaxed);
  }
  if (opts & RD_Option_DebugStr) {
    dw_print_debug_str(arena, out, indent, input);
  }
  if (opts & RD_Option_DebugLoc) {
    dw_print_debug_loc(arena, out, indent, input, arch, image_type, relaxed);
  }
  if (opts & RD_Option_DebugRanges) {
    dw_print_debug_ranges(arena, out, indent, input, arch, image_type, relaxed);
  }
  if (opts & RD_Option_DebugARanges) {
    dw_print_debug_aranges(arena, out, indent, input);
  }
  if (opts & RD_Option_DebugAddr) {
    dw_print_debug_addr(arena, out, indent, input);
  }
  if (opts & RD_Option_DebugLocLists) {
    dw_print_debug_loclists(arena, out, indent, input, segment_vranges, arch);
  }
  if (opts & RD_Option_DebugRngLists) {
    dw_print_debug_rnglists(arena, out, indent, input, segment_vranges);
  }
  if (opts & RD_Option_DebugPubNames) {
    dw_print_debug_pubnames(arena, out, indent, input);
  }
  if (opts & RD_Option_DebugPubTypes) {
    dw_print_debug_pubtypes(arena, out, indent, input);
  }
  if (opts & RD_Option_DebugLineStr) {
    dw_print_debug_line_str(arena, out, indent, input);
  }
  if (opts & RD_Option_DebugStrOffsets) {
    dw_print_debug_str_offsets(arena, out, indent, input);
  }
  
  scratch_end(scratch);
}

// CodeView

internal String8
cv_string_from_reg_off(Arena *arena, CV_Arch arch, U32 reg_idx, U32 reg_off)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 reg_str = cv_string_from_reg_id(scratch.arena, arch, reg_idx);
  String8 result  = rd_string_from_reg_off(arena, reg_str, reg_off);
  scratch_end(scratch);
  return result;
}

internal void
cv_print_binary_annots(Arena *arena, String8List *out, String8 indent, CV_Arch arch, String8 raw_data)
{
  if (raw_data.size) {
    Temp scratch = scratch_begin(&arena, 1);
    
    rd_printf("Binary Annotations:");
    rd_indent();
    
    U64 cursor = 0;
    for (; cursor < raw_data.size; ) {
      String8List op_list = {0};
      
      U8 op;
      cursor += str8_deserial_read_struct(raw_data, cursor, &op);
      if (op == CV_InlineBinaryAnnotation_Null) {
        break;
      }
      
      U8 params[2];
      U32 param_count = (op == CV_InlineBinaryAnnotation_ChangeCodeOffsetAndLineOffset) ? 2 : 1;
      cursor += str8_deserial_read_array(raw_data, cursor, &params[0], param_count);
      
      String8 opcode_str = cv_string_from_binary_opcode(op);
      str8_list_pushf(scratch.arena, &op_list, "%S", opcode_str);
      for (U32 i = 0; i < param_count; ++i) {
        str8_list_pushf(scratch.arena, &op_list, " %x", params[i]);
      }
      
      String8 op_str = str8_list_join(scratch.arena, &op_list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", op_str);
    }
    rd_unindent();
    
    rd_printf("Binary Annotations Length: %u bytes (%u bytes padding)", raw_data.size, raw_data.size - cursor);
    
    scratch_end(scratch);
  }
}

internal void
cv_print_lvar_addr_range(Arena *arena, String8List *out, String8 indent, CV_LvarAddrRange range)
{
  rd_printf("Address Range: %04x:%08x Size: %#x", range.sec, range.off, range.len);
}

internal void
cv_print_lvar_addr_gap(Arena *arena, String8List *out, String8 indent, String8 raw_data)
{
  U64 count = raw_data.size / sizeof(CV_LvarAddrGap);
  if (count > 0) {
    U64 cursor = 0;
    rd_printf("# Address Gaps");
    rd_indent();
    for (U64 i = 0; i < count; ++i) {
      CV_LvarAddrGap gap = {0};
      cursor += str8_deserial_read_struct(raw_data, cursor, &gap);
      rd_printf("Off: %#x, Len %#x", gap.off, gap.len);
    }
    rd_unindent();
  }
}

internal void
cv_print_lvar_attr(Arena *arena, String8List *out, String8 indent, CV_LocalVarAttr attr)
{
  Temp scratch = scratch_begin(&arena,1);
  rd_printf("Address: %S", cv_string_sec_off(scratch.arena, attr.seg, attr.off));
  rd_printf("Flags:   %S", cv_string_from_local_flags(scratch.arena, attr.flags));
  scratch_end(scratch);
}

internal void
cv_print_symbol(Arena *arena, String8List *out, String8 indent, CV_Arch arch, CV_TypeIndex min_itype, CV_SymKind type, String8 raw_symbol)
{
  Temp scratch = scratch_begin(&arena, 1);
  U64 cursor = 0;
  switch (type) {
    case CV_SymKind_THUNK32_ST:
    case CV_SymKind_THUNK32: {
      CV_SymThunk32 sym  = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name   : %S",  name);
      rd_printf("Parent : %#x", sym.parent);
      rd_printf("End    : %#x", sym.end);
      rd_printf("Next   : %#x", sym.next);
      rd_printf("Address: %S",  cv_string_sec_off(scratch.arena, sym.sec, sym.off));
      rd_printf("Length : %u",  sym.len);
      rd_printf("Ordinal: %S",  cv_string_from_thunk_ordinal(sym.ord));
    } break;
    case CV_SymKind_FILESTATIC: {
      CV_SymFileStatic sym   = {0};
      String8          name = str8_zero();
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      rd_printf("Name : %S", name);
      rd_printf("Type : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("Flags: %S", cv_string_from_local_flags(scratch.arena, sym.flags));
    } break;
    case CV_SymKind_CALLERS:
    case CV_SymKind_CALLEES: {
      CV_SymFunctionList sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      CV_TypeIndex *funcs = push_array(scratch.arena, CV_TypeIndex, sym.count);
      cursor += str8_deserial_read_array(raw_symbol, cursor, &funcs[0], sym.count);
      U32  invocation_count = (raw_symbol.size - cursor) / sizeof(U32);
      U32 *invocations      = push_array(arena, U32, invocation_count);
      cursor += str8_deserial_read_array(raw_symbol, cursor, &invocations[0], invocation_count);
      
      rd_printf("Count: %u", sym.count);
      rd_indent();
      for (U32 i = 0; i < sym.count; ++i) {
        U32 invoks = i < invocation_count ? invocations[i] : 0;
        rd_printf("%#08x (%u)", funcs[i], invoks);
      }
      rd_unindent();
    } break;
    case CV_SymKind_INLINEES: {
      CV_SymInlinees sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Count: %u", sym.count);
      rd_indent();
      for (U32 i = 0; i < sym.count; ++i) {
        U32 itype;
        cursor += str8_deserial_read_struct(raw_symbol, cursor, &itype);
        rd_printf("%S", cv_string_from_itype(arena, min_itype, itype));
      }
      rd_unindent();
    } break;
    case CV_SymKind_INLINESITE: {
      CV_SymInlineSite sym         = {0};
      String8          raw_annots = str8_skip(raw_symbol, sizeof(CV_SymInlineSite));
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += raw_annots.size;
      
      rd_printf("Parent : %#x", sym.parent);
      rd_printf("End    : %#x", sym.end);
      rd_printf("Inlinee: %S",  cv_string_from_itemid(arena, sym.inlinee));
      cv_print_binary_annots(arena, out, indent, arch, raw_annots);
    } break;
    case CV_SymKind_INLINESITE2: {
      CV_SymInlineSite2 sym        = {0};
      String8           raw_annots = str8_skip(raw_symbol, sizeof(CV_SymInlineSite2));
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += raw_annots.size;
      
      rd_printf("Parent     : %#x", sym.parent_off);
      rd_printf("End        : %#x", sym.end_off);
      rd_printf("Inlinee    : %S",  cv_string_from_itemid(arena, sym.inlinee));
      rd_printf("Invocations: %u",  sym.invocations);
      cv_print_binary_annots(arena, out, indent, arch, raw_annots);
    } break;
    case CV_SymKind_INLINESITE_END: {
      // nothing to report
    } break;
    case CV_SymKind_LTHREAD32_ST:
    case CV_SymKind_GTHREAD32_ST:
    case CV_SymKind_LTHREAD32:
    case CV_SymKind_GTHREAD32: {
      CV_SymThread32 sym  = {0};
      String8        name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name       : %S", name);
      rd_printf("TSL Address: %S", cv_string_sec_off(scratch.arena, sym.tls_seg, sym.tls_off));
      rd_printf("Type       : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
    } break;
    case CV_SymKind_OBJNAME: {
      CV_SymObjName sym  = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name     : %S",  name);
      rd_printf("Signature: %#x", sym.sig);
    } break;
    case CV_SymKind_BLOCK32_ST:
    case CV_SymKind_BLOCK32: {
      CV_SymBlock32 sym  = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Parent : %#x", sym.parent);
      rd_printf("End    : %#x", sym.end);
      rd_printf("Address: %S",  cv_string_sec_off(scratch.arena, sym.sec, sym.off));
      rd_printf("Length : %u",  sym.len);
      rd_printf("Name   : %S",  name);
      if (name.size) {
      }
    } break;
    case CV_SymKind_LABEL32_ST:
    case CV_SymKind_LABEL32: {
      CV_SymLabel32 sym  = {0};
      String8       name = str8_zero();
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Address: %S, Flags: %S, Name: %S", cv_string_sec_off(scratch.arena, sym.sec, sym.off), cv_string_from_proc_flags(scratch.arena, sym.flags), name);
    } break;
    case CV_SymKind_COMPILE: {
      Assert(!"TODO: test");
      CV_SymCompile sym            = {0};
      String8       version_string = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &version_string);
      
      U32     language     = CV_CompileFlags_Extract_Language(sym.flags);
      U32     float_prec   = CV_CompileFlags_Extract_FloatPrec(sym.flags);
      U32     float_pkg    = CV_CompileFlags_Extract_FloatPkg(sym.flags);
      U32     ambient_data = CV_CompileFlags_Extract_AmbientData(sym.flags);
      U32     mode         = CV_CompileFlags_Extract_Mode(sym.flags);
      rd_printf("Arch          : %#x (%S)", sym.machine, cv_string_from_arch(sym.machine));
      rd_printf("Language      : %#x (%S)", language, cv_string_from_language(language));
      rd_printf("FloatPrec     : %#x",      float_prec);
      rd_printf("FloatPkg      : %#x",      float_pkg);
      rd_printf("Ambient Data  : %#x",      ambient_data);
      rd_printf("Mode          : %#x",      mode);
      rd_printf("Version String: %S",       version_string);
    } break;
    case CV_SymKind_COMPILE2_ST:
    case CV_SymKind_COMPILE2: {
      Assert(!"TODO: test");
      CV_SymCompile2 sym            = {0};
      String8        version_string = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &version_string);
      
      U32 language = CV_Compile2Flags_Extract_Language(sym.flags);
      rd_printf("Machine         : %#x (%S)", sym.machine, cv_string_from_arch(sym.machine));
      rd_printf("Flags           : %#x",      sym.flags);
      rd_printf("Language        : %#x (%S)", language, cv_string_from_language(language));
      rd_printf("Frontend Version: %u.%u",    sym.ver_fe_major, sym.ver_fe_minor);
      rd_printf("Frontend Build  : %u",       sym.ver_fe_build);
      rd_printf("Backend Version : %u.%u",    sym.ver_major, sym.ver_minor);
      rd_printf("Backend Build   : %u",       sym.ver_build);
      rd_printf("Version String  : %S",       version_string);
    } break;
    case CV_SymKind_COMPILE3: {
      CV_SymCompile3 sym            = {0};
      String8        version_string = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &version_string);
      
      U32 language = CV_Compile3Flags_Extract_Language(sym.flags);
      rd_printf("Machine         : %#x (%S)", sym.machine, cv_string_from_arch(sym.machine));
      rd_printf("Flags           : %#x",      sym.flags);
      rd_printf("Language        : %#x (%S)", language, cv_string_from_language(language));
      rd_printf("Frontend Version: %u.%u",    sym.ver_fe_major, sym.ver_fe_minor);
      rd_printf("Frontend Build  : %u",       sym.ver_fe_build);
      rd_printf("Fontend QFE     : %u",       sym.ver_feqfe);
      rd_printf("Backend Version : %u.%u",    sym.ver_major, sym.ver_minor);
      rd_printf("Backend Build   : %u",       sym.ver_build);
      rd_printf("Backend QFE     : %u",       sym.ver_qfe);
      rd_printf("Version String  : %S",       version_string);
    } break;
    case CV_SymKind_GPROC32_ST:
    case CV_SymKind_LPROC32_ST:
    case CV_SymKind_GPROC32_ID:
    case CV_SymKind_LPROC32_ID:
    case CV_SymKind_LPROC32:
    case CV_SymKind_GPROC32: {
      CV_SymProc32 sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      String8 name = str8_zero();
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name       : %S",       name);
      rd_printf("Parent     : %#x",      sym.parent);
      rd_printf("End        : %#x",      sym.end);
      rd_printf("Next       : %#x",      sym.next);
      rd_printf("Address    : %S",       cv_string_sec_off(scratch.arena, sym.sec, sym.off));
      rd_printf("Length     : %u",       sym.len);
      rd_printf("Type       : %S",       cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("Flags      : %#x (%S)", sym.flags, cv_string_from_proc_flags(scratch.arena, sym.flags));
      rd_printf("Debug Start: %#x",      sym.dbg_start);
      rd_printf("Debug End  : %#x",      sym.dbg_end);
    } break;
    case CV_SymKind_LDATA32_ST:
    case CV_SymKind_GDATA32_ST:
    case CV_SymKind_GDATA32:
    case CV_SymKind_LDATA32: {
      CV_SymData32 sym  = {0};
      String8      name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name   : %S", name);
      rd_printf("Type   : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("Address: %S", cv_string_sec_off(scratch.arena, sym.sec, sym.off));
    } break;
    case CV_SymKind_CONSTANT_ST:
    case CV_SymKind_CONSTANT: {
      CV_SymConstant   sym  = {0};
      CV_NumericParsed size = {0};
      String8          name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += cv_read_numeric(raw_symbol, cursor, &size);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name: %S", name);
      rd_printf("Type: %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("Size: %S", cv_string_from_numeric(scratch.arena, size));
    } break;
    case CV_SymKind_FRAMEPROC: {
      CV_SymFrameproc sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      String8 flags     = cv_string_from_frame_proc_flags(scratch.arena, sym.flags);
      U32     local_ptr = CV_FrameprocFlags_Extract_LocalBasePointer(sym.flags);
      U32     param_ptr = CV_FrameprocFlags_Extract_ParamBasePointer(sym.flags);
      rd_printf("Frame Size         : %#x", sym.frame_size);
      rd_printf("Pad Size           : %#x", sym.pad_size);
      rd_printf("Pad Offset         : %#x", sym.pad_off);
      rd_printf("Save Registers Area: %u",  sym.save_reg_size);
      rd_printf("Exception Handler  : %S",  cv_string_sec_off(arena, sym.eh_sec, sym.eh_off));
      rd_printf("Flags              : %S",  flags);
      rd_printf("Local pointer      : %S",  cv_string_from_reg_id(scratch.arena, arch, cv_map_encoded_base_pointer(arch, local_ptr)));
      rd_printf("Param pointer      : %S",  cv_string_from_reg_id(scratch.arena, arch, cv_map_encoded_base_pointer(arch, param_ptr)));
    } break;
    case CV_SymKind_LOCAL: {
      CV_SymLocal sym  = {0};
      String8     name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name : %S", name);
      rd_printf("Type : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("Flags: %S", cv_string_from_local_flags(scratch.arena, sym.flags));
    } break;
    case CV_SymKind_DEFRANGE: {
      CV_SymDefrange sym      = {0};
      String8        raw_gaps = str8_skip(raw_symbol, sizeof(CV_SymDefrange));
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += raw_gaps.size;
      
      rd_printf("Program: %#x", sym.program);
      cv_print_lvar_addr_range(arena, out, indent, sym.range);
      cv_print_lvar_addr_gap(arena, out, indent, raw_gaps);
    } break;
    case CV_SymKind_DEFRANGE_REGISTER: {
      CV_SymDefrangeRegister sym      = {0};
      String8                raw_gaps = str8_skip(raw_symbol, sizeof(CV_SymDefrangeRegisterRel));
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += raw_gaps.size;
      
      rd_printf("Register  : %S", cv_string_from_reg_id(scratch.arena, arch, sym.reg));
      rd_printf("Attributes: %S", cv_string_from_range_attribs(scratch.arena, sym.attribs));
      cv_print_lvar_addr_range(arena, out, indent, sym.range);
      cv_print_lvar_addr_gap(arena, out, indent, raw_gaps);
    } break;
    case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL: {
      CV_SymDefrangeFramepointerRel sym      = {0};
      String8                       raw_gaps = str8_skip(raw_symbol, sizeof(CV_SymDefrangeFramepointerRel));
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Offset: %#x", sym.off);
      cv_print_lvar_addr_range(arena, out, indent, sym.range);
      cv_print_lvar_addr_gap(arena, out, indent, raw_gaps);
    } break;
    case CV_SymKind_DEFRANGE_SUBFIELD_REGISTER: {
      CV_SymDefrangeSubfieldRegister sym      = {0};
      String8                        raw_gaps = str8_skip(raw_symbol, sizeof(CV_SymDefrangeSubfieldRegister));
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += raw_gaps.size;
      
      rd_printf("Register     : %#x (%S)", sym.reg, cv_string_from_reg_id(scratch.arena, arch, sym.reg));
      rd_printf("Attributes   : %#x (%S)", sym.attribs, cv_string_from_range_attribs(scratch.arena, sym.attribs));
      rd_printf("Parent Offset: %#x",      sym.field_offset);
      cv_print_lvar_addr_range(arena, out, indent, sym.range);
      cv_print_lvar_addr_gap(arena, out, indent, raw_gaps);
    } break;
    case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE: {
      CV_SymDefrangeFramepointerRelFullScope sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      rd_printf("Offset: %#x", sym.off);
    } break;
    case CV_SymKind_DEFRANGE_REGISTER_REL: {
      CV_SymDefrangeRegisterRel sym      = {0};
      String8                   raw_gaps = str8_skip(raw_symbol, sizeof(CV_SymDefrangeRegisterRel));
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += raw_gaps.size;
      
      rd_printf("Flags  : %#x (%S)", sym.flags, cv_string_from_defrange_register_rel_flags(scratch.arena, sym.flags));
      rd_printf("Address: %S",       cv_string_from_reg_off(scratch.arena, arch, sym.reg, sym.reg_off));
      cv_print_lvar_addr_gap(arena, out, indent, raw_gaps);
    } break;
    case CV_SymKind_END:
    case CV_SymKind_PROC_ID_END: {
      // no data
    } break;
    case CV_SymKind_UDT_ST:
    case CV_SymKind_UDT: {
      CV_SymUDT sym  = {0};
      String8   name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name: %S", name);
      rd_printf("Type: %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
    } break;
    case CV_SymKind_BUILDINFO: {
      CV_SymBuildInfo sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      rd_printf("ID: %#x", sym.id);
    } break;
    case CV_SymKind_UNAMESPACE_ST:
    case CV_SymKind_UNAMESPACE: {
      String8 name = {0};
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name: %S", name);
    } break;
    case CV_SymKind_REGREL32_ST:
    case CV_SymKind_REGREL32: {
      CV_SymRegrel32 sym  = {0};
      String8        name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Address: %S, Type: %S, Name: %S",
                cv_string_from_reg_off(scratch.arena, arch, sym.reg, sym.reg_off),
                cv_string_from_itype(scratch.arena, min_itype, sym.itype),
                name);
    } break;
    case CV_SymKind_CALLSITEINFO: {
      CV_SymCallSiteInfo sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Address: %S", cv_string_sec_off(scratch.arena, sym.sec, sym.off));
      rd_printf("Pad    : %u", sym.pad);
      rd_printf("Type   : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
    } break;
    case CV_SymKind_FRAMECOOKIE: {
      CV_SymFrameCookie sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Address: %S",       cv_string_sec_off(arena, sym.reg, sym.off));
      rd_printf("Kind   : %#x (%S)", sym.kind, cv_string_from_frame_cookie_kind(sym.kind));
      rd_printf("Flags  : %#x",      sym.flags); // TODO: llvm and cvinfo.h don't define these flags...
    } break;
    case CV_SymKind_HEAPALLOCSITE: {
      CV_SymHeapAllocSite sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      String8 addr  = cv_string_sec_off(arena, sym.sec, sym.off);
      String8 itype = cv_string_from_itype(arena, min_itype, sym.itype);
      rd_printf("Address                : %S", addr);
      rd_printf("Type                   : %S", itype);
      rd_printf("Call instruction length: %x", sym.call_inst_len);
    } break;
    case CV_SymKind_ALIGN: {
      // spec:
      // Unused data. Use the length field that precedes every symbol record
      // to skip this record. The pad bytes must be zero. For sstGlobalSym
      // and sstGlobalPub, the length of the pad field must be at least the
      // sizeof (long). There must be an S_Align symbol at the end of these
      // tables with a pad field containing 0xffffffff. The sstStaticSym table
      // does not have this requirement
    } break;
    case CV_SymKind_SKIP: {
      // Unused data, tools use this symbol to reserve space for future expansion
      // in incremental builds.
    } break;
    case CV_SymKind_ENDARG: {
      // spec:
      // This symbol specifies the end of symbol records used in formal arguments for a function. Use of
      // this symbol is optional for OMF and required for MIPS-compiled code. In OMF format, the end
      // of arguments can also be deduced from the fact that arguments for a function have a positive
      // offset from the frame pointer.
    } break;
    case CV_SymKind_CVRESERVE: {
      // Reserved for MS debugger
    } break;
    case CV_SymKind_SSEARCH: {
      Assert(!"TODO: test");
      
      CV_SymStartSearch sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Start Symbol: %#x", sym.start_symbol);
      rd_printf("Segment     : %#x", sym.segment);
    } break;
    case CV_SymKind_RETURN: {
      Assert(!"TODO: test");
      
      CV_SymReturn sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Flags     : %#x (%S)", sym.flags, cv_string_from_generic_flags(scratch.arena, sym.flags));
      rd_printf("Style     : %#x (%S)", sym.style, cv_string_from_generic_style(sym.style));
      if (sym.style == CV_GenericStyle_REG) {
        U8 count = 0;
        cursor += str8_deserial_read_struct(raw_symbol, cursor, &count);
        
        String8 data = rd_format_hex_array(scratch.arena, raw_symbol.str, raw_symbol.size);
        rd_printf("Byte Count: %u", count);
        rd_printf("Data      : %S", data);
      }
    } break;
    case CV_SymKind_ENTRYTHIS: {
      Assert(!"TODO: test");
      
      U16 symbol_size = 0, symbol_type = 0;
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &symbol_size);
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &symbol_type);
      String8 raw_subsym = str8_skip(raw_symbol, cursor);
      
      cv_print_symbol(arena, out, indent, arch, min_itype, type, raw_subsym);
    } break;
    case CV_SymKind_SLINK32: {
      Assert(!"TODO: test");
      
      CV_SymSLink32 sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Frame Size: %x", sym.frame_size);
      rd_printf("Address   : %S", cv_string_from_reg_off(scratch.arena, arch, sym.reg, sym.offset));
    } break;
    case CV_SymKind_OEM: {
      Assert(!"TODO: test");
      
      CV_SymOEM sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      // TODO: Not clear what to do about user data that follows, are we supposed to assume that
      // rest of the range is it? 
      //
      // CV-spec doesn't even mention S_OEM just LF_OEM and cvdump.exe prints out type with guid...
      rd_printf("Type: %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("ID  : %S", string_from_guid(scratch.arena, sym.id));
    } break;
    case CV_SymKind_VFTABLE32:{
      Assert(!"TODO: test");
      
      CV_SymVPath32 sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Root   : %S", cv_string_from_itype(scratch.arena, min_itype, sym.root));
      rd_printf("Path   : %S", cv_string_from_itype(scratch.arena, min_itype, sym.path));
      rd_printf("Address: %S", cv_string_sec_off(scratch.arena, sym.seg, sym.off));
    } break;
    case CV_SymKind_PUB32_ST:
    case CV_SymKind_PUB32: {
      CV_SymPub32 sym  = {0};
      String8     name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name   : %S", name);
      rd_printf("Flags  : %S", cv_string_from_pub32_flags(scratch.arena, sym.flags));
      rd_printf("Address: %S", cv_string_sec_off(scratch.arena, sym.sec, sym.off));
    } break;
    case CV_SymKind_BPREL32_ST:
    case CV_SymKind_BPREL32: {
      Assert(!"TODO: test");
      
      CV_SymBPRel32 sym  = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name  : %S",  name);
      rd_printf("Offset: %#x", sym.off);
      rd_printf("Type  : %S",  cv_string_from_itype(scratch.arena, min_itype, sym.itype));
    } break;
    case CV_SymKind_REGISTER: {
      Assert(!"TODO: test");
      
      CV_SymRegister sym  = {0};
      String8        name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name    : %S", name);
      rd_printf("Register: %S", cv_string_from_reg_id(scratch.arena, arch, sym.reg));
      rd_printf("Type    : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
    } break;
    case CV_SymKind_PROCREF_ST:
    case CV_SymKind_DATAREF_ST:
    case CV_SymKind_LPROCREF_ST:
    case CV_SymKind_ANNOTATIONREF:
    case CV_SymKind_LPROCREF:
    case CV_SymKind_PROCREF:
    case CV_SymKind_DATAREF: {
      Assert(!"TODO: test");
      
      CV_SymRef2 sym  = {0};
      String8    name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name                : %S",  name);
      rd_printf("SUC                 : %#x", sym.suc_name);
      rd_printf("IMod                : %#x", sym.imod);
      rd_printf("Symbol Stream Offset: %#x", sym.sym_off);
    } break;
    case CV_SymKind_SEPCODE: {
      Assert(!"TODO: test");
      
      CV_SymSepcode sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Parent        : %#x",      sym.parent);
      rd_printf("End           : %#x",      sym.end);
      rd_printf("Length        : %u",       sym.len);
      rd_printf("Flags         : %#x (%S)", sym.flags, cv_string_from_sepcode(scratch.arena, sym.flags));
      rd_printf("Address       : %S",       cv_string_sec_off(scratch.arena, sym.sec, sym.sec_off));
      rd_printf("Parent Address: %S",       cv_string_sec_off(scratch.arena, sym.sec_parent, sym.sec_parent_off));
    } break;
    case CV_SymKind_PARAMSLOT_ST:
    case CV_SymKind_LOCALSLOT_ST:
    case CV_SymKind_LOCALSLOT: {
      Assert(!"TODO: test");
      
      CV_SymSlot sym  = {0};
      String8    name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name: %S", name);
      rd_printf("Slot: %u", sym.slot_index);
      rd_printf("Type: %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
    } break;
    case CV_SymKind_TRAMPOLINE: {
      Assert(!"TODO: test");
      
      CV_SymTrampoline sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Type      : %S", cv_string_from_trampoline_kind(sym.kind));
      rd_printf("Thunk Size: %u", sym.thunk_size);
      rd_printf("Thunk     : %S", cv_string_sec_off(scratch.arena, sym.thunk_sec, sym.thunk_sec_off));
      rd_printf("Target    : %S", cv_string_sec_off(scratch.arena, sym.target_sec, sym.target_sec_off));
    } break;
    case CV_SymKind_POGODATA: {
      Assert(!"TODO: test");
      
      CV_SymPogoInfo sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Invocations                         : %u", sym.invocations);
      rd_printf("Dynamic instruction count           : %u", sym.dynamic_inst_count);
      rd_printf("Static instruction count            : %u", sym.static_inst_count);
      rd_printf("Post inline static instruction count: %u", sym.post_inline_static_inst_count);
    } break;
    case CV_SymKind_MANYREG: {
      Assert(!"TODO: test");
      
      CV_SymManyreg sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Type     : %S", cv_string_from_itype(arena, min_itype, sym.itype));
      rd_printf("Reg Count: %u", sym.count);
      rd_printf("Regs     :");
      rd_indent();
      for (U8 i = 0; i < sym.count; ++i) {
        U8 v = 0;
        cursor += str8_deserial_read_struct(raw_symbol, cursor, &v);
        rd_printf("%S", cv_string_from_reg_id(scratch.arena, arch, v));
      }
      rd_unindent();
    } break;
    case CV_SymKind_MANYREG2_ST:
    case CV_SymKind_MANYREG2: {
      Assert(!"TODO: test");
      
      CV_SymManyreg sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Type     : %S", cv_string_from_itype(arena, min_itype, sym.itype));
      rd_printf("Reg Count: %u", sym.count);
      rd_printf("Regs     :");
      rd_indent();
      for (U16 i = 0; i < sym.count; ++i) {
        U16 v = 0;
        cursor += str8_deserial_read_struct(raw_symbol, cursor, &v);
        rd_printf("%S", cv_string_from_reg_id(scratch.arena, arch, v));
      }
      rd_unindent();
    } break;
    case CV_SymKind_SECTION: {
      Assert(!"TODO: test");
      
      CV_SymSection sym  = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name           : %S",  name);
      rd_printf("Index          : %u",  sym.sec_index);
      rd_printf("Align          : %u",  sym.align);
      rd_printf("Virtual Offset : %#x", sym.rva);
      rd_printf("Size           : %u",  sym.size);
      rd_printf("Characteristics: %S",  coff_string_from_section_flags(scratch.arena, sym.characteristics));
    } break;
    case CV_SymKind_ENVBLOCK: {
      CV_SymEnvBlock sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      for (; cursor < raw_symbol.size; ) {
        String8 id = str8_zero();
        cursor += str8_deserial_read_cstr(raw_symbol, cursor, &id);
        String8 path = str8_zero();
        cursor += str8_deserial_read_cstr(raw_symbol, cursor, &path);
        if (id.size == 0 && path.size == 0) {
          break;
        }
        rd_printf("%S = %S", id, path);
      }
    } break;
    case CV_SymKind_COFFGROUP: {
      Assert(!"TODO: test");
      
      CV_SymCoffGroup sym  = {0};
      String8         name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name           : %S",       name);
      rd_printf("Size           : %u",       sym.size);
      rd_printf("Characteristics: %#x (%S)", sym.characteristics, coff_string_from_section_flags(scratch.arena, sym.characteristics));
      rd_printf("Address        : %S",       cv_string_sec_off(scratch.arena, sym.sec, sym.off));
    } break;
    case CV_SymKind_EXPORT: {
      Assert(!"TODO: test");
      
      CV_SymExport sym  = {0};
      String8      name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name   : %S",  name);
      rd_printf("Ordinal: %#x", sym.ordinal);
      rd_printf("Flags  : %S",  cv_string_from_export_flags(scratch.arena, sym.flags));
    } break;
    case CV_SymKind_ANNOTATION: {
      Assert(!"TODO: test");
      
      CV_SymAnnotation sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Address    : %S", cv_string_sec_off(scratch.arena, sym.seg, sym.off));
      rd_printf("Count      : %u", sym.count);
      rd_printf("Annotations:");
      rd_indent();
      for (U16 i = 0; i < sym.count; ++i) {
        String8 str = str8_zero();
        cursor += str8_deserial_read_cstr(raw_symbol, cursor, &str);
        rd_printf("%S", str);
      }
      rd_unindent();
    } break;
    case CV_SymKind_MANFRAMEREL:
    case CV_SymKind_ATTR_FRAMEREL: {
      Assert(!"TODO: test");
      
      CV_SymAttrFrameRel sym  = {0};
      String8            name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name  : %S",  name);
      rd_printf("Offset: %#x", sym.off);
      rd_printf("Type  : %S",  cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      cv_print_lvar_attr(arena, out, indent, sym.attr);
    } break;
    case CV_SymKind_MANREGISTER:
    case CV_SymKind_ATTR_REGISTER: {
      Assert(!"TODO: test");
      
      CV_SymAttrReg sym  = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name    : %S", name);
      rd_printf("Type    : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("Register: %S", cv_string_from_reg_id(scratch.arena, arch, sym.reg));
      cv_print_lvar_attr(arena, out, indent, sym.attr);
    } break;
    case CV_SymKind_ATTR_REGREL: {
      Assert(!"TODO: test");
      
      CV_SymAttrRegRel sym  = {0};
      String8          name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name   : %S", name);
      rd_printf("Type   : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      rd_printf("Address: %S", cv_string_from_reg_off(scratch.arena, arch, sym.reg, sym.off));
      cv_print_lvar_attr(arena, out, indent, sym.attr);
    } break;
    case CV_SymKind_MANYREG_ST:
    case CV_SymKind_MANMANYREG:
    case CV_SymKind_ATTR_MANYREG: {
      Assert(!"TODO: test");
      
      CV_SymAttrManyReg sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      U8 *regs = push_array(scratch.arena, U8, sym.count);
      cursor += str8_deserial_read_array(raw_symbol, cursor, &regs[0], sym.count);
      String8 name = str8_zero();
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name     : %S", name);
      rd_printf("Type     : %S", cv_string_from_itype(scratch.arena, min_itype, sym.itype));
      cv_print_lvar_attr(arena, out, indent, sym.attr);
      rd_printf("Reg Count: %u", sym.count);
      rd_printf("Regs     :");
      rd_indent();
      for (U8 i = 0; i < sym.count; ++i) {
        rd_printf("%S", cv_string_from_reg_id(scratch.arena, arch, regs[i]));
      }
      rd_unindent();
    } break;
    case CV_SymKind_MOD_TYPEREF: {
      
      Assert(!"TODO: test");
      
      CV_SymModTypeRef sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      String8List flags_list = {0};
      if (sym.flags & CV_ModTypeRefFlag_None) {
        str8_list_pushf(scratch.arena, &flags_list, "No TypeRef");
      } else if (sym.flags & CV_ModTypeRefFlag_OwnTMR) {
        str8_list_pushf(scratch.arena, &flags_list, "/Z7 TypeRef, SN=%04X", sym.word0);
        if (sym.flags & CV_ModTypeRefFlag_OwnTMPCT) {
          str8_list_pushf(scratch.arena, &flags_list, "own PCH types");
        }
        if (sym.flags & CV_ModTypeRefFlag_RefTMPCT) {
          str8_list_pushf(scratch.arena, &flags_list, "reference PCH types in module %04X", (sym.word1+1));
        }
      } else {
        str8_list_pushf(scratch.arena, &flags_list, "/Zi TypeRef");
        if (sym.flags & CV_ModTypeRefFlag_OwnTM) {
          str8_list_pushf(scratch.arena, &flags_list, "SN=%04X (type), SN=%04X (ID)", sym.word0, sym.word1);
        }
        if (sym.flags & CV_ModTypeRefFlag_RefTM) {
          str8_list_pushf(scratch.arena, &flags_list, "shared with Module %04X", sym.word0+1);
        }
      }
      String8 flags_str = str8_list_join(scratch.arena, &flags_list, &(StringJoin){.sep=str8_lit(", ")});
      
      rd_printf("%S", flags_str);
    } break;
    case CV_SymKind_DISCARDED: {
      Assert(!"TODO: test");
      
      CV_SymDiscarded sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      U32 symbol_type = 0;
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &symbol_type);
      String8 raw_subsym = str8_skip(raw_symbol, cursor);
      
      rd_printf("Kind            : %x", sym.kind);
      rd_printf("File ID         : %x", sym.file_id);
      rd_printf("File Line Number: %u", sym.file_ln);
      rd_printf("# Discarded Symbol");
      cv_print_symbol(arena, out, indent, arch, min_itype, symbol_type, raw_subsym);
    } break;
    case CV_SymKind_PDBMAP: {
      Assert(!"TODO: test");
      
      String8 from = {0};
      String8 to   = {0};
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &from);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &to);
      
      rd_printf("From: %S", from);
      rd_printf("To  : %S", to);
    } break;
    case CV_SymKind_FASTLINK: {
      Assert(!"TODO: test");
      
      CV_SymFastLink sym  = {0};
      String8        name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name : %S",  name);
      rd_printf("Flags: %#x", sym.flags);
      rd_printf("Type : %S",  cv_string_from_itype(arena, min_itype, sym.itype));
    } break;
    case CV_SymKind_ARMSWITCHTABLE: {
      Assert(!"TODO: test");
      
      CV_SymArmSwitchTable sym = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      
      rd_printf("Base Address  : %S", cv_string_sec_off(scratch.arena, sym.sec_base,   sym.off_base));
      rd_printf("Branch Address: %S", cv_string_sec_off(scratch.arena, sym.sec_branch, sym.off_branch));
      rd_printf("Table Address : %S", cv_string_sec_off(scratch.arena, sym.sec_table,  sym.off_table));
      rd_printf("Entry count   : %u", sym.entry_count);
      rd_printf("Switch Type   : %x", sym.kind);
    } break;
    case CV_SymKind_REF_MINIPDB: {
      Assert(!"TODO: test");
      
      CV_SymRefMiniPdb sym  = {0};
      String8          name = {0};
      cursor += str8_deserial_read_struct(raw_symbol, cursor, &sym);
      cursor += str8_deserial_read_cstr(raw_symbol, cursor, &name);
      
      rd_printf("Name      : %S", name);
      rd_printf("Flags     : %x", sym.flags);
      rd_printf("IMod      : %04x", sym.imod);
      if (sym.flags & CV_RefMiniPdbFlag_UDT) {
        rd_printf("Type      : %S", cv_string_from_itype(scratch.arena, min_itype, (CV_TypeIndex)sym.data));
      } else {
        rd_printf("Coff ISect: %#x", sym.data);
      }
    } break;
    // COBOL
    case CV_SymKind_CEXMODEL32:
    case CV_SymKind_COBOLUDT_ST:
    case CV_SymKind_COBOLUDT:
    // Pascal
    case CV_SymKind_WITH32_ST:
    case CV_SymKind_WITH32:
    //~ 16bit
    case CV_SymKind_REGISTER_16t:
    case CV_SymKind_CONSTANT_16t:
    case CV_SymKind_UDT_16t:
    case CV_SymKind_OBJNAME_ST:
    case CV_SymKind_COBOLUDT_16t:
    case CV_SymKind_MANYREG_16t:
    case CV_SymKind_BPREL16:
    case CV_SymKind_LDATA16:
    case CV_SymKind_GDATA16:
    case CV_SymKind_PUB16:
    case CV_SymKind_LPROC16:
    case CV_SymKind_GPROC16:
    case CV_SymKind_THUNK16:
    case CV_SymKind_BLOCK16:
    case CV_SymKind_WITH16:
    case CV_SymKind_LABEL16:
    case CV_SymKind_CEXMODEL16:
    case CV_SymKind_VFTABLE16:
    case CV_SymKind_REGREL16:
    case CV_SymKind_TI16_MAX:
    //~ 16:32 memory model
    case CV_SymKind_BPREL32_16t:
    case CV_SymKind_LDATA32_16t:
    case CV_SymKind_GDATA32_16t:
    case CV_SymKind_PUB32_16t:
    case CV_SymKind_LPROC32_16t:
    case CV_SymKind_GPROC32_16t:
    case CV_SymKind_VFTABLE32_16t:
    case CV_SymKind_REGREL32_16t:
    case CV_SymKind_LTHREAD32_16t:
    case CV_SymKind_GTHREAD32_16t:
    case CV_SymKind_LPROCMIPS_16t:
    case CV_SymKind_GPROCMIPS_16t:
    // MIPS
    case CV_SymKind_LPROCMIPS_ST:
    case CV_SymKind_GPROCMIPS_ST:
    case CV_SymKind_LPROCMIPS:
    case CV_SymKind_GPROCMIPS:
    case CV_SymKind_LPROCIA64:
    case CV_SymKind_GPROCIA64:
    case CV_SymKind_LPROCMIPS_ID:
    case CV_SymKind_GPROCMIPS_ID:
    // Managed
    case CV_SymKind_TOKENREF:
    case CV_SymKind_GMANPROC_ST:
    case CV_SymKind_LMANPROC_ST:
    case CV_SymKind_LMANDATA_ST:
    case CV_SymKind_GMANDATA_ST:
    case CV_SymKind_MANFRAMEREL_ST:
    case CV_SymKind_MANREGISTER_ST:
    case CV_SymKind_MANSLOT_ST:
    case CV_SymKind_MANTYPREF:
    case CV_SymKind_MANMANYREG_ST:
    case CV_SymKind_MANREGREL_ST:
    case CV_SymKind_MANMANYREG2_ST:
    case CV_SymKind_MANMANYREG2:
    case CV_SymKind_MANREGREL:
    case CV_SymKind_MANSLOT:
    case CV_SymKind_MANCONSTANT:
    case CV_SymKind_LMANDATA:
    case CV_SymKind_GMANDATA:
    case CV_SymKind_GMANPROC:
    case CV_SymKind_LMANPROC:
    // HLSL
    case CV_SymKind_DEFRANGE_DPC_PTR_TAG:
    case CV_SymKind_DPC_SYM_TAG_MAP:
    case CV_SymKind_DEFRANGE_HLSL:
    case CV_SymKind_GDATA_HLSL:
    case CV_SymKind_LDATA_HLSL:
    case CV_SymKind_LPROC32_DPC:
    case CV_SymKind_LPROC32_DPC_ID:
    case CV_SymKind_GDATA_HLSL32:
    case CV_SymKind_LDATA_HLSL32:
    case CV_SymKind_GDATA_HLSL32_EX:
    case CV_SymKind_LDATA_HLSL32_EX: 
    // IA64
    case CV_SymKind_LPROCIA64_ID:
    case CV_SymKind_GPROCIA64_ID:
    // VS2005
    case CV_SymKind_DEFRANGE_2005:
    case CV_SymKind_DEFRANGE2_2005:
    case CV_SymKind_ST_MAX: 
    case CV_SymKind_RESERVED1:
    case CV_SymKind_RESERVED2:
    case CV_SymKind_RESERVED3:
    case CV_SymKind_RESERVED4: {
    } break;
  }
  scratch_end(scratch);
}

internal U64
cv_print_leaf(Arena *arena, String8List *out, String8 indent, CV_TypeIndex min_itype, CV_LeafKind kind, String8 raw_leaf)
{
  Temp scratch = scratch_begin(&arena, 1);
  U64 cursor = 0;
  switch (kind) {
    case CV_LeafKind_NOTYPE: {
      // empty
    } break;
    case CV_LeafKind_BITFIELD: {
      CV_LeafBitField lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Type    : %S", cv_string_from_itype(scratch.arena, min_itype, lf.itype));
      rd_printf("Length  : %u", lf.len);
      rd_printf("Position: %u", lf.pos);
    } break;
    case CV_LeafKind_CLASS2:
    case CV_LeafKind_STRUCT2: {
      CV_LeafStruct2   lf  = {0};
      CV_NumericParsed size = {0};
      String8          name = str8_zero();
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &size);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name       : %S",       name);
      rd_printf("Fields     : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.field_itype));
      rd_printf("Properties : %#x (%S)", lf.props, cv_string_from_type_props(scratch.arena, lf.props));
      rd_printf("Derived    : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.derived_itype));
      rd_printf("VShape     : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.vshape_itype));
      rd_printf("Unknown    : %#x",      lf.unknown);
      if (lf.props & CV_TypeProp_HasUniqueName) {
        String8 unique_name = str8_zero();
        cursor += str8_deserial_read_cstr(raw_leaf, cursor, &unique_name);
        rd_printf("Unique Name:  %S", unique_name);
      }
    } break;
    case CV_LeafKind_PRECOMP_ST: 
    case CV_LeafKind_PRECOMP: { 
      CV_LeafPreComp lf   = {0};
      String8        name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name       : %S", name);
      rd_printf("Start Index: %x", lf.start_index);
      rd_printf("Count      : %u", lf.count);
      rd_printf("Signature  : %x", lf.sig);
    } break;
    case CV_LeafKind_TYPESERVER2: {
      CV_LeafTypeServer2 lf   = {0};
      String8            name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name : %S", name);
      rd_printf("Sig70: %S", string_from_guid(arena, lf.sig70));
      rd_printf("Age  : %u", lf.age);
    } break;
    case CV_LeafKind_BUILDINFO: {
      CV_LeafBuildInfo lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Entry Count: %u", lf.count);
      rd_indent();
      for (U16 i = 0; i < lf.count; ++i) {
        CV_ItemId id = 0;
        cursor += str8_deserial_read_struct(raw_leaf, cursor, &id);
        rd_printf("%S", cv_string_from_itemid(scratch.arena, id));
      }
      rd_unindent();
    } break;
    case CV_LeafKind_MFUNC_ID: {
      CV_LeafMFuncId lf   = {0};
      String8        name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      rd_printf("Name      : %S", name);
      rd_printf("Owner Type: %S", cv_string_from_itype(scratch.arena, min_itype, lf.owner_itype));
      rd_printf("Type      : %S", cv_string_from_itype(scratch.arena, min_itype, lf.itype));
    } break;
    case CV_LeafKind_VFUNCTAB: {
      CV_LeafVFuncTab lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Type: %S", cv_string_from_itype(scratch.arena, min_itype, lf.itype));
    } break;
    case CV_LeafKind_METHODLIST: {
      for (; cursor < raw_leaf.size; ) {
        CV_LeafMethodListMember ml = {0};
        cursor += str8_deserial_read_struct(raw_leaf, cursor, &ml);
        U32 mprop     = CV_FieldAttribs_Extract_MethodProp(ml.attribs);
        B32 has_vbase = (mprop == CV_MethodProp_PureIntro) || (mprop == CV_MethodProp_Intro);
        U32 vbase     = 0;
        if (has_vbase) {
          cursor += str8_deserial_read_struct(raw_leaf, cursor, &vbase);
        }
        rd_printf("Attribs     : %#x (%S)", ml.attribs, cv_string_from_field_attribs(scratch.arena, ml.attribs));
        rd_printf("Type        : %S",       cv_string_from_itype(scratch.arena, min_itype, ml.itype));
        if (has_vbase) {
          rd_printf("Virtual Base: %x", vbase);
        }
      }
    } break;
    case CV_LeafKind_ONEMETHOD_ST: 
    case CV_LeafKind_ONEMETHOD: {
      CV_LeafOneMethod lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      U32 mprop     = CV_FieldAttribs_Extract_MethodProp(lf.attribs);
      B32 has_vbase = (mprop == CV_MethodProp_PureIntro) || (mprop == CV_MethodProp_Intro);
      U32 vbase     = 0;
      if (has_vbase) {
        cursor += str8_deserial_read_struct(raw_leaf, cursor, &vbase);
      }
      String8 name = {0};
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      rd_printf("Name         : %S",       name);
      rd_printf("Field Attribs: %#x (%S)", lf.attribs, cv_string_from_field_attribs(scratch.arena, lf.attribs));
      rd_printf("Type         : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.itype));
      if (has_vbase) {
        rd_printf("Virtual Base:  %#x", vbase);
      }
    } break;
    case CV_LeafKind_METHOD_ST: 
    case CV_LeafKind_METHOD: {
      CV_LeafMethod lf   = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name     : %S", name);
      rd_printf("Count    : %u", lf.count);
      rd_printf("Type List: %S", cv_string_from_itype(scratch.arena, min_itype, lf.list_itype));
    } break;
    case CV_LeafKind_VBCLASS:
    case CV_LeafKind_IVBCLASS: {
      CV_LeafVBClass   lf          = {0};
      CV_NumericParsed vbptr_off   = {0};
      CV_NumericParsed vbtable_off = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &vbptr_off);
      cursor += cv_read_numeric(raw_leaf, cursor, &vbtable_off);
      
      rd_printf("Attribs         : %#x (%S)", lf.attribs, cv_string_from_field_attribs(scratch.arena, lf.attribs));
      rd_printf("Direct Base Type: %S", cv_string_from_itype(scratch.arena, min_itype, lf.itype));
      rd_printf("Virtual Base Ptr: %S", cv_string_from_itype(scratch.arena, min_itype, lf.vbptr_itype));
      rd_printf("vbpoff          : %S", cv_string_from_numeric(scratch.arena, vbptr_off));
      rd_printf("vbind           : %S", cv_string_from_numeric(scratch.arena, vbtable_off));
    } break;
    case CV_LeafKind_BCLASS: {
      CV_LeafBClass    lf     = {0};
      CV_NumericParsed offset = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &offset);
      
      rd_printf("Attribs: %#x (%S)", lf.attribs, cv_string_from_field_attribs(scratch.arena, lf.attribs));
      rd_printf("Type   : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.itype));
      rd_printf("Offset : %S",       cv_string_from_numeric(scratch.arena, offset));
    } break;
    case CV_LeafKind_VTSHAPE: {
      CV_LeafVTShape lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Entry Count: %u", lf.count);
      rd_indent();
      for (U16 i = 0; i < lf.count; ++i) {
        U8 packed_kind = 0;
        str8_deserial_read_struct(raw_leaf, cursor + (i / 2), &packed_kind);
        U8 kind = (packed_kind >> ((i % 2)*4)) & 0xF;
        rd_printf("%S", cv_string_from_virtual_table_shape_kind(kind));
      }
      rd_unindent();
      cursor += (lf.count * sizeof(U8) + 1) / 2;
    } break;
    case CV_LeafKind_STMEMBER_ST: 
    case CV_LeafKind_STMEMBER: {
      CV_LeafStMember lf   = {0};
      String8         name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name   : %S",       name);
      rd_printf("Attribs: %#x (%S)", lf.attribs, cv_string_from_field_attribs(scratch.arena, lf.attribs));
      rd_printf("Type   : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.itype));
    } break;
    case CV_LeafKind_MFUNCTION: {
      CV_LeafMFunction lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Return Type     : %S", cv_string_from_itype(scratch.arena, min_itype, lf.ret_itype));
      rd_printf("Class Type      : %S", cv_string_from_itype(scratch.arena, min_itype, lf.class_itype));
      rd_printf("This Type       : %S", cv_string_from_itype(scratch.arena, min_itype, lf.this_itype));
      rd_printf("Call Kind       : %#x (%S)", lf.call_kind, cv_string_from_call_kind(lf.call_kind));
      rd_printf("Function Attribs: %#x (%S)", lf.attribs, cv_string_from_function_attribs(scratch.arena, lf.attribs));
      rd_printf("Argument Count  : %u", lf.arg_count);
      rd_printf("Argument Type   : %S", cv_string_from_itype(scratch.arena, min_itype, lf.arg_itype));
    } break;
#if 0
    case CV_LeafKind_SKIP_16t: {
      CV_LeafSkip_16t lf = {0};
      cursor += str8_deserial_read_struct(base, range, cursor, &lf);
      
      rd_printf("Type: %S", cv_string_from_itype(scratch.arena, min_itype, lf.type));
    } break;
#endif
    case CV_LeafKind_SKIP: {
      // ms-symbol-pdf:
      // This is used by incremental compilers to reserve space for indices.
      CV_LeafSkip lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      rd_printf("Type: %S", cv_string_from_itype(scratch.arena, min_itype, lf.itype));
    } break;
    case CV_LeafKind_ENUM_ST: 
    case CV_LeafKind_ENUM: {
      CV_LeafEnum lf  = {0};
      String8     name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name       : %S",       name);
      rd_printf("Field Count: %u",       lf.count);
      rd_printf("Properties : %#x (%S)", lf.props, cv_string_from_type_props(scratch.arena, lf.props));
      rd_printf("Type       : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.base_itype));
      rd_printf("Field      : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.field_itype));
      if (lf.props & CV_TypeProp_HasUniqueName) {
        String8 unique_name = {0};
        cursor += str8_deserial_read_cstr(raw_leaf, cursor, &unique_name);
        rd_printf("Unique Name: %S", unique_name);
      }
    } break;
    case CV_LeafKind_ENUMERATE: {
      CV_LeafEnumerate lf   = {0};
      CV_NumericParsed value = {0};
      String8          name  = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &value);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name   : %S", name);
      rd_printf("Attribs: %S", cv_string_from_field_attribs(scratch.arena, lf.attribs));
      rd_printf("Value  : %S", cv_string_from_numeric(scratch.arena, value));
    } break;
    case CV_LeafKind_NESTTYPE_ST: 
    case CV_LeafKind_NESTTYPE: {
      CV_LeafNestType lf  = {0};
      String8         name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      rd_printf("Name : %S", name);
      rd_printf("Index: %S", cv_string_from_itype(scratch.arena, min_itype, lf.itype));
    } break;
    case CV_LeafKind_NOTTRAN: {
      // ms-symbol-pdf: 
      //  This is used when CVPACK encounters a type record that has no equivalent in the Microsoftsymbol information format.
    } break;
    case CV_LeafKind_UDT_SRC_LINE: {
      CV_LeafUDTSrcLine lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Type = %S, Source File = %x, Line = %u", cv_string_from_itype(scratch.arena, min_itype, lf.udt_itype), lf.src_string_id, lf.line);
    } break;
    case CV_LeafKind_STRING_ID: {
      CV_LeafStringId lf    = {0};
      String8         string = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &string);
      
      rd_printf("string    : %S", string);
      rd_printf("Substrings: %S", cv_string_from_itemid(arena, lf.substr_list_id)); // TODO: print actual strings instead
    } break;
    case CV_LeafKind_POINTER: {
      CV_LeafPointer lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      CV_PointerKind kind = CV_PointerAttribs_Extract_Kind(lf.attribs);
      CV_PointerMode mode = CV_PointerAttribs_Extract_Mode(lf.attribs);
      
      rd_printf("Type   : %S", cv_string_from_itype(scratch.arena, min_itype, lf.itype));
      rd_printf("Attribs: %S", cv_string_from_pointer_attribs(arena, lf.attribs));
      rd_indent();
      if (mode == CV_PointerMode_PtrMem) {
        CV_TypeIndex         itype = 0;
        CV_MemberPointerKind pm    = 0;
        cursor += str8_deserial_read_struct(raw_leaf, cursor, &itype);
        cursor += str8_deserial_read_struct(raw_leaf, cursor, &pm);
        
        rd_printf("Class Type: %S", cv_string_from_itype(scratch.arena, min_itype, itype));
        rd_printf("Format    : %S", cv_string_from_member_pointer_kind(pm));
      } else {
        if (kind == CV_PointerKind_BaseSeg) {
          U16 seg;
          cursor += str8_deserial_read_struct(raw_leaf, cursor, &seg);
          
          rd_printf("Base Segment: %#04x", seg);
        } else if (kind == CV_PointerKind_BaseType) {
          CV_TypeIndex base_itype = 0;
          String8      name       = {0};
          cursor += str8_deserial_read_struct(raw_leaf, cursor, &base_itype);
          cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
          
          rd_printf("Base Type: %S", cv_string_from_itype(scratch.arena, min_itype, base_itype));
          rd_printf("Name     : %S", name);
        }
      }
      rd_unindent();
    } break;
    case CV_LeafKind_UNION_ST: 
    case CV_LeafKind_UNION: {
      CV_LeafUnion     lf   = {0};
      CV_NumericParsed num  = {0};
      String8          name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &num);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name       : %S",       name);
      rd_printf("Field Count: %u",       lf.count);
      rd_printf("Properties : %#x (%S)", lf.props, cv_string_from_type_props(scratch.arena, lf.props));
      rd_printf("Field      : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.field_itype));
      rd_printf("Size       : %S",       cv_string_from_numeric(scratch.arena, num));
      if (lf.props & CV_TypeProp_HasUniqueName) {
        String8 unique_name = {0};
        cursor += str8_deserial_read_cstr(raw_leaf, cursor, &unique_name);
        rd_printf("Unique Name: %S", unique_name);
      }
    } break;
    case CV_LeafKind_CLASS_ST: 
    case CV_LeafKind_STRUCTURE_ST: 
    case CV_LeafKind_CLASS:
    case CV_LeafKind_STRUCTURE: {
      CV_LeafStruct    lf  = {0};
      CV_NumericParsed num  = {0};
      String8          name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &num);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name           : %S",       name);
      rd_printf("Field Count    : %u",       lf.count);
      rd_printf("Properties     : %#x (%S)", lf.props, cv_string_from_type_props(scratch.arena, lf.props));
      rd_printf("Field List Type: %S",       cv_string_from_itype(scratch.arena, min_itype, lf.field_itype));
      rd_printf("Derived Type   : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.derived_itype));
      rd_printf("VShape         : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.vshape_itype));
      rd_printf("Size           : %S",       cv_string_from_numeric(scratch.arena, num));
      if (lf.props & CV_TypeProp_HasUniqueName) {
        String8 unique_name = {0};
        cursor += str8_deserial_read_cstr(raw_leaf, cursor, &unique_name);
        rd_printf("Unique Name:      %S", unique_name);
      }
    } break;
    case CV_LeafKind_SUBSTR_LIST:
    case CV_LeafKind_ARGLIST: {
      CV_LeafArgList lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Types %u", lf.count);
      rd_indent();
      for (U32 i = 0; i < lf.count; ++i) {
        U32 itype = 0;
        cursor += str8_deserial_read_struct(raw_leaf, cursor, &itype);
        rd_printf("%S", cv_string_from_itype(scratch.arena, min_itype, itype));
      }
      rd_unindent();
    } break;
    case CV_LeafKind_PROCEDURE: {
      CV_LeafProcedure lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      String8 call_kind    = cv_string_from_call_kind(lf.call_kind);
      String8 func_attribs = cv_string_from_function_attribs(scratch.arena, lf.attribs);
      
      rd_printf("Return type       : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.ret_itype));
      rd_printf("Call Convention   : %#x (%S)", lf.call_kind, call_kind);
      rd_printf("Function Attribs  : %#x (%S)", lf.attribs, func_attribs);
      rd_printf("Argumnet Count    : %u",       lf.arg_count);
      rd_printf("Argument List Type: %S",       cv_string_from_itype(scratch.arena, min_itype, lf.arg_itype));
    } break;
    case CV_LeafKind_FUNC_ID: {
      CV_LeafFuncId lf  = {0};
      String8       name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name      : %S",       name);
      rd_printf("Scope Type: %#x (%S)", lf.scope_string_id, cv_string_from_itype(scratch.arena, min_itype, lf.scope_string_id));
      rd_printf("Type      : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.itype));
    } break;
    case CV_LeafKind_MODIFIER: {
      CV_LeafModifier lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Type : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.itype));
      rd_printf("Flags: %#x (%S)", lf.flags, cv_string_from_modifier_flags(scratch.arena, lf.flags));
    } break;
    case CV_LeafKind_ARRAY_ST: 
    case CV_LeafKind_ARRAY: {
      CV_LeafArray     lf  = {0};
      CV_NumericParsed num  = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &num);
      
      rd_printf("Entry type: %S", cv_string_from_itype(scratch.arena, min_itype, lf.entry_itype));
      rd_printf("Index type: %S", cv_string_from_itype(scratch.arena, min_itype, lf.index_itype));
      rd_printf("Length    : %S", cv_string_from_numeric(scratch.arena, num));
    } break;
    case CV_LeafKind_FIELDLIST: {
      for (U64 idx = 0; cursor < raw_leaf.size;) {
        U16 member_type = 0;
        cursor += str8_deserial_read_struct(raw_leaf, cursor, &member_type);
        String8 raw_member = str8_skip(raw_leaf, cursor);
        
        rd_printf("list[%u] = %S", idx++, cv_string_from_leaf_name(arena, member_type));
        rd_indent();
        cursor += cv_print_leaf(arena, out, indent, min_itype, member_type, raw_member);
        cursor  = AlignPow2(cursor, 4);
        rd_unindent();
      }
    } break;
    case CV_LeafKind_MEMBER_ST:
    case CV_LeafKind_MEMBER: {
      CV_LeafMember    lf   = {0};
      CV_NumericParsed num  = {0};
      String8          name = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      cursor += cv_read_numeric(raw_leaf, cursor, &num);
      cursor += str8_deserial_read_cstr(raw_leaf, cursor, &name);
      
      rd_printf("Name   : %S",       name);
      rd_printf("Attribs: %#x (%S)", lf.attribs, cv_string_from_field_attribs(scratch.arena, lf.attribs));
      rd_printf("Type   : %S",       cv_string_from_itype(scratch.arena, min_itype, lf.itype));
      rd_printf("Offset : %S",       cv_string_from_numeric(scratch.arena, num));
    } break;
    case CV_LeafKind_LABEL: {
      CV_LeafLabel lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      
      rd_printf("Kind: %S", cv_string_from_label_kind(scratch.arena, lf.kind));
    } break;
    case CV_LeafKind_ENDPRECOMP: {
      CV_LeafEndPreComp lf = {0};
      cursor += str8_deserial_read_struct(raw_leaf, cursor, &lf);
      rd_printf("Sig: %#x", lf.sig);
    } break;
    // 16bit
    case CV_LeafKind_OEM_16t: 
    case CV_LeafKind_MODIFIER_16t: 
    case CV_LeafKind_POINTER_16t: 
    case CV_LeafKind_ARRAY_16t: 
    case CV_LeafKind_CLASS_16t: 
    case CV_LeafKind_STRUCTURE_16t: 
    case CV_LeafKind_UNION_16t: 
    case CV_LeafKind_ENUM_16t: 
    case CV_LeafKind_PROCEDURE_16t: 
    case CV_LeafKind_MFUNCTION_16t: 
    case CV_LeafKind_COBOL0_16t: 
    case CV_LeafKind_BARRAY_16t: 
    case CV_LeafKind_DIMARRAY_16t: 
    case CV_LeafKind_VFTPATH_16t: 
    case CV_LeafKind_PRECOMP_16t: 
    case CV_LeafKind_ARGLIST_16t: 
    case CV_LeafKind_DEFARG_16t: 
    case CV_LeafKind_FIELDLIST_16t: 
    case CV_LeafKind_DERIVED_16t: 
    case CV_LeafKind_BITFIELD_16t: 
    case CV_LeafKind_METHODLIST_16t: 
    case CV_LeafKind_DIMCONU_16t: 
    case CV_LeafKind_DIMCONLU_16t: 
    case CV_LeafKind_DIMVARU_16t: 
    case CV_LeafKind_DIMVARLU_16t: 
    case CV_LeafKind_BCLASS_16t: 
    case CV_LeafKind_VBCLASS_16t: 
    case CV_LeafKind_IVBCLASS_16t: 
    case CV_LeafKind_ENUMERATE_ST: 
    case CV_LeafKind_FRIENDFCN_16t: 
    case CV_LeafKind_INDEX_16t: 
    case CV_LeafKind_MEMBER_16t: 
    case CV_LeafKind_STMEMBER_16t: 
    case CV_LeafKind_METHOD_16t: 
    case CV_LeafKind_NESTTYPE_16t: 
    case CV_LeafKind_VFUNCTAB_16t: 
    case CV_LeafKind_FRIENDCLS_16t: 
    case CV_LeafKind_ONEMETHOD_16t: 
    case CV_LeafKind_VFUNCOFF_16t: 
    case CV_LeafKind_ST_MAX: 
    // HLSL
    case CV_LeafKind_HLSL: 
    // COBOL
    case CV_LeafKind_COBOL0: 
    case CV_LeafKind_COBOL1: 
    // Manged
    case CV_LeafKind_MANAGED_ST: 
    // undefined
    case CV_LeafKind_LIST: 
    case CV_LeafKind_REFSYM: 
    case CV_LeafKind_BARRAY: 
    case CV_LeafKind_DIMARRAY_ST: 
    case CV_LeafKind_VFTPATH: 
    case CV_LeafKind_OEM: 
    case CV_LeafKind_ALIAS_ST: 
    case CV_LeafKind_OEM2: 
    case CV_LeafKind_DEFARG_ST: 
    case CV_LeafKind_DERIVED: 
    case CV_LeafKind_DIMCONU: 
    case CV_LeafKind_DIMCONLU: 
    case CV_LeafKind_DIMVARU: 
    case CV_LeafKind_DIMVARLU: 
    case CV_LeafKind_FRIENDFCN_ST: 
    case CV_LeafKind_INDEX: 
    case CV_LeafKind_FRIENDCLS: 
    case CV_LeafKind_VFUNCOFF: 
    case CV_LeafKind_MEMBERMODIFY_ST: 
    case CV_LeafKind_TYPESERVER_ST: 
    case CV_LeafKind_TYPESERVER: 
    case CV_LeafKind_DIMARRAY: 
    case CV_LeafKind_ALIAS: 
    case CV_LeafKind_DEFARG: 
    case CV_LeafKind_FRIENDFCN: 
    case CV_LeafKind_NESTTYPEEX: 
    case CV_LeafKind_MEMBERMODIFY: 
    case CV_LeafKind_MANAGED: 
    case CV_LeafKind_STRIDED_ARRAY: 
    case CV_LeafKind_MODIFIER_EX: 
    case CV_LeafKind_INTERFACE: 
    case CV_LeafKind_BINTERFACE: 
    case CV_LeafKind_VECTOR: 
    case CV_LeafKind_MATRIX: 
    case CV_LeafKind_VFTABLE: 
    case CV_LeafKind_UDT_MOD_SRC_LINE: {
      rd_errorf("TODO: %#x", kind);
    } break;
  }
  scratch_end(scratch);
  return cursor;
}

internal void
cv_print_debug_t(Arena *arena, String8List *out, String8 indent, CV_DebugT debug_t)
{
  Temp scratch = scratch_begin(&arena, 1);
  for (U64 lf_idx = 0; lf_idx < debug_t.count; ++lf_idx) {
    CV_Leaf      lf     = cv_debug_t_get_leaf(debug_t, lf_idx);
    U64          offset = (U64)(lf.data.str-debug_t.v[0]);
    CV_TypeIndex itype  = CV_MinComplexTypeIndex + lf_idx;
    rd_printf("LF_%S (%x) [%04llx-%04llx)", cv_string_from_leaf_kind(lf.kind), itype, offset, offset+lf.data.size);
    rd_indent();
    cv_print_leaf(arena, out, indent, CV_MinComplexTypeIndex, lf.kind, lf.data);
    rd_unindent();
  }
  scratch_end(scratch);
}

internal void
cv_print_symbols_c13(Arena *arena, String8List *out, String8 indent, CV_Arch arch, String8 raw_data)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  B32 scope_depth = 0;
  
  for (U64 cursor = 0; cursor < raw_data.size; ) {
    CV_SymbolHeader header = {0};
    cursor += str8_deserial_read_struct(raw_data, cursor, &header);
    
    if (header.size >= sizeof(header.kind)) {
      Temp temp = temp_begin(scratch.arena);
      
      U64     symbol_end = cursor + (header.size - sizeof(header.kind));
      String8 raw_symbol = str8_substr(raw_data, rng_1u64(cursor, symbol_end));
      
      if (header.kind == CV_SymKind_END || header.kind == CV_SymKind_INLINESITE_END) {
        if (scope_depth > 0) {
          rd_unindent();
          --scope_depth;
        } else {
          rd_errorf("unbalanced scopes");
        }
      }
      
      rd_printf("%S [%04llx-%04llx)", cv_string_from_symbol_type(temp.arena, header.kind), cursor, header.size-sizeof(header.size));
      rd_indent();
      cv_print_symbol(arena, out, indent, arch, CV_MinComplexTypeIndex, header.kind, raw_symbol);
      rd_unindent();
      
      if (header.kind == CV_SymKind_BLOCK32 || header.kind == CV_SymKind_INLINESITE) {
        rd_indent();
        ++scope_depth;
      }
      
      cursor = symbol_end;
      
      temp_end(temp);
    } else {
      rd_errorf("symbol must be at least two bytes long");
    }
  }
  
  scratch_end(scratch);
}

internal void 
cv_print_lines_c13(Arena *arena, String8List *out, String8 indent, String8 raw_lines)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64 cursor = 0;
  
  CV_C13SubSecLinesHeader header = {0};
  cursor += str8_deserial_read_struct(raw_lines, cursor, &header);
  
  B32 has_columns = !!(header.flags & CV_C13SubSecLinesFlag_HasColumns);
  if (has_columns) {
    rd_errorf("TOOD: columns");
  }
  
  rd_printf("%04x:%08x-%08x, flags = %04x", header.sec, header.sec_off, header.len, header.flags);
  
  for (; cursor < raw_lines.size; ) {
    CV_C13File file = {0};
    cursor += str8_deserial_read_struct(raw_lines, cursor, &file);
    
    rd_printf("file = %08x, line count = %u, block size %08x", file.file_off, file.num_lines, file.block_size);
    
    Temp        temp    = temp_begin(scratch.arena);
    String8List columns = {0};
    for (U32 line_idx = 0; line_idx < file.num_lines; ++line_idx) {
      CV_C13Line line = {0};
      cursor += str8_deserial_read_struct(raw_lines, cursor, &line);
      
      B32 always_step_in_line_number = line.off == 0xFEEFEE;
      B32 never_step_in_line_number  = line.off == 0xF00F00;
      
      U32 ln = CV_C13LineFlags_Extract_LineNumber(line.flags);
      //U32 delta   = CV_C13LineFlags_Extract_DeltaToEnd(line.flags);
      //B32 is_stmt = CV_C13LineFlags_Extract_Statement(line.flags);
      
      if (always_step_in_line_number || never_step_in_line_number) {
        str8_list_pushf(temp.arena, &columns, "%x %08X", ln, line.off);
      } else {
        str8_list_pushf(temp.arena, &columns, "%5u %08X", ln, line.off);
      }
      
      if ((line_idx+1) % 4 == 0 || (line_idx+1) == file.num_lines) {
        String8 line_str = str8_list_join(scratch.arena, &columns, &(StringJoin){.sep=str8_lit("\t")});
        rd_printf("%S", line_str);
        
        temp_end(temp);
        temp = temp_begin(scratch.arena);
        MemoryZeroStruct(&columns);
      }
    }
    temp_end(temp);
    
    if (cursor < raw_lines.size) {
      rd_newline();
    }
  }
  
  scratch_end(scratch);
}

internal void
cv_print_file_checksums(Arena *arena, String8List *out, String8 indent, String8 raw_chksums)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("%-8s %-8s %-8s %-16s", "File", "Size", "Type", "Checksum");
  for (U64 cursor = 0; cursor < raw_chksums.size; ) {
    CV_C13Checksum chksum = {0};
    cursor += str8_deserial_read_struct(raw_chksums, cursor, &chksum);
    
    Temp     temp       = temp_begin(scratch.arena);
    String8  chksum_str = str8_lit("???");
    U8      *chksum_ptr = str8_deserial_get_raw_ptr(raw_chksums, cursor, chksum.len);
    if (chksum_ptr) {
      chksum_str = rd_format_hex_array(temp.arena, chksum_ptr, chksum.len);
    }
    
    rd_printf("%08x %08x %-8S %S", 
              chksum.name_off, 
              chksum.len, 
              cv_string_from_c13_checksum_kind(chksum.kind),
              chksum_str);
    
    temp_end(temp);
    
    cursor += chksum.len;
    cursor = AlignPow2(cursor, CV_FileCheckSumsAlign);
  }
  
  scratch_end(scratch);
}

internal void
cv_print_string_table(Arena *arena, String8List *out, String8 indent, String8 raw_strtab)
{
  for (U64 cursor = 0; cursor < raw_strtab.size; ) {
    String8 str = {0};
    cursor += str8_deserial_read_cstr(raw_strtab, cursor, &str);
    rd_printf("%08x %S", cursor, str);
  }
}

internal void
cv_print_inlinee_lines(Arena *arena, String8List *out, String8 indent, String8 raw_data)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64 cursor = 0;
  
  U32 inlinee_sig = ~0u;
  cursor += str8_deserial_read_struct(raw_data, cursor, &inlinee_sig);
  
  switch (inlinee_sig) {
    case CV_C13InlineeLinesSig_NORMAL: {
      rd_printf("%-8s %-8s %-8s", "Inlinee", "File ID", "Base LN");
      for (; cursor < raw_data.size; ) {
        CV_C13InlineeSourceLineHeader line = {0};
        cursor += str8_deserial_read_struct(raw_data, cursor, &line);
        rd_printf("%08x %08x %8u", line.inlinee, line.file_off, line.first_source_ln);
      }
      
    } break;
    case CV_C13InlineeLinesSig_EXTRA_FILES: {
      rd_printf("%-8s %-8s %-8s %s", "Inlinee", "File ID", "Base LN", "Extra FileIDs");
      for (; cursor < raw_data.size; ) {
        Temp temp = temp_begin(scratch.arena);
        
        CV_C13InlineeSourceLineHeader line             = {0};
        U32                           extra_file_count = 0;
        cursor += str8_deserial_read_struct(raw_data, cursor, &line);
        cursor += str8_deserial_read_struct(raw_data, cursor, &extra_file_count);
        
        String8List extra_files_list = {0};
        for (U32 i = 0; i < extra_file_count; ++i) {
          U32 file_id = 0;
          cursor += str8_deserial_read_struct(raw_data, cursor, &file_id);
          str8_list_pushf(temp.arena, &extra_files_list, "%08x", file_id);
        }
        String8 extra_files = str8_list_join(temp.arena, &extra_files_list, &(StringJoin){.sep=str8_lit(" ,")});
        
        rd_printf("%08x %08x %u %S", line.inlinee, line.file_off, line.first_source_ln, extra_files);
        
        temp_end(temp);
      }
    } break;
  }
  
  scratch_end(scratch);
}

internal void
cv_print_symbols_section(Arena       *arena,
                         String8List *out,
                         String8      indent,
                         CV_Arch      arch,
                         String8      raw_ss)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  U64 cursor = 0;
  U32 cv_sig = 0;
  cursor += str8_deserial_read_struct(raw_ss, cursor, &cv_sig);
  
  for (; cursor < raw_ss.size; ) {
    U64                    sst_offset = 0;
    CV_C13SubSectionHeader ss_header  = {0};
    char                  *ss_ver     = "???";
    switch (cv_sig) {
      case CV_Signature_C6: {
        rd_printf("TODO: C6");
      } break;
      case CV_Signature_C7: {
        rd_printf("TODO: C7");
      } break;
      case CV_Signature_C11: {
        ss_header.kind = CV_C13SubSectionKind_Symbols;
        ss_header.size = raw_ss.size - sizeof(cv_sig);
        ss_ver         = "C11";
      } break;
      case CV_Signature_C13: {
        sst_offset = cursor;
        cursor += str8_deserial_read_struct(raw_ss, cursor, &ss_header);
        ss_ver = "C13";
      } break;
    }
    
    U64     sst_end = cursor + ss_header.size;
    String8 raw_sst = str8_substr(raw_ss, rng_1u64(cursor, sst_end));
    cursor = AlignPow2(sst_end, CV_C13SubSectionAlign);
    
    rd_printf("# %S %s [%llx-%llx)", cv_string_from_c13_subsection_kind(ss_header.kind), ss_ver, sst_offset, sst_end);
    rd_indent();
    switch (ss_header.kind) {
      case CV_C13SubSectionKind_Symbols: {
        cv_print_symbols_c13(arena, out, indent, arch, raw_sst);
        rd_newline();
      } break;
      case CV_C13SubSectionKind_Lines: {
        cv_print_lines_c13(arena, out, indent, raw_sst);
        rd_newline();
      } break;
      case CV_C13SubSectionKind_FileChksms: {
        cv_print_file_checksums(arena, out, indent, raw_sst);
        rd_newline();
      } break;
      case CV_C13SubSectionKind_StringTable: {
        cv_print_string_table(arena, out, indent, raw_sst);
        rd_newline();
      } break;
      case CV_C13SubSectionKind_InlineeLines: {
        cv_print_inlinee_lines(arena, out, indent, raw_sst);
        rd_newline();
      } break;
      case CV_C13SubSectionKind_FrameData: 
      case CV_C13SubSectionKind_CrossScopeImports:
      case CV_C13SubSectionKind_CrossScopeExports:
      case CV_C13SubSectionKind_IlLines:
      case CV_C13SubSectionKind_FuncMDTokenMap:
      case CV_C13SubSectionKind_TypeMDTokenMap:
      case CV_C13SubSectionKind_MergedAssemblyInput:
      case CV_C13SubSectionKind_CoffSymbolRVA:
      case CV_C13SubSectionKind_XfgHashType:
      case CV_C13SubSectionKind_XfgHashVirtual:
      default: {
        rd_printf("TODO");
      } break;
    }
    rd_unindent();
  }
  
  scratch_end(scratch);
}

internal void
cv_format_debug_sections(Arena *arena, String8List *out, String8 indent, String8 raw_image, String8 string_table, U64 section_count, COFF_SectionHeader *sections)
{
  CV_Arch arch = ~0;
  {
    B32 keep_parsing = 1;
    for (U64 i = 0; i < section_count && keep_parsing; ++i) {
      COFF_SectionHeader *header      = &sections[i];
      String8             sect_name   = coff_name_from_section_header(string_table, header);
      Rng1U64             sect_frange = rng_1u64(header->foff, header->foff+header->fsize);
      String8             raw_sect    = str8_substr(raw_image, sect_frange);
      if (str8_match_lit(".debug$S", sect_name, 0)) {
        Temp scratch = scratch_begin(&arena, 1);
        CV_DebugS debug_s = cv_parse_debug_s(scratch.arena, raw_sect);
        for (String8Node *string_n = debug_s.data_list[CV_C13SubSectionIdxKind_Symbols].first;
             string_n != 0 && keep_parsing; string_n = string_n->next) {
          Temp temp = temp_begin(scratch.arena);
          CV_SymbolList symbol_list = {0};
          cv_parse_symbol_sub_section(temp.arena, &symbol_list, 0, string_n->string, CV_SymbolAlign);
          for (CV_SymbolNode *symbol_n = symbol_list.first; symbol_n != 0 && keep_parsing; symbol_n = symbol_n->next) {
            CV_Symbol symbol = symbol_n->data;
            if (symbol.kind == CV_SymKind_COMPILE) {
              if (symbol.data.size >= sizeof(CV_SymCompile)) {
                CV_SymCompile *comp = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*comp));
                arch = comp->machine;
              } else {
                rd_printf("not enough bytes to read S_COMPILE");
              }
              keep_parsing = 0;
            } else if (symbol.kind == CV_SymKind_COMPILE2) {
              if (symbol.data.size >= sizeof(CV_SymCompile2)) {
                CV_SymCompile2 *comp = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*comp));
                arch = comp->machine;
              } else {
                rd_printf("not enough bytes to read S_COMPILE2");
              }
              keep_parsing = 0;
            } else if (symbol.kind == CV_SymKind_COMPILE3) {
              if (symbol.data.size >= sizeof(CV_SymCompile3)) {
                CV_SymCompile3 *comp = str8_deserial_get_raw_ptr(symbol.data, 0, sizeof(*comp));
                arch = comp->machine;
              } else {
                rd_printf("not enough bytes to read S_COMPILE3");
              }
              keep_parsing = 0;
            }
          }
          temp_end(temp);
        }
        scratch_end(scratch);
      }
    }
  }
  
  for (U64 i = 0; i < section_count; ++i) {
    COFF_SectionHeader *header      = &sections[i];
    String8             sect_name   = coff_name_from_section_header(string_table, header);
    Rng1U64             sect_frange = rng_1u64(header->foff, header->foff+header->fsize);
    String8             raw_sect    = str8_substr(raw_image, sect_frange);
    if (str8_match_lit(".debug$S", sect_name, 0)) {
      rd_printf("# .debug$S No. %llx", i+1);
      rd_indent();
      cv_print_symbols_section(arena, out, indent, arch, raw_sect);
      rd_unindent();
    } else if (str8_match_lit(".debug$T", sect_name, 0)) {
      Temp scratch = scratch_begin(&arena, 1);
      CV_Signature sig = 0;
      str8_deserial_read_struct(raw_sect, 0, &sig);
      
      String8 raw_types = str8_skip(raw_sect, sizeof(sig));
      CV_DebugT debug_t = {0};
      if (sig == CV_Signature_C13) {
        debug_t = cv_debug_t_from_data(scratch.arena, raw_types, CV_LeafAlign);
      } else {
        NotImplemented;
      }
      
      rd_printf("# .debug$T No. %llx", i+1);
      rd_indent();
      cv_print_debug_t(arena, out, indent, debug_t);
      rd_unindent();
      scratch_end(scratch);
    }
  }
}

// COFF

internal void
coff_print_archive_member_header(Arena *arena, String8List *out, String8 indent, COFF_ParsedArchiveMemberHeader header, String8 long_names)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header.time_stamp);
  
  rd_printf("Name      : %S"             , header.name    );
  rd_printf("Time Stamp: (%#x) %S"       , header.time_stamp, time_stamp     );
  rd_printf("User ID   : %u"             , header.user_id );
  rd_printf("Group ID  : %u"             , header.group_id);
  rd_printf("Mode      : %S"             , header.mode    );
  rd_printf("Data      : [%#llx-%#llx)", header.data_range.min, header.data_range.max);
  
  scratch_end(scratch);
}

internal void
coff_print_section_table(Arena               *arena,
                         String8List        *out,
                         String8             indent,
                         String8             string_table,
                         COFF_Symbol32Array  symbol_table,
                         U64                 section_count,
                         COFF_SectionHeader *section_table)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 *symlinks = push_array(scratch.arena, String8, section_count);
  for (U64 i = 0; i < symbol_table.count; ++i) {
    COFF_Symbol32              *symbol = symbol_table.v+i;
    COFF_SymbolValueInterpType  interp = coff_interp_symbol(symbol->section_number, symbol->value, symbol->storage_class);
    if (interp == COFF_SymbolValueInterp_Regular &&
        symbol->aux_symbol_count == 0 &&
        (symbol->storage_class == COFF_SymStorageClass_External || symbol->storage_class == COFF_SymStorageClass_Static)) {
      if (symbol->section_number > 0 && symbol->section_number <= symbol_table.count) {
        COFF_SectionHeader *header = section_table+(symbol->section_number-1);
        if (header->flags & COFF_SectionFlag_LnkCOMDAT) {
          symlinks[symbol->section_number-1] = coff_read_symbol_name(string_table, &symbol->name);
        }
      }
    }
    i += symbol->aux_symbol_count;
  }
  
  if (section_count) {
    rd_printf("# Section Table");
    rd_indent();
    
    rd_printf("%-4s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-5s %-10s %s",
              "No.",
              "Name",
              "VirtSize",
              "VirtOff",
              "FileSize",
              "FileOff",
              "RelocOff",
              "LinesOff",
              "RelocCnt",
              "LineCnt",
              "Align",
              "Flags",
              "Symlink");
    
    for (U64 i = 0; i < section_count; ++i) {
      COFF_SectionHeader *header = section_table+i;
      
      String8 name      = str8_cstring_capped(header->name, header->name+sizeof(header->name));
      String8 full_name = coff_name_from_section_header(string_table, header);
      
      String8 align;
      {
        U64 align_size = coff_align_size_from_section_flags(header->flags);
        align = push_str8f(scratch.arena, "%u", align_size);
      }
      
      String8 flags;
      {
        String8List mem_flags = {0};
        if (header->flags & COFF_SectionFlag_MemRead) {
          str8_list_pushf(scratch.arena, &mem_flags, "r");
        }
        if (header->flags & COFF_SectionFlag_MemWrite) {
          str8_list_pushf(scratch.arena, &mem_flags, "w");
        }
        if (header->flags & COFF_SectionFlag_MemExecute) {
          str8_list_pushf(scratch.arena, &mem_flags, "x");
        }
        
        String8List cnt_flags = {0};
        if (header->flags & COFF_SectionFlag_CntCode) {
          str8_list_pushf(scratch.arena, &cnt_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_CntInitializedData) {
          str8_list_pushf(scratch.arena, &cnt_flags, "d");
        }
        if (header->flags & COFF_SectionFlag_CntUninitializedData) {
          str8_list_pushf(scratch.arena, &cnt_flags, "u");
        }
        
        String8List mem_extra_flags = {0};
        if (header->flags & COFF_SectionFlag_MemShared) {
          str8_list_pushf(scratch.arena, &mem_flags, "s");
        }
        if (header->flags & COFF_SectionFlag_Mem16Bit) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "h");
        }
        if (header->flags & COFF_SectionFlag_MemLocked) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "l");
        }
        if (header->flags & COFF_SectionFlag_MemDiscardable) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "d");
        }
        if (header->flags & COFF_SectionFlag_MemNotCached) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_MemNotPaged) {
          str8_list_pushf(scratch.arena, &mem_extra_flags, "p");
        }
        
        String8List lnk_flags = {0};
        if (header->flags & COFF_SectionFlag_LnkRemove) {
          str8_list_pushf(scratch.arena, &lnk_flags, "r");
        }
        if (header->flags & COFF_SectionFlag_LnkCOMDAT) {
          str8_list_pushf(scratch.arena, &lnk_flags, "c");
        }
        if (header->flags & COFF_SectionFlag_LnkOther) {
          str8_list_pushf(scratch.arena, &lnk_flags, "o");
        }
        if (header->flags & COFF_SectionFlag_LnkInfo) {
          str8_list_pushf(scratch.arena, &lnk_flags, "i");
        }
        if (header->flags & COFF_SectionFlag_LnkNRelocOvfl) {
          str8_list_pushf(scratch.arena, &lnk_flags, "f");
        }
        
        String8List other_flags = {0};
        if (header->flags & COFF_SectionFlag_TypeNoPad) {
          str8_list_pushf(scratch.arena, &other_flags, "n");
        }
        if (header->flags & COFF_SectionFlag_GpRel) {
          str8_list_pushf(scratch.arena, &other_flags, "g");
        }
        
        String8 mem = str8_list_join(scratch.arena, &mem_flags, 0);
        String8 cnt = str8_list_join(scratch.arena, &cnt_flags, 0);
        String8 lnk = str8_list_join(scratch.arena, &lnk_flags, 0);
        String8 ext = str8_list_join(scratch.arena, &mem_extra_flags, 0);
        String8 oth = str8_list_join(scratch.arena, &other_flags, 0);
        
        String8List f = {0};
        str8_list_push(scratch.arena, &f, mem);
        str8_list_push(scratch.arena, &f, cnt);
        str8_list_push(scratch.arena, &f, ext);
        str8_list_push(scratch.arena, &f, lnk);
        str8_list_push(scratch.arena, &f, oth);
        
        flags = str8_list_join(scratch.arena, &f, &(StringJoin){ .sep = str8_lit("-") });
        
        if (!flags.size) {
          flags = str8_lit("none");
        }
      }
      
      String8List l = {0};
      str8_list_pushf(scratch.arena, &l, "%-4x",  i+1                );
      str8_list_pushf(scratch.arena, &l, "%-8S",  name               );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->vsize      );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->voff       );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->fsize      );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->foff       );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->relocs_foff);
      str8_list_pushf(scratch.arena, &l, "%08x",  header->lines_foff );
      str8_list_pushf(scratch.arena, &l, "%08x",  header->reloc_count);
      str8_list_pushf(scratch.arena, &l, "%08x",  header->line_count );
      str8_list_pushf(scratch.arena, &l, "%-5S",  align              );
      str8_list_pushf(scratch.arena, &l, "%-10S", flags              );
      if (symlinks[i].size > 0) {
        str8_list_pushf(scratch.arena, &l, "%S", symlinks[i]);
      } else {
        str8_list_pushf(scratch.arena, &l, "[no symlink]");
      }
      
      String8 line = str8_list_join(scratch.arena, &l, &(StringJoin){ .sep = str8_lit(" "), });
      rd_printf("%S", line);
      
      if (full_name.size != name.size) {
        rd_indent();
        rd_printf("Full Name: %S", full_name);
        rd_unindent();
      }
    }
    
    rd_newline();
    rd_printf("Flags:");
    rd_indent();
    rd_printf("r = MemRead    w = MemWrite        x = MemExecute");
    rd_printf("c = CntCode    d = InitializedData u = UninitializedData");
    rd_printf("s = MemShared  h = Mem16bit        l = MemLocked          d = MemDiscardable c = MemNotCached  p = MemNotPaged");
    rd_printf("r = LnkRemove  c = LnkComdat       o = LnkOther           i = LnkInfo        f = LnkNRelocOvfl");
    rd_printf("g = GpRel      n = TypeNoPad");
    rd_unindent();
    
    rd_unindent();
    rd_newline();
  }
  
  scratch_end(scratch);
}

internal void
coff_disasm_sections(Arena              *arena,
                     String8List        *out,
                     String8             indent,
                     String8             raw_data,
                     COFF_MachineType    machine,
                     U64                 image_base,
                     B32                 is_obj,
                     RD_MarkerArray     *section_markers,
                     U64                 section_count,
                     COFF_SectionHeader *sections)
{
  if (section_count) {
    for (U64 sect_idx = 0; sect_idx < section_count; ++sect_idx) {
      COFF_SectionHeader *sect = sections+sect_idx;
      if (sect->flags & COFF_SectionFlag_CntCode) {
        U64            sect_off    = is_obj ? sect->foff : sect->voff;
        U64            sect_size   = is_obj ? sect->fsize : sect->vsize;
        String8        raw_code    = str8_substr(raw_data, rng_1u64(sect->foff, sect->foff+sect_size));
        U64            sect_number = sect_idx+1;
        RD_MarkerArray markers     = section_markers[sect_number];
        
        rd_printf("# Disassembly [Section No. %#llx]", sect_number);
        rd_indent();
        rd_print_disasm(arena, out, indent, arch_from_coff_machine(machine), image_base, sect_off, markers.count, markers.v, raw_code);
        rd_unindent();
      }
    }
  }
}

internal void
coff_raw_data_sections(Arena              *arena,
                       String8List        *out,
                       String8             indent,
                       String8             raw_data,
                       B32                 is_obj,
                       RD_MarkerArray     *section_markers,
                       U64                 section_count,
                       COFF_SectionHeader *section_table)
{
  if (section_count) {
    for (U64 sect_idx = 0; sect_idx < section_count; ++sect_idx) {
      COFF_SectionHeader *sect = section_table+sect_idx;
      if (sect->fsize > 0) {
        U64         sect_size = is_obj ? sect->fsize : sect->vsize;
        String8     raw_sect  = str8_substr(raw_data, rng_1u64(sect->foff, sect->foff+sect_size));
        RD_MarkerArray markers   = section_markers[sect_idx];
        
        rd_printf("# Raw Data [Section No. %#llx]", (sect_idx+1));
        rd_indent();
        rd_print_raw_data(arena, out, indent, 32, markers.count, markers.v, raw_sect);
        rd_unindent();
        rd_newline();
      }
    }
  }
}

internal void
coff_print_relocs(Arena              *arena,
                  String8List        *out,
                  String8             indent,
                  String8             raw_data,
                  String8             string_table,
                  COFF_MachineType    machine,
                  U64                 sect_count,
                  COFF_SectionHeader *sect_headers,
                  COFF_Symbol32Array  symbols)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  B32 print_header = 1;
  
  for (U64 sect_idx = 0; sect_idx < sect_count; ++sect_idx) {
    COFF_SectionHeader *sect_header = sect_headers+sect_idx;
    COFF_RelocInfo      reloc_info  = coff_reloc_info_from_section_header(raw_data, sect_header);
    
    if (reloc_info.count) {
      if (print_header) {
        print_header = 0;
        rd_printf("# Relocations");
        rd_indent();
      }
      
      rd_printf("## Section %llx", sect_idx);
      rd_indent();
      
      rd_printf("%-4s %-8s %-16s %-16s %-8s %-7s", "No.", "Offset", "Type", "ApplyTo", "SymIdx", "SymName");
      
      for (U64 reloc_idx = 0; reloc_idx < reloc_info.count; ++reloc_idx) {
        COFF_Reloc *reloc      = (COFF_Reloc*)(raw_data.str + reloc_info.array_off) + reloc_idx;
        String8     type       = coff_string_from_reloc(machine, reloc->type);
        U64         apply_size = coff_apply_size_from_reloc(machine, reloc->type);
        
        U64 apply_foff = sect_header->foff + reloc->apply_off;
        if (apply_foff + apply_size > raw_data.size) {
          rd_errorf("out of bounds apply file offset %#llx in relocation %#llx", apply_foff, reloc_idx);
          break;
        }
        
        U64 raw_apply;
        AssertAlways(apply_size <= sizeof(raw_apply));
        MemoryCopy(&raw_apply, raw_data.str + apply_foff, apply_size);
        S64 apply = extend_sign64(raw_apply, apply_size);
        
        if (reloc->isymbol > symbols.count) {
          rd_errorf("out of bounds symbol index %u in relocation %#llx", reloc->isymbol, reloc_idx);
          break;
        }
        
        COFF_Symbol32 *symbol      = symbols.v+reloc->isymbol;
        String8        symbol_name = coff_read_symbol_name(string_table, &symbol->name);
        
        String8List line = {0};
        str8_list_pushf(scratch.arena, &line, "%-4x",  reloc_idx       );
        str8_list_pushf(scratch.arena, &line, "%08x",  reloc->apply_off);
        str8_list_pushf(scratch.arena, &line, "%-16S", type            );
        str8_list_pushf(scratch.arena, &line, "%016x", apply           );
        str8_list_pushf(scratch.arena, &line, "%S",    symbol_name     );
        
        String8 l = str8_list_join(scratch.arena, &line, &(StringJoin){.sep=str8_lit(" ")});
        rd_printf("%S", l);
      }
      
      rd_unindent();
    }
  }
  
  if (!print_header) {
    rd_unindent();
  }
  rd_newline();
  
  scratch_end(scratch);
}

internal void
coff_print_symbol_table(Arena              *arena,
                        String8List        *out,
                        String8             indent,
                        String8             raw_data,
                        B32                 is_big_obj,
                        String8             string_table,
                        COFF_Symbol32Array  symbols)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  if (symbols.count) {
    rd_printf("# Symbol Table");
    rd_indent();
    
    rd_printf("%-4s %-8s %-10s %-4s %-4s %-4s %-16s %-20s", 
              "No.", "Value", "SectNum", "Aux", "Msb", "Lsb", "Storage", "Name");
    
    for (U64 i = 0; i < symbols.count; ++i) {
      COFF_Symbol32 *symbol        = &symbols.v[i];
      String8        name          = coff_read_symbol_name(string_table, &symbol->name);
      String8        msb           = coff_string_from_sym_dtype(symbol->type.u.msb);
      String8        lsb           = coff_string_from_sym_type(symbol->type.u.lsb);
      String8        storage_class = coff_string_from_sym_storage_class(symbol->storage_class);
      String8        section_number;
      switch (symbol->section_number) {
        case COFF_Symbol_UndefinedSection: section_number = str8_lit("Undef"); break;
        case COFF_Symbol_AbsSection32:     section_number = str8_lit("Abs");   break;
        case COFF_Symbol_DebugSection32:   section_number = str8_lit("Debug"); break;
        default:                           section_number = push_str8f(scratch.arena, "%010x", symbol->section_number); break;
      }
      
      String8List line = {0};
      str8_list_pushf(scratch.arena, &line, "%-4x",  i                       );
      str8_list_pushf(scratch.arena, &line, "%08x",  symbol->value           );
      str8_list_pushf(scratch.arena, &line, "%-10S", section_number          );
      str8_list_pushf(scratch.arena, &line, "%-4u",  symbol->aux_symbol_count);
      str8_list_pushf(scratch.arena, &line, "%-4S",  msb                     );
      str8_list_pushf(scratch.arena, &line, "%-4S",  lsb                     );
      str8_list_pushf(scratch.arena, &line, "%-16S", storage_class           );
      str8_list_pushf(scratch.arena, &line, "%S",    name                    );
      
      String8 l = str8_list_join(scratch.arena, &line, &(StringJoin){.sep = str8_lit(" ")});
      rd_printf("%S", l);
      
      rd_indent();
      for (U64 k=i+1, c = i+symbol->aux_symbol_count; k <= c; ++k) {
        void *raw_aux = &symbols.v[k];
        switch (symbol->storage_class) {
          case COFF_SymStorageClass_External: {
            COFF_SymbolFuncDef *func_def = (COFF_SymbolFuncDef*)&symbols.v[k];
            rd_printf("Tag Index %#x, Total Size %#x, Line Numbers %#x, Next Function %#x", 
                      func_def->tag_index, func_def->total_size, func_def->ptr_to_ln, func_def->ptr_to_next_func);
          } break;
          case COFF_SymStorageClass_Function: {
            COFF_SymbolFunc *func = raw_aux;
            rd_printf("Ordinal Line Number %#x, Next Function %#x", func->ln, func->ptr_to_next_func);
          } break;
          case COFF_SymStorageClass_WeakExternal: {
            COFF_SymbolWeakExt *weak = raw_aux;
            String8             type = coff_string_from_weak_ext_type(weak->characteristics);
            rd_printf("Tag Index %#x, Characteristics %S", weak->tag_index, type);
          } break;
          case COFF_SymStorageClass_File: {
            COFF_SymbolFile *file = raw_aux;
            String8          name = str8_cstring_capped(file->name, file->name+sizeof(file->name));
            rd_printf("Name %S", name);
          } break;
          case COFF_SymStorageClass_Static: {
            COFF_SymbolSecDef *sd        = raw_aux;
            String8            selection = coff_string_from_comdat_select_type(sd->selection);
            U32 number = sd->number_lo;
            if (is_big_obj) {
              number |= (U32)sd->number_hi << 16;
            }
            if (number) {
              rd_printf("Length %x, Reloc Count %u, Line Count %u, Checksum %x, Section %x, Selection %S",
                        sd->length, sd->number_of_relocations, sd->number_of_ln, sd->check_sum, number, selection);
            } else {
              rd_printf("Length %x, Reloc Count %u, Line Count %u, Checksum %x",
                        sd->length, sd->number_of_relocations, sd->number_of_ln, sd->check_sum);
            }
          } break;
          default: {
            rd_printf("???");
          } break;
        }
      }
      
      i += symbol->aux_symbol_count;
      rd_unindent();
    }
    
    rd_unindent();
    rd_newline();
  }
  
  scratch_end(scratch);
}

internal void
coff_print_big_obj_header(Arena *arena, String8List *out, String8 indent, COFF_BigObjHeader *header)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header->time_stamp);
  String8 machine    = coff_string_from_machine_type(header->machine);
  
  rd_printf("# Big Obj");
  rd_indent();
  rd_printf("Time Stamp   : %#x (%S)",  header->time_stamp, time_stamp);
  rd_printf("Machine      : %#x (%S)",  header->machine, machine      );
  rd_printf("Section Count: %u",  header->section_count    );
  rd_printf("Symbol Table : %#x", header->symbol_table_foff);
  rd_printf("Symbol Count : %u",  header->symbol_count     );
  rd_unindent();
  
  scratch_end(scratch);
}

internal void
coff_print_file_header(Arena *arena, String8List *out, String8 indent, COFF_FileHeader *header)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header->time_stamp);
  String8 machine    = coff_string_from_machine_type(header->machine);
  String8 flags      = coff_string_from_flags(scratch.arena, header->flags);
  
  rd_printf("# COFF File Header");
  rd_indent();
  rd_printf("Time Stamp          : %#x (%S)", header->time_stamp, time_stamp                            );
  rd_printf("Machine             : %#x %S",   header->machine, machine                                  );
  rd_printf("Section Count       : %u",       header->section_count                                     );
  rd_printf("Symbol Table        : %#x",      header->symbol_table_foff                                 );
  rd_printf("Symbol Count        : %u",       header->symbol_count                                      );
  rd_printf("Optional Header Size: %#x (%m)", header->optional_header_size, header->optional_header_size);
  rd_printf("Flags               : %#x (%S)", header->flags, flags                                      );
  rd_unindent();
  
  scratch_end(scratch);
}

internal void
coff_print_import(Arena *arena, String8List *out, String8 indent, COFF_ParsedArchiveImportHeader *header)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 machine    = coff_string_from_machine_type(header->machine);
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, header->time_stamp);
  
  rd_printf("# Import");
  rd_indent();
  rd_printf("Version   : %u", header->version        );
  rd_printf("Machine   : %S", machine                );
  rd_printf("Time Stamp: %#x (%S)", header->time_stamp, time_stamp      );
  rd_printf("Data Size : %#x (%m)", header->data_size, header->data_size);
  rd_printf("Hint      : %u", header->hint_or_ordinal);
  rd_printf("Type      : %u", header->type           );
  rd_printf("Import By : %u", header->import_by      );
  rd_printf("Function  : %S", header->func_name      );
  rd_printf("DLL       : %S", header->dll_name       );
  rd_unindent();
  
  scratch_end(scratch);
}

internal void
coff_print_big_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  COFF_FileHeaderInfo header_info = coff_file_header_info_from_data(raw_data);
  
  String8 raw_header        = str8_substr(raw_data, header_info.header_range);
  String8 raw_section_table = str8_substr(raw_data, header_info.section_table_range);
  String8 raw_string_table  = str8_substr(raw_data, header_info.string_table_range);
  
  COFF_BigObjHeader  *big_obj       = (COFF_BigObjHeader *)raw_header.str;
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)raw_section_table.str;
  COFF_Symbol32Array  symbol_table  = coff_symbol_array_from_data_32(scratch.arena, raw_data, header_info.symbol_table_range.min, big_obj->symbol_count);
  
  if (opts & RD_Option_Headers) {
    coff_print_big_obj_header(arena, out, indent, big_obj);
    rd_newline();
  }
  
  if (opts & RD_Option_Sections) {
    Rng1U64 sect_headers_range = rng_1u64(sizeof(*big_obj), sizeof(*big_obj) + sizeof(COFF_SectionHeader)*big_obj->section_count);
    Rng1U64 symbols_range      = rng_1u64(big_obj->symbol_table_foff, big_obj->symbol_table_foff + sizeof(COFF_Symbol32)*big_obj->symbol_count);
    
    if (sect_headers_range.max > raw_data.size) {
      rd_errorf("not enough bytes to read big obj section headers");
      goto exit;
    }
    if (big_obj->symbol_count) {
      if (symbols_range.max > raw_data.size) {
        rd_errorf("not enough bytes to read big obj symbol table");
        goto exit;
      }
      if (contains_1u64(symbols_range, sect_headers_range.min) ||
          contains_1u64(symbols_range, sect_headers_range.max)) {
        rd_errorf("section headers and symbol table ranges overlap");
        goto exit;
      }
    }
    
    coff_print_section_table(arena, out, indent, raw_string_table, symbol_table, big_obj->section_count, section_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Relocs) {
    coff_print_relocs(arena, out, indent, raw_data, raw_string_table, big_obj->machine, big_obj->section_count, section_table, symbol_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Symbols) {
    coff_print_symbol_table(arena, out, indent, raw_data, 1, raw_string_table, symbol_table);
    rd_newline();
  }
  
  exit:;
  scratch_end(scratch);
}

internal void
coff_print_obj(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  COFF_FileHeaderInfo header_info = coff_file_header_info_from_data(raw_data);
  
  String8 raw_header        = str8_substr(raw_data, header_info.header_range);
  String8 raw_section_table = str8_substr(raw_data, header_info.section_table_range);
  String8 raw_string_table  = str8_substr(raw_data, header_info.string_table_range);
  
  COFF_FileHeader    *header        = (COFF_FileHeader *)raw_header.str;
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)raw_section_table.str;
  COFF_Symbol32Array  symbol_table  = coff_symbol_array_from_data_16(scratch.arena, raw_data, header_info.symbol_table_range.min, header->symbol_count);
  Arch                arch          = arch_from_coff_machine(header->machine);
  
  if (opts & RD_Option_Headers) {
    coff_print_file_header(arena, out, indent, header);
    rd_newline();
  }
  
  if (opts & RD_Option_Sections) {
    Rng1U64 sect_headers_range = rng_1u64(sizeof(*header), sizeof(*header) + sizeof(COFF_SectionHeader)*header->section_count);
    Rng1U64 symbols_range      = rng_1u64(header->symbol_table_foff, header->symbol_table_foff + sizeof(COFF_Symbol16)*header->symbol_count);
    
    if (sect_headers_range.max > raw_data.size) {
      rd_errorf("not enough bytes to read obj section headers");
      goto exit;
    }
    if (header->symbol_count) {
      if (symbols_range.max > raw_data.size) {
        rd_errorf("not enough bytes to read obj symbol table");
        goto exit;
      }
      if (contains_1u64(symbols_range, sect_headers_range.min) ||
          contains_1u64(symbols_range, sect_headers_range.max)) {
        rd_errorf("section headers and symbol table ranges overlap");
        goto exit;
      }
    }
    
    coff_print_section_table(arena, out, indent, raw_string_table, symbol_table, header->section_count, section_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Relocs) {
    coff_print_relocs(arena, out, indent, raw_data, raw_string_table, header->machine, header->section_count, section_table, symbol_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Symbols) {
    coff_print_symbol_table(arena, out, indent, raw_data, 0, raw_string_table, symbol_table);
    rd_newline();
  }
  
  RD_MarkerArray *section_markers = 0;
  if (opts & (RD_Option_Disasm|RD_Option_Rawdata)) {
    section_markers = rd_section_markers_from_coff_symbol_table(scratch.arena, raw_string_table, header->section_count, symbol_table);
  }
  
  if (opts & RD_Option_Rawdata) {
    coff_raw_data_sections(arena, out, indent, raw_data, 1, section_markers, header->section_count, section_table);
  }
  
  if (opts & RD_Option_Disasm) {
    coff_disasm_sections(arena, out, indent, raw_data, header->machine, 0, 1, section_markers, header->section_count, section_table);
    rd_newline();
  }
  
  if (opts & RD_Option_Codeview) {
    cv_format_debug_sections(arena, out, indent, raw_data, raw_string_table, header->section_count, section_table);
  }
  
  if (opts & RD_Option_Dwarf) {
    DW_Input dwarf_input = dw_input_from_coff_section_table(scratch.arena, raw_data, raw_string_table, header->section_count, section_table);
    dw_format(arena, out, indent, opts, &dwarf_input, arch, Image_CoffPe);
  }
  
  exit:;
  scratch_end(scratch);
}

internal void
coff_print_archive(Arena *arena, String8List *out, String8 indent, String8 raw_archive, RD_Option opts)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  COFF_ArchiveParse archive_parse = coff_archive_parse_from_data(raw_archive);
  
  if (archive_parse.error.size) {
    rd_errorf("%S", archive_parse.error);
    return;
  }
  
  COFF_ArchiveFirstMember first_member = archive_parse.first_member;
  {
    rd_printf("# First Header");
    rd_indent();
    
    rd_printf("Symbol Count     : %u",         first_member.symbol_count);
    rd_printf("String Table Size: %#llx (%M)", first_member.string_table.size, first_member.string_table.size);
    
    rd_printf("Members:");
    rd_indent();
    
    String8List string_table = str8_split_by_string_chars(scratch.arena, first_member.string_table, str8_lit("\0"), 0);
    
    if (string_table.node_count == first_member.member_offset_count) {
      String8Node *string_n = string_table.first;
      
      for (U64 i = 0; i < string_table.node_count; ++i, string_n = string_n->next) {
        U32 offset = from_be_u32(first_member.member_offsets[i]);
        rd_printf("[%4u] %#08x %S", i, offset, string_n->string);
      }
    } else {
      rd_errorf("Member offset count (%llu) doesn't match string table count (%llu)", first_member.member_offset_count);
    }
    
    rd_unindent();
    rd_unindent();
    rd_newline();
  }
  
  if (archive_parse.has_second_header) {
    COFF_ArchiveSecondMember second_member = archive_parse.second_member;
    
    rd_printf("# Second Header");
    rd_indent();
    
    rd_printf("Member Count     : %u",         second_member.member_count);
    rd_printf("Symbol Count     : %u",         second_member.symbol_count);
    rd_printf("String Table Size: %#llx (%M)", second_member.string_table.size, second_member.string_table.size);
    
    String8List string_table = str8_split_by_string_chars(scratch.arena, second_member.string_table, str8_lit("\0"), 0);
    
    rd_printf("Members:");
    rd_indent();
    if (second_member.symbol_index_count == second_member.symbol_count) {
      String8Node *string_n = string_table.first;
      for (U64 i = 0; i < second_member.symbol_count; ++i, string_n = string_n->next) {
        U16 symbol_number = second_member.symbol_indices[i];
        if (symbol_number > 0 && symbol_number <= second_member.member_offset_count) {
          U16 symbol_idx    = symbol_number - 1;
          U32 member_offset = second_member.member_offsets[i];
          rd_printf("[%4u] %#08x %S", i, member_offset, string_n->string);
        } else {
          rd_errorf("[%4u] Out of bounds symbol number %u", i, symbol_number);
          break;
        }
      }
    } else {
      rd_errorf("Symbol index count %u doesn't match symbol count %u",
                second_member.symbol_index_count, second_member.symbol_count);
    }
    rd_unindent();
    
    rd_unindent();
    rd_newline();
  }
  
  if (archive_parse.has_long_names && opts & RD_Option_LongNames) {
    rd_printf("# Long Names");
    rd_indent();
    
    String8List long_names = str8_split_by_string_chars(scratch.arena, archive_parse.long_names, str8_lit("\0"), 0);
    U64 name_idx = 0;
    for (String8Node *name_n = long_names.first; name_n != 0; name_n = name_n->next, ++name_idx) {
      U64 offset = (U64)(name_n->string.str - archive_parse.long_names.str);
      rd_printf("[%-4u] %#08x %S", name_idx, offset, name_n->string);
    }
    
    rd_unindent();
    rd_newline();
  }
  
  U64  member_offset_count = 0;
  U32 *member_offsets      = 0;
  if (archive_parse.has_second_header) {
    member_offset_count = archive_parse.second_member.member_offset_count;
    member_offsets      = archive_parse.second_member.member_offsets;
  } else {
    HashTable *ht = hash_table_init(scratch.arena, 0x1000);
    for (U64 i = 0; i < archive_parse.first_member.member_offset_count; ++i) {
      U32 member_offset = from_be_u32(archive_parse.first_member.member_offsets[i]);
      if (!hash_table_search_u32(ht, member_offset)) {
        hash_table_push_u32_raw(scratch.arena, ht, member_offset, 0);
      }
    }
    member_offset_count = ht->count;
    member_offsets      = keys_from_hash_table_u32(scratch.arena, ht);
    radsort(member_offsets, member_offset_count, u32_is_before);
  }
  
  rd_printf("# Members");
  rd_indent();
  
  for (U64 i = 0; i < member_offset_count; ++i) {
    U64                next_member_offset = i+1 < member_offset_count ? member_offsets[i+1] : raw_archive.size;
    U64                member_offset      = member_offsets[i];
    String8            raw_member         = str8_substr(raw_archive, rng_1u64(member_offset, next_member_offset));
    COFF_ArchiveMember member             = coff_archive_member_from_data(raw_member);
    COFF_DataType      member_type        = coff_data_type_from_data(member.data);
    
    rd_printf("Member @ %#llx", member_offset);
    rd_indent();
    
    if (opts & RD_Option_Headers) {
      coff_print_archive_member_header(arena, out, indent, member.header, archive_parse.long_names);
      rd_newline();
    }
    
    switch (member_type) {
      case COFF_DataType_Obj: {
        coff_print_obj(arena, out, indent, member.data, opts);
      } break;
      case COFF_DataType_BigObj: {
        coff_print_big_obj(arena, out, indent, member.data, opts);
      } break;
      case COFF_DataType_Import: {
        if (opts & RD_Option_Headers) {
          COFF_ParsedArchiveImportHeader header = {0};
          U64 parse_size = coff_parse_import(member.data, 0, &header);
          if (parse_size) {
            coff_print_import(arena, out, indent, &header);
          } else {
            rd_errorf("not enough bytes to parse import header");
          }
        }
      } break;
      case COFF_DataType_Null: {
        rd_errorf("unknown member format", member_offset);
      } break;
    }
    
    rd_unindent();
    rd_newline();
  }
  
  rd_unindent();
  
  scratch_end(scratch);
}

// MSVC CRT

internal void
mscrt_print_eh_handler_type32(Arena *arena, String8List *out, String8 indent, RDI_Parsed *rdi, MSCRT_EhHandlerType32 *handler)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 catch_line     = rd_format_line_from_voff(scratch.arena, rdi, handler->catch_handler_voff, PathStyle_WindowsAbsolute);
  String8 adjectives_str = mscrt_string_from_eh_adjectives(scratch.arena, handler->adjectives);
  rd_printf("Adjectives               : %#x (%S)", handler->adjectives, adjectives_str);
  rd_printf("Descriptor               : %#x",      handler->descriptor_voff);
  rd_printf("Catch Object Frame Offset: %#x",      handler->catch_obj_frame_offset);
  rd_printf("Catch Handler            : %#x%s%S",  handler->catch_handler_voff, catch_line.size ? " " : "", catch_line);
  rd_printf("Delta to FP Handler      : %#x",      handler->fp_distance);
  scratch_end(scratch);
}

////////////////////////////////
//~ PE

internal void
pe_print_data_directory_ranges(Arena *arena, String8List *out, String8 indent, U64 count, PE_DataDirectory *dirs)
{
  Temp scratch = scratch_begin(&arena, 1);
  rd_printf("# Data Directories");
  rd_indent();
  for (U64 i = 0; i < count; ++i) {
    String8 dir_name;
    if (i < PE_DataDirectoryIndex_COUNT) {
      dir_name = pe_string_from_data_directory_index(i);
    } else {
      dir_name = push_str8f(scratch.arena, "%#x", i);
    }
    rd_printf("%-16S [%08x-%08x) %m", dir_name, dirs[i].virt_off, dirs[i].virt_off+dirs[i].virt_size, dirs[i].virt_size);
  }
  rd_unindent();
  scratch_end(scratch);
}

internal void
pe_print_optional_header32(Arena *arena, String8List *out, String8 indent, PE_OptionalHeader32 *opt_header, PE_DataDirectory *dirs)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 subsystem = pe_string_from_subsystem(opt_header->subsystem);
  String8 dll_chars = pe_string_from_dll_characteristics(scratch.arena, opt_header->dll_characteristics);
  
  rd_printf("# PE Optional Header 32");
  rd_indent();
  rd_printf("Magic                : %#x",        opt_header->magic);
  rd_printf("Linker version       : %u.%u",      opt_header->major_linker_version, opt_header->minor_linker_version);
  rd_printf("Size of code         : %#-8x (%m)", opt_header->sizeof_code, opt_header->sizeof_code);
  rd_printf("Size of inited data  : %#-8x (%m)", opt_header->sizeof_inited_data, opt_header->sizeof_inited_data);
  rd_printf("Size of uninited data: %#-8x (%m)", opt_header->sizeof_uninited_data, opt_header->sizeof_uninited_data);
  rd_printf("Entry point          : %#x",        opt_header->entry_point_va);
  rd_printf("Code base            : %#x",        opt_header->code_base);
  rd_printf("Data base            : %#x",        opt_header->data_base);
  rd_printf("Image base           : %#x",        opt_header->image_base);
  rd_printf("Section align        : %#x",        opt_header->section_alignment);
  rd_printf("File align           : %#x",        opt_header->file_alignment);
  rd_printf("OS version           : %u.%u",      opt_header->major_os_ver, opt_header->minor_os_ver);
  rd_printf("Image Version        : %u.%u",      opt_header->major_img_ver, opt_header->minor_img_ver);
  rd_printf("Subsystem version    : %u.%u",      opt_header->major_subsystem_ver, opt_header->minor_subsystem_ver);
  rd_printf("Win32 version        : %u",         opt_header->win32_version_value);
  rd_printf("Size of image        : %#x (%m)",   opt_header->sizeof_image, opt_header->sizeof_image);
  rd_printf("Size of headers      : %#x (%m)",   opt_header->sizeof_headers, opt_header->sizeof_headers);
  rd_printf("Checksum             : %#x",        opt_header->check_sum);
  rd_printf("Subsystem            : %#x (%S)",   opt_header->subsystem, subsystem);
  rd_printf("DLL Characteristics  : %#x (%S)",   opt_header->dll_characteristics, dll_chars);
  rd_printf("Stack reserve        : %#-8x (%m)", opt_header->sizeof_stack_reserve, opt_header->sizeof_stack_reserve);
  rd_printf("Stack commit         : %#-8x (%m)", opt_header->sizeof_stack_commit, opt_header->sizeof_stack_commit);
  rd_printf("Heap reserve         : %#-8x (%m)", opt_header->sizeof_heap_reserve, opt_header->sizeof_heap_reserve);
  rd_printf("Heap commit          : %#-8x (%m)", opt_header->sizeof_heap_commit, opt_header->sizeof_heap_commit);
  rd_printf("Loader flags         : %#x",        opt_header->loader_flags);
  rd_printf("RVA and offset count : %u",         opt_header->data_dir_count);
  rd_newline();
  
  pe_print_data_directory_ranges(arena, out, indent, opt_header->data_dir_count, dirs);
  rd_newline();
  
  rd_unindent();
  scratch_end(scratch);
}

internal void
pe_print_optional_header32plus(Arena *arena, String8List *out, String8 indent, PE_OptionalHeader32Plus *opt_header, PE_DataDirectory *dirs)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 subsystem = pe_string_from_subsystem(opt_header->subsystem);
  String8 dll_chars = pe_string_from_dll_characteristics(scratch.arena, opt_header->dll_characteristics);
  
  rd_printf("# PE Optional Header 32+");
  rd_indent();
  rd_printf("Magic                : %#x",          opt_header->magic);
  rd_printf("Linker version       : %u.%u",        opt_header->major_linker_version, opt_header->minor_linker_version);
  rd_printf("Size of code         : %#-8x (%m)",   opt_header->sizeof_code, opt_header->sizeof_code);
  rd_printf("Size of inited data  : %#-8x (%m)",   opt_header->sizeof_inited_data, opt_header->sizeof_inited_data);
  rd_printf("Size of uninited data: %#-8x (%m)",   opt_header->sizeof_uninited_data, opt_header->sizeof_uninited_data);
  rd_printf("Entry point          : %#x",          opt_header->entry_point_va);
  rd_printf("Code base            : %#x",          opt_header->code_base);
  rd_printf("Image base           : %#llx",        opt_header->image_base);
  rd_printf("Section align        : %#x",          opt_header->section_alignment);
  rd_printf("File align           : %#x",          opt_header->file_alignment);
  rd_printf("OS version           : %u.%u",        opt_header->major_os_ver, opt_header->minor_os_ver);
  rd_printf("Image Version        : %u.%u",        opt_header->major_img_ver, opt_header->minor_img_ver);
  rd_printf("Subsystem version    : %u.%u",        opt_header->major_subsystem_ver, opt_header->minor_subsystem_ver);
  rd_printf("Win32 version        : %u",           opt_header->win32_version_value);
  rd_printf("Size of image        : %#x (%m)",     opt_header->sizeof_image, opt_header->sizeof_image);
  rd_printf("Size of headers      : %#x (%m)",     opt_header->sizeof_headers, opt_header->sizeof_headers);
  rd_printf("Checksum             : %#x",          opt_header->check_sum);
  rd_printf("Subsystem            : %#llx (%S)",   opt_header->subsystem, subsystem);
  rd_printf("DLL Characteristics  : %#llx (%S)",   opt_header->dll_characteristics, dll_chars);
  rd_printf("Stack reserve        : %#-8llx (%M)", opt_header->sizeof_stack_reserve, opt_header->sizeof_stack_reserve);
  rd_printf("Stack commit         : %#-8llx (%M)", opt_header->sizeof_stack_commit, opt_header->sizeof_stack_commit);
  rd_printf("Heap reserve         : %#-8llx (%M)", opt_header->sizeof_heap_reserve, opt_header->sizeof_heap_reserve);
  rd_printf("Heap commit          : %#-8llx (%M)", opt_header->sizeof_heap_commit, opt_header->sizeof_heap_commit);
  rd_printf("Loader flags         : %#x",          opt_header->loader_flags);
  rd_printf("RVA and offset count : %u",           opt_header->data_dir_count);
  rd_newline();
  
  pe_print_data_directory_ranges(arena, out, indent, opt_header->data_dir_count, dirs);
  rd_newline();
  
  rd_unindent();
  scratch_end(scratch);
}

internal void
pe_print_load_config32(Arena *arena, String8List *out, String8 indent, PE_LoadConfig32 *lc)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 time_stamp        = coff_string_from_time_stamp(scratch.arena, lc->time_stamp);
  String8 global_flag_clear = pe_string_from_global_flags(scratch.arena, lc->global_flag_clear);
  String8 global_flag_set   = pe_string_from_global_flags(scratch.arena, lc->global_flag_set);
  
  rd_printf("# Load Config 32");
  rd_indent();
  
  rd_printf("Size:                          %m",       lc->size);
  rd_printf("Time stamp:                    %#x (%S)", lc->time_stamp, time_stamp);
  rd_printf("Version:                       %u.%u",    lc->major_version, lc->minor_version);
  rd_printf("Global flag clear:             %#x %S",   global_flag_clear);
  rd_printf("Global flag set:               %#x %S",   global_flag_set);
  rd_printf("Critical section timeout:      %u",       lc->critical_section_timeout);
  rd_printf("Decommit free block threshold: %#x",      lc->decommit_free_block_threshold);
  rd_printf("Decommit total free threshold: %#x",      lc->decommit_total_free_threshold);
  rd_printf("Lock prefix table:             %#x",      lc->lock_prefix_table);
  rd_printf("Maximum alloc size:            %m",       lc->maximum_allocation_size);
  rd_printf("Virtual memory threshold:      %m",       lc->virtual_memory_threshold);
  rd_printf("Process affinity mask:         %#x",      lc->process_affinity_mask);
  rd_printf("Process heap flags:            %#x",      lc->process_heap_flags);
  rd_printf("CSD version:                   %u",       lc->csd_version);
  rd_printf("Edit list:                     %#x",      lc->edit_list);
  rd_printf("Security Cookie:               %#x",      lc->security_cookie);
  if (lc->size < OffsetOf(PE_LoadConfig64, seh_handler_table)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("SEH Handler Table: %#x", lc->seh_handler_table);
  rd_printf("SEH Handler Count: %u",   lc->seh_handler_count);
  if (lc->size < OffsetOf(PE_LoadConfig64, guard_cf_check_func_ptr)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("Guard CF Check Function:    %#x", lc->guard_cf_check_func_ptr);
  rd_printf("Guard CF Dispatch Function: %#x", lc->guard_cf_dispatch_func_ptr);
  rd_printf("Guard CF Function Table:    %#x", lc->guard_cf_func_table);
  rd_printf("Guard CF Function Count:    %u",  lc->guard_cf_func_count);
  rd_printf("Guard Flags:                %#x", lc->guard_flags);
  if (lc->size < OffsetOf(PE_LoadConfig64, code_integrity)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("Code integrity:                        { Flags = %#x, Catalog = %#x, Catalog Offset = %#x }",
            lc->code_integrity.flags, lc->code_integrity.catalog, lc->code_integrity.catalog_offset);
  rd_printf("Guard address taken IAT entry table:   %#x", lc->guard_address_taken_iat_entry_table);
  rd_printf("Guard address taken IAT entry count:   %u",  lc->guard_address_taken_iat_entry_count);
  rd_printf("Guard long jump target table:          %#x", lc->guard_long_jump_target_table);
  rd_printf("Guard long jump target count:          %u",  lc->guard_long_jump_target_count);
  rd_printf("Dynamic value reloc table:             %#x", lc->dynamic_value_reloc_table);
  rd_printf("CHPE Metadata ptr:                     %#x", lc->chpe_metadata_ptr);
  rd_printf("Guard RF failure routine:              %#x", lc->guard_rf_failure_routine);
  rd_printf("Guard RF failure routine func ptr:     %#x", lc->guard_rf_failure_routine_func_ptr);
  rd_printf("Dynamic value reloc section:           %#x", lc->dynamic_value_reloc_table_section);
  rd_printf("Dynamic value reloc section offset:    %#x", lc->dynamic_value_reloc_table_offset);
  rd_printf("Guard RF verify SP func ptr:           %#x", lc->guard_rf_verify_stack_pointer_func_ptr);
  rd_printf("Hot patch table offset:                %#x", lc->hot_patch_table_offset);
  if (lc->size < OffsetOf(PE_LoadConfig64, enclave_config_ptr)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("Enclave config ptr:                    %#x", lc->enclave_config_ptr);
  rd_printf("Volatile metadata ptr:                 %#x", lc->volatile_metadata_ptr);
  rd_printf("Guard EH continuation table:           %#x", lc->guard_eh_continue_table);
  rd_printf("Guard EH continuation count:           %u",  lc->guard_eh_continue_count);
  rd_printf("Guard XFG check func ptr:              %#x", lc->guard_xfg_check_func_ptr);
  rd_printf("Guard XFG dispatch func ptr:           %#x", lc->guard_xfg_dispatch_func_ptr);
  rd_printf("Guard XFG table dispatch func ptr:     %#x", lc->guard_xfg_table_dispatch_func_ptr);
  rd_printf("Cast guard OS determined failure mode: %#x", lc->cast_guard_os_determined_failure_mode);
  rd_newline();
  
  exit:;
  rd_unindent();
  scratch_end(scratch);
}

internal void
pe_print_load_config64(Arena *arena, String8List *out, String8 indent, PE_LoadConfig64 *lc)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 time_stamp        = coff_string_from_time_stamp(scratch.arena, lc->time_stamp);
  String8 global_flag_clear = pe_string_from_global_flags(scratch.arena, lc->global_flag_clear);
  String8 global_flag_set   = pe_string_from_global_flags(scratch.arena, lc->global_flag_set);
  
  rd_printf("# Load Config 64");
  rd_indent();
  
  rd_printf("Size:                          %m",       lc->size);
  rd_printf("Time stamp:                    %#x (%S)", lc->time_stamp, time_stamp);
  rd_printf("Version:                       %u.%u",    lc->major_version, lc->minor_version);
  rd_printf("Global flag clear:             %#x %S",   lc->global_flag_clear, global_flag_clear);
  rd_printf("Global flag set:               %#x %S",   lc->global_flag_set, global_flag_set);
  rd_printf("Critical section timeout:      %u",       lc->critical_section_timeout);
  rd_printf("Decommit free block threshold: %#llx",    lc->decommit_free_block_threshold);
  rd_printf("Decommit total free threshold: %#llx",    lc->decommit_total_free_threshold);
  rd_printf("Lock prefix table:             %#llx",    lc->lock_prefix_table);
  rd_printf("Maximum alloc size:            %M",       lc->maximum_allocation_size);
  rd_printf("Virtual memory threshold:      %M",       lc->virtual_memory_threshold);
  rd_printf("Process affinity mask:         %#x",      lc->process_affinity_mask);
  rd_printf("Process heap flags:            %#x",      lc->process_heap_flags);
  rd_printf("CSD version:                   %u",       lc->csd_version);
  rd_printf("Edit list:                     %#llx",    lc->edit_list);
  rd_printf("Security Cookie:               %#llx",    lc->security_cookie);
  if (lc->size < OffsetOf(PE_LoadConfig64, seh_handler_table)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("SEH Handler Table: %#llx", lc->seh_handler_table);
  rd_printf("SEH Handler Count: %llu",  lc->seh_handler_count);
  if (lc->size < OffsetOf(PE_LoadConfig64, guard_cf_check_func_ptr)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("Guard CF Check Function:    %#llx", lc->guard_cf_check_func_ptr);
  rd_printf("Guard CF Dispatch Function: %#llx", lc->guard_cf_dispatch_func_ptr);
  rd_printf("Guard CF Function Table:    %#llx", lc->guard_cf_func_table);
  rd_printf("Guard CF Function Count:    %llu",  lc->guard_cf_func_count);
  rd_printf("Guard Flags:                %#x",   lc->guard_flags);
  if (lc->size < OffsetOf(PE_LoadConfig64, code_integrity)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("Code integrity:                      { Flags = %#x, Catalog = %#x, Catalog Offset = %#x }",
            lc->code_integrity.flags, lc->code_integrity.catalog, lc->code_integrity.catalog_offset);
  rd_printf("Guard address taken IAT entry table: %#llx", lc->guard_address_taken_iat_entry_table);
  rd_printf("Guard address taken IAT entry count: %llu",  lc->guard_address_taken_iat_entry_count);
  rd_printf("Guard long jump target table:        %#llx", lc->guard_long_jump_target_table);
  rd_printf("Guard long jump target count:        %llu",  lc->guard_long_jump_target_count);
  rd_printf("Dynamic value reloc table:           %#llx", lc->dynamic_value_reloc_table);
  rd_printf("CHPE Metadata ptr:                   %#llx", lc->chpe_metadata_ptr);
  rd_printf("Guard RF failure routine:            %#llx", lc->guard_rf_failure_routine);
  rd_printf("Guard RF failure routine func ptr:   %#llx", lc->guard_rf_failure_routine_func_ptr);
  rd_printf("Dynamic value reloc section:         %#llx", lc->dynamic_value_reloc_table_section);
  rd_printf("Dynamic value reloc section offset:  %#llx", lc->dynamic_value_reloc_table_offset);
  rd_printf("Guard RF verify SP func ptr:         %#llx", lc->guard_rf_verify_stack_pointer_func_ptr);
  rd_printf("Hot patch table offset:              %#llx", lc->hot_patch_table_offset);
  if (lc->size < OffsetOf(PE_LoadConfig64, enclave_config_ptr)) {
    goto exit;
  }
  rd_newline();
  
  rd_printf("Enclave config ptr:                    %#llx", lc->enclave_config_ptr);
  rd_printf("Volatile metadata ptr:                 %#llx", lc->volatile_metadata_ptr);
  rd_printf("Guard EH continuation table:           %#llx", lc->guard_eh_continue_table);
  rd_printf("Guard EH continuation count:           %llu",  lc->guard_eh_continue_count);
  rd_printf("Guard XFG check func ptr:              %#llx", lc->guard_xfg_check_func_ptr);
  rd_printf("Guard XFG dispatch func ptr:           %#llx", lc->guard_xfg_dispatch_func_ptr);
  rd_printf("Guard XFG table dispatch func ptr:     %#llx", lc->guard_xfg_table_dispatch_func_ptr);
  rd_printf("Cast guard OS determined failure mode: %#llx", lc->cast_guard_os_determined_failure_mode);
  rd_newline();
  
  exit:;
  rd_unindent();
  scratch_end(scratch);
}

internal void
pe_print_tls(Arena *arena, String8List *out, String8 indent, PE_ParsedTLS tls)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# TLS");
  rd_indent();
  
  String8 tls_chars = coff_string_from_section_flags(scratch.arena, tls.header.characteristics);
  rd_printf("Raw data start:    %#llx", tls.header.raw_data_start);
  rd_printf("Raw data end:      %#llx", tls.header.raw_data_end);
  rd_printf("Index address:     %#llx", tls.header.index_address);
  rd_printf("Callbacks address: %#llx", tls.header.callbacks_address);
  rd_printf("Zero-fill size:    %m",    tls.header.zero_fill_size);
  rd_printf("Characteristics:   %S",    tls_chars);
  
  if (tls.callback_count) {
    rd_newline();
    rd_printf("## Callbacks");
    rd_indent();
    for (U64 i = 0; i < tls.callback_count; ++i) {
      rd_printf("%#llx", tls.callback_addrs[i]);
    }
    rd_unindent();
  }
  
  rd_unindent();
  rd_newline();
  
  scratch_end(scratch);
}

internal void
pe_print_debug_diretory(Arena *arena, String8List *out, String8 indent, String8 raw_data, String8 raw_dir)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  rd_printf("# Debug Directory");
  rd_indent();
  
  PE_DebugInfoList debug_info_list = pe_parse_debug_directory(scratch.arena, raw_data, raw_dir);
  U64 i = 0;
  for (PE_DebugInfoNode *entry = debug_info_list.first; entry != 0; entry = entry->next, ++i) {
    PE_DebugInfo *de = &entry->v;
    
    if (entry != debug_info_list.first) {
      rd_newline();
    }
    
    rd_printf("Entry[%llu]", i);
    rd_indent();
    
    // print header
    rd_printf("Characteristics: %#x",   de->header.characteristics);
    rd_printf("Time Stamp:      %S",    coff_string_from_time_stamp(scratch.arena, de->header.time_stamp));
    rd_printf("Version:         %u.%u", de->header.major_ver, de->header.minor_ver);
    rd_printf("Type:            %S",    pe_string_from_debug_directory_type(de->header.type));
    rd_printf("Size:            %u",    de->header.size);
    rd_printf("Data virt off:   %#x",   de->header.voff);
    rd_printf("Data file off:   %#x",   de->header.foff);
    rd_newline();
    
    // print directory contents
    rd_indent();
    switch (de->header.type) {
      case PE_DebugDirectoryType_ILTCG:
      case PE_DebugDirectoryType_MPX:
      case PE_DebugDirectoryType_EXCEPTION:
      case PE_DebugDirectoryType_FIXUP:
      case PE_DebugDirectoryType_OMAP_TO_SRC:
      case PE_DebugDirectoryType_OMAP_FROM_SRC:
      case PE_DebugDirectoryType_BORLAND:
      case PE_DebugDirectoryType_CLSID:
      case PE_DebugDirectoryType_REPRO:
      case PE_DebugDirectoryType_EX_DLLCHARACTERISTICS: {
        NotImplemented;
      } break;
      case PE_DebugDirectoryType_COFF_GROUP: {
        U64 off = 0;
        
        // TODO: is this version?
        U32 unknown  = 0;
        off += str8_deserial_read_struct(de->u.raw_data, off, &unknown);
        if (unknown != 0) {
          rd_printf("TODO: unknown: %u", unknown);
        }
        
        rd_printf("%-8s %-8s %-8s", "VOFF", "Size", "Name");
        for (; off < de->u.raw_data.size; ) {
          U32     voff = 0;
          U32     size = 0;
          String8 name = str8_zero();
          
          off += str8_deserial_read_struct(de->u.raw_data, off, &voff);
          off += str8_deserial_read_struct(de->u.raw_data, off, &size);
          if (voff == 0 && size == 0) {
            break;
          }
          off += str8_deserial_read_cstr(de->u.raw_data, off, &name);
          off = AlignPow2(off, 4);
          
          rd_printf("%08x %08x %S", voff, size, name);
        }
      } break;
      case PE_DebugDirectoryType_VC_FEATURE: {
        MSCRT_VCFeatures *feat = str8_deserial_get_raw_ptr(de->u.raw_data, 0, sizeof(*feat));
        if (feat) {
          rd_printf("Pre-VC++ 11.0: %u", feat->pre_vcpp);
          rd_printf("C/C++:         %u", feat->c_cpp);
          rd_printf("/GS:           %u", feat->gs);
          rd_printf("/sdl:          %u", feat->sdl);
          rd_printf("guardN:        %u", feat->guard_n);
        } else {
          rd_errorf("not enough bytes to read VC Features");
        }
      } break;
      case PE_DebugDirectoryType_FPO: {
        PE_DebugFPO *fpo = str8_deserial_get_raw_ptr(de->u.raw_data, 0, sizeof(*fpo));
        if (fpo) {
          U8          prolog_size     = PE_FPOEncoded_Extract_PROLOG_SIZE(fpo->flags);
          U8          saved_regs_size = PE_FPOEncoded_Extract_SAVED_REGS_SIZE(fpo->flags);
          PE_FPOType  type            = PE_FPOEncoded_Extract_FRAME_TYPE(fpo->flags);
          PE_FPOFlags flags           = PE_FPOEncoded_Extract_FLAGS(fpo->flags);
          
          String8 type_string  = pe_string_from_fpo_type(type);
          String8 flags_string = pe_string_from_fpo_flags(scratch.arena, flags);
          
          rd_printf("Function offset: %#x", fpo->func_code_off);
          rd_printf("Function size:   %#x", fpo->func_size);
          rd_printf("Locals size:     %u",  fpo->locals_size);
          rd_printf("Params size:     %u",  fpo->params_size);
          rd_printf("Prolog size:     %u",  prolog_size);
          rd_printf("Saved regs size: %u",  saved_regs_size);
          rd_printf("Type:            %S",  type_string);
          rd_printf("Flags:           %S",  flags_string);
        } else {
          rd_errorf("not enough bytes to read FPO");
        }
      } break;
      case PE_DebugDirectoryType_CODEVIEW: {
        switch (de->u.codeview.magic) {
          case PE_CODEVIEW_PDB20_MAGIC: {
            PE_CvHeaderPDB20 *header = &de->u.codeview.pdb20.header;
            
            rd_printf("Time stamp: %S", coff_string_from_time_stamp(scratch.arena, header->time_stamp));
            rd_printf("Age:        %u", header->age);
            rd_printf("Name:       %S", de->u.codeview.pdb20.path);
          } break;
          case PE_CODEVIEW_PDB70_MAGIC: {
            PE_CvHeaderPDB70 *header = &de->u.codeview.pdb70.header;
            
            rd_printf("GUID: %S", string_from_guid(scratch.arena, header->guid));
            rd_printf("Age:  %u", header->age);
            rd_printf("Name: %S", de->u.codeview.pdb70.path);
          } break;
          case PE_CODEVIEW_RDI_MAGIC: {
            PE_CvHeaderRDI *header = &de->u.codeview.rdi.header;
            
            rd_printf("GUID: %S", string_from_guid(scratch.arena, header->guid));
            rd_printf("Name: %S", de->u.codeview.rdi.path);
          } break;
          default: {
            rd_errorf("unknown CodeView magic %#x", de->u.codeview.magic);
          } break;
        }
      } break;
      case PE_DebugDirectoryType_MISC: {
        PE_DebugMisc *misc = str8_deserial_get_raw_ptr(de->u.raw_data, 0, sizeof(*misc));
        
        String8 type_string = pe_string_from_misc_type(misc->data_type);
        
        rd_printf("Data type: %S", type_string);
        rd_printf("Size:      %u", misc->size);
        rd_printf("Unicode:   %u", misc->unicode);
        
        switch (misc->data_type) {
          case PE_DebugMiscType_EXE_NAME: {
            String8 name;
            str8_deserial_read_cstr(de->u.raw_data, sizeof(*misc), &name);
            rd_printf("Name: %S", name);
          } break;
          default: {
            rd_printf("???");
          } break;
        }
      } break;
    }
    rd_unindent();
    rd_unindent();
  }
  
  rd_unindent();
  rd_newline();
  
  scratch_end(scratch);
}

internal void
pe_print_export_table(Arena *arena, String8List *out, String8 indent, PE_ParsedExportTable exptab)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 time_stamp = coff_string_from_time_stamp(scratch.arena, exptab.time_stamp);
  
  rd_printf("# Export Table");
  rd_indent();
  
  rd_printf("Characteristics: %u",      exptab.flags);
  rd_printf("Time stamp:      %S",      time_stamp);
  rd_printf("Version:         %u.%02u", exptab.major_ver, exptab.minor_ver);
  rd_printf("Ordinal base:    %u",      exptab.ordinal_base);
  rd_printf("");
  
  rd_printf("%-4s %-8s %-8s %-8s", "No.", "Oridnal", "VOff", "Name");
  
  for (U64 i = 0; i < exptab.export_count; ++i) {
    PE_ParsedExport *exp = exptab.exports+i;
    if (exp->forwarder.size) {
      rd_printf("%4u %8u %8x %S (forwarded to %S)", i, exp->ordinal, exp->voff, exp->name, exp->forwarder);
    } else {
      rd_printf("%4u %8u %8x %S", i, exp->ordinal, exp->voff, exp->name);
    }
  }
  
  rd_unindent();
  scratch_end(scratch);
}

internal void
pe_print_static_import_table(Arena *arena, String8List *out, String8 indent, U64 image_base, PE_ParsedStaticImportTable imptab)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  if (imptab.count) {
    rd_printf("# Import Table");
    rd_indent();
    for (U64 dll_idx = 0; dll_idx < imptab.count; ++dll_idx) {
      PE_ParsedStaticDLLImport *dll = imptab.v+dll_idx;
      
      rd_printf("Name:                 %S",    dll->name);
      rd_printf("Import address table: %#llx", image_base + dll->import_address_table_voff);
      rd_printf("Import name table:    %#llx", image_base + dll->import_name_table_voff);
      rd_printf("Time stamp:           %#x",   dll->time_stamp);
      rd_newline();
      
      if (dll->import_count) {
        rd_indent();
        for (U64 imp_idx = 0; imp_idx < dll->import_count; ++imp_idx) {
          PE_ParsedImport *imp = dll->imports+imp_idx;
          if (imp->type == PE_ParsedImport_Ordinal) {
            rd_printf("%#-6x", imp->u.ordinal);
          } else if (imp->type == PE_ParsedImport_Name) {
            rd_printf("%#-6x %S", imp->u.name.hint, imp->u.name.string);
          }
        }
        rd_unindent();
        rd_newline();
      }
    }
    rd_unindent();
  }
  
  scratch_end(scratch);
}

internal void
pe_print_delay_import_table(Arena *arena, String8List *out, String8 indent, U64 image_base, PE_ParsedDelayImportTable imptab)
{
  if (imptab.count) {
    Temp scratch = scratch_begin(&arena, 1);
    rd_printf("# Delay Import Table");
    rd_indent();
    
    for (U64 dll_idx = 0; dll_idx < imptab.count; ++dll_idx) {
      PE_ParsedDelayDLLImport *dll = imptab.v+dll_idx;
      
      rd_printf("Attributes:               %#08x", dll->attributes);
      rd_printf("Name:                     %S",    dll->name);
      rd_printf("HMODULE address:          %#llx", image_base + dll->module_handle_voff);
      rd_printf("Import address table:     %#llx", image_base + dll->iat_voff);
      rd_printf("Import name table:        %#llx", image_base + dll->name_table_voff);
      rd_printf("Bound import name table:  %#llx", image_base + dll->bound_table_voff);
      rd_printf("Unload import name table: %#llx", image_base + dll->unload_table_voff);
      rd_printf("Time stamp:               %#x",   dll->time_stamp);
      rd_newline();
      
      rd_indent();
      for (U64 imp_idx = 0; imp_idx < dll->import_count; ++imp_idx) {
        PE_ParsedImport *imp = dll->imports+imp_idx;
        
        String8 bound = str8_lit("NULL");
        if (imp_idx < dll->bound_table_count) {
          U64 bound_addr = dll->bound_table[imp_idx];
          bound = push_str8f(scratch.arena, "%#llx", bound_addr);
        }
        
        String8 unload = str8_lit("NULL");
        if (imp_idx < dll->unload_table_count) {
          U64 unload_addr = dll->unload_table[imp_idx];
          unload = push_str8f(scratch.arena, "%#llx", unload_addr);
        }
        
        if (imp->type == PE_ParsedImport_Ordinal) {
          rd_printf("%-16S %-16S %#-4x", bound, unload, imp->u.ordinal);
        } else if (imp->type == PE_ParsedImport_Name) {
          rd_printf("%-16S %-16S %#-4x %S", bound, unload, imp->u.name.hint, imp->u.name.string);
        }
      }
      rd_unindent();
      
      rd_newline();
    }
    
    rd_unindent();
    scratch_end(scratch);
  }
}

internal void
pe_print_resources(Arena *arena, String8List *out, String8 indent, PE_ResourceDir *root)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // setup stack
  struct stack_s {
    struct stack_s  *next;
    B32              print_table;
    B32              is_named;
    PE_ResourceNode *curr_name_node;
    PE_ResourceNode *curr_id_node;
    U64              name_idx;
    U64              id_idx;
    U64              dir_idx;
    U64              dir_id;
    String8          dir_name;
    PE_ResourceDir  *table;
  } *stack = push_array(scratch.arena, struct stack_s, 1);
  stack->table          = root;
  stack->print_table    = 1;
  stack->is_named       = 1;
  stack->dir_name       = str8_lit("ROOT");
  stack->curr_name_node = root->named_list.first;
  stack->curr_id_node   = root->id_list.first;
  
  if (stack) {
    rd_printf("# Resources");
    
    // traverse resource tree
    while (stack) {
      if (stack->print_table) {
        stack->print_table = 0;
        rd_indent();
        
        if (stack->is_named) {
          rd_printf("[%u] %S { Time Stamp: %u, Version %u.%u Name Count: %u, ID Count %u, Characteristics: %u }", 
                    stack->dir_idx,
                    stack->dir_name,
                    stack->table->time_stamp, 
                    stack->table->major_version, stack->table->minor_version, 
                    stack->table->named_list.count, stack->table->id_list.count,
                    stack->table->characteristics);
        } else {
          B32 is_actually_leaf = stack->table->id_list.count == 1 && 
            stack->table->id_list.first->data.kind != PE_ResDataKind_DIR;
          if (is_actually_leaf) {
            rd_printf("[%u] %u { Time Stamp: %u, Version %u.%u Name Count: %u, ID Count %u, Characteristics: %u }", 
                      stack->dir_idx,
                      stack->dir_id,
                      stack->table->time_stamp, 
                      stack->table->major_version, stack->table->minor_version, 
                      stack->table->named_list.count, stack->table->id_list.count,
                      stack->table->characteristics);
          } else {
            String8 id_str = pe_resource_kind_to_string(stack->dir_id);
            rd_printf("[%u] %S { Time Stamp: %u, Version %u.%u Name Count: %u, ID Count %u, Characteristics: %u }", 
                      stack->dir_idx,
                      id_str,
                      stack->dir_id,
                      stack->table->time_stamp, 
                      stack->table->major_version, stack->table->minor_version, 
                      stack->table->named_list.count, stack->table->id_list.count,
                      stack->table->characteristics);
          }
        }
      }
      
      while (stack->curr_name_node) {
        PE_ResourceNode *named_node = stack->curr_name_node;
        stack->curr_name_node = stack->curr_name_node->next;
        U64 name_idx = stack->name_idx++;
        
        PE_Resource *res = &named_node->data;
        if (res->kind == PE_ResDataKind_DIR) {
          struct stack_s *frame = push_array(scratch.arena, struct stack_s, 1);
          frame->table          = res->u.dir;
          frame->print_table    = 1;
          frame->dir_idx        = stack->name_idx;
          frame->dir_name       = res->id.u.string;
          frame->is_named       = 1;
          frame->curr_name_node = frame->table->named_list.first;
          frame->curr_id_node   = frame->table->id_list.first;
          SLLStackPush(stack, frame);
          goto yield;
        } else if (res->kind == PE_ResDataKind_COFF_LEAF) {
          COFF_ResourceDataEntry *entry = &res->u.leaf;
          rd_printf("[%u] %S Data VOFF: %#08x, Data Size: %#08x, Code Page: %u", 
                    name_idx, res->id.u.string, entry->data_voff, entry->data_size, entry->code_page);
        } else {
          InvalidPath;
        }
      }
      
      while (stack->curr_id_node) {
        PE_ResourceNode *id_node = stack->curr_id_node;
        PE_Resource     *res     = &id_node->data;
        stack->curr_id_node = stack->curr_id_node->next;
        U64 id_idx = stack->id_idx++;
        
        if (res->kind == PE_ResDataKind_DIR) {
          struct stack_s *frame = push_array(scratch.arena, struct stack_s, 1);
          frame->table          = res->u.dir;
          frame->print_table    = 1;
          frame->dir_idx        = stack->table->named_list.count + id_idx;
          frame->dir_id         = res->id.u.number;
          frame->curr_name_node = frame->table->named_list.first;
          frame->curr_id_node   = frame->table->id_list.first;
          SLLStackPush(stack, frame);
          goto yield;
        } else if (res->kind == PE_ResDataKind_COFF_LEAF) {
          COFF_ResourceDataEntry *entry = &res->u.leaf;
          rd_printf("[%u] ID: %u Data VOFF: %#08x, Data Size: %#08x, Code Page: %u", id_idx, res->id.u.number, entry->data_voff, entry->data_size, entry->code_page);
        } else {
          InvalidPath;
        }
      }
      
      if (stack->curr_id_node == 0 && stack->curr_name_node == 0) {
        rd_unindent();
      }
      
      SLLStackPop(stack);
      
      yield:;
    }
    
    rd_newline();
  }
  
  scratch_end(scratch);
}

internal void
pe_print_exceptions_x8664(Arena              *arena,
                          String8List        *out,
                          String8             indent,
                          U64                 section_count,
                          COFF_SectionHeader *sections,
                          String8             raw_data,
                          Rng1U64             except_frange,
                          RDI_Parsed         *rdi)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8 raw_except = str8_substr(raw_data, except_frange);
  U64     count      = raw_except.size / sizeof(PE_IntelPdata);
  for (U64 i = 0; i < count; ++i) {
    Temp temp = temp_begin(scratch.arena);
    
    U64            pdata_offset = i * sizeof(PE_IntelPdata);
    PE_IntelPdata *pdata        = str8_deserial_get_raw_ptr(raw_except, pdata_offset, sizeof(*pdata));
    String8        pdata_name   = rd_proc_name_from_voff(rdi, pdata->voff_first);
    
    U64            unwind_info_offset = coff_foff_from_voff(sections, section_count, pdata->voff_unwind_info);
    PE_UnwindInfo *uwinfo             = str8_deserial_get_raw_ptr(raw_data, unwind_info_offset, sizeof(*uwinfo));
    
    U8 version        = PE_UNWIND_INFO_VERSION_FROM_HDR(uwinfo->header);
    U8 flags          = PE_UNWIND_INFO_FLAGS_FROM_HDR(uwinfo->header);
    U8 frame_register = PE_UNWIND_INFO_REG_FROM_FRAME(uwinfo->frame);
    U8 frame_offset   = PE_UNWIND_INFO_OFF_FROM_FRAME(uwinfo->frame);
    
    B32 is_chained       = (flags & PE_UnwindInfoFlag_CHAINED) != 0;
    B32 has_handler_data = !is_chained && (flags & (PE_UnwindInfoFlag_EHANDLER | PE_UnwindInfoFlag_UHANDLER)) != 0;
    
    String8 flags_str = str8_zero();
    {
      U64 f = flags;
      
      String8List flags_list = {0};
      if (f & PE_UnwindInfoFlag_EHANDLER) {
        f &= ~PE_UnwindInfoFlag_EHANDLER;
        str8_list_pushf(scratch.arena, &flags_list, "EHANDLER");
      }
      if (f & PE_UnwindInfoFlag_UHANDLER) {
        f &= ~PE_UnwindInfoFlag_UHANDLER;
        str8_list_pushf(scratch.arena, &flags_list, "UHANDLER");
      }
      if (f & PE_UnwindInfoFlag_CHAINED) {
        f &= ~PE_UnwindInfoFlag_CHAINED;
        str8_list_pushf(scratch.arena, &flags_list, "CHAINED");
      }
      if (f) {
        str8_list_pushf(scratch.arena, &flags_list, "%#llx", f);
      }
      if (flags_list.node_count == 0) {
        str8_list_pushf(scratch.arena, &flags_list, "%#llx", f);
      }
      flags_str = str8_list_join(scratch.arena, &flags_list, &(StringJoin){.sep=str8_lit(", ")});
    }
    
    U64            codes_offset = unwind_info_offset + sizeof(PE_UnwindInfo);
    PE_UnwindCode *code_ptr     = str8_deserial_get_raw_ptr(raw_data, codes_offset, sizeof(*code_ptr) * uwinfo->codes_num);
    PE_UnwindCode *code_opl     = code_ptr + uwinfo->codes_num;
    
    if (i > 0) {
      rd_newline();
    }
    rd_printf("%08x %08x %08x %08x%s%S",
              pdata_offset,
              pdata->voff_first,
              pdata->voff_one_past_last,
              pdata->voff_unwind_info,
              pdata_name.size ? " " : "", pdata_name);
    rd_printf("Version:     %u",  version);
    rd_printf("Flags:       %S",  flags_str);
    rd_printf("Prolog Size: %#x", uwinfo->prolog_size);
    rd_printf("Code Count:  %u",  uwinfo->codes_num);
    rd_printf("Frame:       %u",  uwinfo->frame);
    rd_printf("Codes:");
    rd_indent();
    for (; code_ptr < code_opl;) {
      Temp code_temp = temp_begin(scratch.arena);
      String8List code_list = {0};
      
      U8 operation_code = PE_UNWIND_OPCODE_FROM_FLAGS(code_ptr[0].flags);
      U8 operation_info = PE_UNWIND_INFO_FROM_FLAGS(code_ptr[0].flags);
      
      str8_list_pushf(code_temp.arena, &code_list, "%#04x:", code_ptr[0].off_in_prolog);
      switch (operation_code) {
        case PE_UnwindOpCode_PUSH_NONVOL: {
          String8 gpr = pe_string_from_unwind_gpr_x64(operation_info);
          str8_list_pushf(code_temp.arena, &code_list, "PUSH_NONVOL %S", gpr);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_ALLOC_LARGE: {
          U64 size = 0;
          switch (operation_info) {
            case 0: { // 136B - 512K
              size = code_ptr[1].u16*8;
            } break;
            case 1: { // 512K - 4GB
              size = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
            } break;
            default: break;
          }
          str8_list_pushf(code_temp.arena, &code_list, "ALLOC_LARGE size=%#x", size);
          code_ptr += 2;
        } break;
        case PE_UnwindOpCode_ALLOC_SMALL: {
          U64 size = operation_info*8 + 8;
          str8_list_pushf(code_temp.arena, &code_list, "ALLOC_SMALL size=%#x", size);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SET_FPREG: {
          U64     offset = frame_offset*16;
          String8 gpr    = pe_string_from_unwind_gpr_x64(frame_register);
          str8_list_pushf(code_temp.arena, &code_list, "SET_FPREG %S, offset=%#x", gpr, offset);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SAVE_NONVOL: {
          String8 gpr             = pe_string_from_unwind_gpr_x64(operation_info);
          U64     register_offset = code_ptr[1].u16*8;
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_NONVOL %S, offset=%#x", gpr, register_offset);
          code_ptr += 2;
        } break;
        case PE_UnwindOpCode_SAVE_NONVOL_FAR: {
          String8 gpr          = pe_string_from_unwind_gpr_x64(operation_info);
          U64     frame_offset = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_NONVOL_FAR %S, offset=%#x", gpr, frame_offset);
          code_ptr += 3;
        } break;
        case PE_UnwindOpCode_EPILOG: {
          str8_list_pushf(code_temp.arena, &code_list, "EPILOG flags=%#x", code_ptr[0].flags);
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SPARE_CODE: {
          str8_list_pushf(code_temp.arena, &code_list, "SPARE_CODE");
          code_ptr += 1;
        } break;
        case PE_UnwindOpCode_SAVE_XMM128: {
          String8 gpr             = pe_string_from_unwind_gpr_x64(operation_info);
          U64     register_offset = code_ptr[1].u16*16;
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_XMM128 %S, offset=%#x", gpr, register_offset);
          code_ptr += 2;
        } break;
        case PE_UnwindOpCode_SAVE_XMM128_FAR: {
          String8 gpr          = pe_string_from_unwind_gpr_x64(operation_info);
          U64     frame_offset = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
          str8_list_pushf(code_temp.arena, &code_list, "SAVE_XMM128_FAR %S, offset=%#x", gpr, frame_offset);
          code_ptr += 3;
        } break;
        case PE_UnwindOpCode_PUSH_MACHFRAME: {
          str8_list_pushf(code_temp.arena, &code_list, "PUSH_MACHFRAME %s", operation_info == 1  ? "with error code" : "without error code");
          code_ptr += 1;
        } break;
        default: {
          str8_list_pushf(code_temp.arena, &code_list, "UNKNOWN_OPCODE %#x", operation_code);
          code_ptr += 1;
        } break;
      }
      
      String8 code_line = str8_list_join(code_temp.arena, &code_list, &(StringJoin){.sep=str8_lit(" ")});
      rd_printf("%S", code_line);
      
      temp_end(code_temp);
    }
    rd_unindent();
    
    if (is_chained) {
      U64            next_pdata_offset = codes_offset + sizeof(PE_UnwindCode) * AlignPow2(uwinfo->codes_num, 2);
      PE_IntelPdata *next_pdata        = str8_deserial_get_raw_ptr(raw_data, next_pdata_offset, sizeof(*next_pdata));
      rd_printf("Chained: %#08x %#08x %#08x", next_pdata->voff_first, next_pdata->voff_one_past_last, next_pdata->voff_unwind_info);
    }
    
    if (has_handler_data) {
#define ExceptionHandlerDataFlag_FuncInfo   (1 << 0)
#define ExceptionHandlerDataFlag_FuncInfo4  (1 << 1)
#define ExceptionHandlerDataFlag_ScopeTable (1 << 2)
#define ExceptionHandlerDataFlag_GS         (1 << 3u)
      
      U64 actual_code_count = PE_UNWIND_INFO_GET_CODE_COUNT(uwinfo->codes_num);
      U64 read_cursor       = codes_offset + actual_code_count * sizeof(PE_UnwindCode);
      
      U32 handler = 0; 
      read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &handler);
      
      String8 handler_name = rd_proc_name_from_voff(rdi, handler);
      rd_printf("Handler: %#llx%s%S", handler, handler_name.size ? " " : "", handler_name);
      
      U32 handler_data_flags = 0;
      if (str8_match_lit("__GSHandlerCheck_EH4", handler_name, 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo4;
      } else if (str8_match_lit("__CxxFrameHandler4", handler_name, 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo4;
      } else if (str8_match_lit("__CxxFrameHandler3", handler_name, 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo;
      } else if (str8_match_lit("__C_specific_handler", handler_name, 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_ScopeTable;
      } else if (str8_match_lit("__GSHandlerCheck", handler_name, 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_GS;
      } else if (str8_match_lit("__GSHandlerCheck_SEH", handler_name, 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_ScopeTable|ExceptionHandlerDataFlag_GS;
      } else if (str8_match_lit("__GSHandlerCheck_EH", handler_name, 0)) {
        handler_data_flags = ExceptionHandlerDataFlag_FuncInfo|ExceptionHandlerDataFlag_GS;
      }
      
      if (handler_data_flags & ExceptionHandlerDataFlag_FuncInfo) {
        MSCRT_FuncInfo func_info;
        read_cursor += mscrt_parse_func_info(arena, raw_data, section_count, sections, read_cursor, &func_info);
        
        rd_printf("Function Info:");
        rd_indent();
        rd_printf("Magic:                      %#x", func_info.magic);
        rd_printf("Max State:                  %u",  func_info.max_state);
        rd_printf("Try Block Count:            %u",  func_info.try_block_map_count);
        rd_printf("IP Map Count:               %u",  func_info.ip_map_count);
        rd_printf("Frame Offset Unwind Helper: %#x", func_info.frame_offset_unwind_helper);
        rd_printf("ES Flags:                   %#x", func_info.eh_flags);
        rd_unindent();
        
        if (func_info.ip_map_count > 0) {
          rd_printf("IP to State Map:");
          rd_indent();
          rd_printf("%8s %8s", "State", "IP");
          for (U32 i = 0; i < func_info.ip_map_count; ++i) {
            MSCRT_IPState32 state = func_info.ip_map[i];
            String8 line = rd_format_line_from_voff(scratch.arena, rdi, state.ip, PathStyle_WindowsAbsolute);
            rd_printf("%8d %08x %S", state.state, state.ip, line);
          }
          rd_unindent();
        }
        
        if (func_info.max_state > 0) {
          rd_printf("Unwind Map:");
          rd_indent();
          rd_printf("%13s  %10s  %8s", "Current State", "Next State", "Action @");
          for (U32 i = 0; i < func_info.max_state; ++i) {
            MSCRT_UnwindMap32 map = func_info.unwind_map[i];
            String8 line = rd_format_line_from_voff(scratch.arena, rdi, map.action_virt_off, PathStyle_WindowsAbsolute);
            rd_printf("%13u  %10d  %8x %S", i, map.next_state, map.action_virt_off, line);
          }
          rd_unindent();
        }
        
        for (U32 i = 0; i < func_info.try_block_map_count; ++i) {
          MSCRT_TryMapBlock try_block = func_info.try_block_map[i];
          rd_printf("Try Map Block #%u", i);
          rd_indent();
          rd_printf("Try State Low:    %u", try_block.try_low);
          rd_printf("Try State High:   %u", try_block.try_high);
          rd_printf("Catch State High: %u", try_block.catch_high);
          rd_printf("Catch Count:      %u", try_block.catch_handlers_count);
          rd_printf("Catches:");
          rd_indent();
          for (U32 ihandler = 0; ihandler < try_block.catch_handlers_count; ++ihandler) {
            rd_printf("Catch #%u", ihandler);
            rd_indent();
            mscrt_print_eh_handler_type32(arena, out, indent, rdi, &try_block.catch_handlers[ihandler]);
            rd_unindent();
          }
          rd_unindent();
          rd_unindent();
        }
        
        if (func_info.es_type_list.count) {
          rd_printf("Exception Specific Types:");
          rd_indent();
          for (U32 i = 0; i < func_info.es_type_list.count; ++i) {
            if (i > 0) {
              rd_newline();
            }
            mscrt_print_eh_handler_type32(arena, out, indent, rdi, &func_info.es_type_list.handlers[i]);
          }
          rd_unindent();
        }
      }
      if (handler_data_flags & ExceptionHandlerDataFlag_FuncInfo4) {
        U32 func_info_voff = 0, unknown = 0;
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &func_info_voff);
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &unknown);
        
        U64                    func_info_foff = coff_foff_from_voff(sections, section_count, func_info_voff);
        MSCRT_ParsedFuncInfoV4 func_info      = {0};
        mscrt_parse_func_info_v4(arena, raw_data, section_count, sections, func_info_foff, pdata->voff_first, &func_info);
        
        String8 header_str = str8_zero();
        {
          String8List header_list = {0};
          if (func_info.header & MSCRT_FuncInfoV4Flag_IsCatch) {
            str8_list_pushf(scratch.arena, &header_list, "IsCatch");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_IsSeparated) {
            str8_list_pushf(scratch.arena, &header_list, "IsSeparted");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_IsBBT) {
            str8_list_pushf(scratch.arena, &header_list, "IsBBT");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_UnwindMap) {
            str8_list_pushf(scratch.arena, &header_list, "UnwindMap");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_TryBlockMap) {
            str8_list_pushf(scratch.arena, &header_list, "TryBlockMap");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_EHs) {
            str8_list_pushf(scratch.arena, &header_list, "EHs");
          }
          if (func_info.header & MSCRT_FuncInfoV4Flag_NoExcept) {
            str8_list_pushf(scratch.arena, &header_list, "NoExcept");
          }
          header_str = str8_list_join(scratch.arena, &header_list, &(StringJoin){.sep=str8_lit(", ")});
        }
        
        rd_printf("Function Info V4:");
        rd_indent();
        rd_printf("Header:                %#x %S", func_info.header, header_str);
        rd_printf("BBT Flags:             %#x",    func_info.bbt_flags);
        
        MSCRT_IP2State32V4 ip2state_map = func_info.ip2state_map;
        rd_printf("IP To State Map:");
        rd_indent();
        rd_printf("%8s %8s", "State", "IP");
        for (U32 i = 0; i < ip2state_map.count; ++i) {
          String8 line_str = rd_format_line_from_voff(scratch.arena, rdi, ip2state_map.voffs[i], PathStyle_WindowsAbsolute);
          rd_printf("%8d %08X %S", ip2state_map.states[i], ip2state_map.voffs[i], line_str);
        }
        rd_unindent();
        
        if (func_info.header & MSCRT_FuncInfoV4Flag_UnwindMap) {
          MSCRT_UnwindMapV4 unwind_map = func_info.unwind_map;
          rd_printf("Unwind Map:");
          rd_indent();
          for (U32 i = 0; i < unwind_map.count; ++i) {
            MSCRT_UnwindEntryV4 *ue       = &unwind_map.v[i];
            String8                 type_str = str8_zero();
            switch (ue->type) {
              case MSCRT_UnwindMapV4Type_NoUW:             type_str = str8_lit("NoUW");             break;
              case MSCRT_UnwindMapV4Type_DtorWithObj:      type_str = str8_lit("DtorWithObj");      break;
              case MSCRT_UnwindMapV4Type_DtorWithPtrToObj: type_str = str8_lit("DtorWithPtrToObj"); break;
              case MSCRT_UnwindMapV4Type_VOFF:             type_str = str8_lit("VOFF");             break;
            }
            if (ue->type == MSCRT_UnwindMapV4Type_DtorWithObj || ue->type == MSCRT_UnwindMapV4Type_DtorWithPtrToObj) {
              rd_printf("[%2u] NextOff=%u Type=%-16S Action=%#08x Object=%#x", i, ue->next_off, type_str, ue->action, ue->object);
            } else if (ue->type == MSCRT_UnwindMapV4Type_VOFF) {
              rd_printf("[%2u] NextOff=%u Type=%-16S Action=%#08x", i, ue->next_off, type_str, ue->action);
            } else {
              rd_printf("[%2u] NextOff=%u Type=%S", i, ue->next_off, type_str);
            }
          }
          rd_unindent();
        }
        
        if (func_info.header & MSCRT_FuncInfoV4Flag_TryBlockMap) {
          MSCRT_TryBlockMapV4Array try_block_map = func_info.try_block_map;
          rd_printf("Try/Catch Blocks:");
          rd_indent();
          for (U32 i = 0; i < try_block_map.count; ++i) {
            MSCRT_TryBlockMapV4 *try_block = &try_block_map.v[i];
            rd_printf("[%2u] TryLow %u TryHigh %u CatchHigh %u", i, try_block->try_low, try_block->try_high, try_block->catch_high);
            if (try_block->handlers.count) {
              for (U32 k = 0; k < try_block->handlers.count; ++k) {
                MSCRT_EhHandlerTypeV4 *handler = &try_block->handlers.v[k];
                
                String8List line_list = {0};
                str8_list_pushf(arena, &line_list, "  ");
                str8_list_pushf(arena, &line_list, "CatchCodeVOff=%#08X", handler->catch_code_voff);
                if (handler->flags & MSCRT_EhHandlerV4Flag_Adjectives) {
                  String8 adjectives = mscrt_string_from_eh_adjectives(arena, handler->adjectives);
                  str8_list_pushf(arena, &line_list, "Adjectives=%S", adjectives);
                }
                if (handler->flags & MSCRT_EhHandlerV4Flag_DispType) {
                  str8_list_pushf(arena, &line_list, "TypeVOff=%#x", handler->type_voff);
                }
                if (handler->flags & MSCRT_EhHandlerV4Flag_DispCatchObj) {
                  str8_list_pushf(arena, &line_list, "CacthObjVOff=%#x", handler->catch_obj_voff);
                }
                if (handler->flags & MSCRT_EhHandlerV4Flag_ContIsVOff) {
                  str8_list_pushf(arena, &line_list, "ContIsVOff");
                }
                for (U32 icont = 0; icont < handler->catch_funclet_cont_addr_count; ++icont) {
                  str8_list_pushf(arena, &line_list, "ContAddr[%u]=%#llx", icont, handler->catch_funclet_cont_addr[icont]);
                }
                
                String8 handler_str = str8_list_join(arena, &line_list, &(StringJoin){.sep=str8_lit(" ")});
                rd_printf("%S", handler_str);
              }
            }
          }
          rd_unindent();
        }
      }
      if (handler_data_flags & ExceptionHandlerDataFlag_ScopeTable) {
        U32 scope_count = 0;
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &scope_count);
        
        PE_HandlerScope *scopes = str8_deserial_get_raw_ptr(raw_data, read_cursor, sizeof(PE_HandlerScope)*scope_count); 
        read_cursor += scope_count*sizeof(scopes[0]);
        
        rd_printf("Count of scope table entries: %u", scope_count);
        rd_indent();
        rd_printf("%-8s %-8s %-8s %-8s", "Begin", "End", "Handler", "Target");
        for (U32 i = 0; i < scope_count; ++i) {
          PE_HandlerScope scope = scopes[i];
          rd_printf("%08x %08x %08x %08x", scope.begin, scope.end, scope.handler, scope.target);
        }
        rd_unindent();
      }
      if (handler_data_flags & ExceptionHandlerDataFlag_GS) {
        U32 gs_data = 0;
        read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &gs_data);
        
        U32 flags               = MSCRT_GSHandler_GetFlags(gs_data);
        U32 cookie_offset       = MSCRT_GSHandler_GetCookieOffset(gs_data);
        U32 aligned_base_offset = 0;
        U32 alignment           = 0;
        if (flags & MSCRT_GSHandlerFlag_HasAlignment) {
          read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &aligned_base_offset);
          read_cursor += str8_deserial_read_struct(raw_data, read_cursor, &alignment);
        }
        
        String8 flags_str;
        {
          String8List flags_list = {0};
          if (flags & MSCRT_GSHandlerFlag_EHandler) {
            str8_list_pushf(arena, &flags_list, "EHandler");
          }
          if (flags & MSCRT_GSHandlerFlag_UHandler) {
            str8_list_pushf(arena, &flags_list, "UHandler");
          }
          if (flags & MSCRT_GSHandlerFlag_HasAlignment) {
            str8_list_pushf(arena, &flags_list, "Has Alignment");
          }
          if (flags == 0) {
            str8_list_pushf(arena, &flags_list, "None");
          }
          flags_str = str8_list_join(arena, &flags_list, &(StringJoin){.sep=str8_lit(", ")});
        }
        rd_printf("GS unwind flags:     %S", flags_str);
        rd_printf("Cookie offset:       %x", cookie_offset);
        if (flags & MSCRT_GSHandlerFlag_HasAlignment) {
          rd_printf("Aligned base offset: %x", aligned_base_offset);
          rd_printf("Alignment:           %x", alignment);
        }
      }
      
#undef ExceptionHandlerDataFlag_FuncInfo
#undef ExceptionHandlerDataFlag_ScopeTable
#undef ExceptionHandlerDataFlag_GS
    }
    
    temp_end(temp);
  } 
  
  scratch_end(scratch);
}

internal void
pe_print_exceptions(Arena              *arena,
                    String8List        *out,
                    String8             indent,
                    COFF_MachineType    machine,
                    U64                 section_count,
                    COFF_SectionHeader *sections,
                    String8             raw_data,
                    Rng1U64             except_frange,
                    RDI_Parsed         *rdi)
{
  if (dim_1u64(except_frange)) {
    rd_printf("# Exceptions");
    rd_indent();
    rd_printf("%-8s %-8s %-8s %-8s", "Offset", "Begin", "End", "Unwind Info");
    switch (machine) {
      case COFF_MachineType_Unknown: break;
      case COFF_MachineType_X64:
      case COFF_MachineType_X86: {
        pe_print_exceptions_x8664(arena, out, indent, section_count, sections, raw_data, except_frange, rdi);
      } break;
      default: NotImplemented; break;
    }
    rd_unindent();
    rd_newline();
  }
}

internal void
pe_print_base_relocs(Arena              *arena,
                     String8List        *out,
                     String8             indent,
                     COFF_MachineType    machine,
                     U64                 image_base,
                     U64                 section_count,
                     COFF_SectionHeader *sections,
                     String8             raw_data,
                     Rng1U64             base_reloc_franges,
                     RDI_Parsed         *rdi)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  String8               raw_base_relocs = str8_substr(raw_data, base_reloc_franges);
  PE_BaseRelocBlockList base_relocs     = pe_base_reloc_block_list_from_data(scratch.arena, raw_base_relocs);
  
  if (base_relocs.count) {
    rd_printf("# Base Relocs");
    rd_indent();
    
    U32 addr_size = 0;
    switch (machine) {
      case COFF_MachineType_Unknown: break;
      case COFF_MachineType_X86:     addr_size = 4; break;
      case COFF_MachineType_X64:     addr_size = 8; break;
      default: NotImplemented;
    }
    
    // convert blocks to string list
    U64 iblock = 0;
    for (PE_BaseRelocBlockNode *node = base_relocs.first; node != 0; node = node->next) {
      PE_BaseRelocBlock *block = &node->v;
      rd_printf("Block No. %u, Virt Off %#x, Reloc Count %u", iblock++, block->page_virt_off, block->entry_count);
      rd_indent();
      for (U64 ientry = 0; ientry < block->entry_count; ++ientry) {
        PE_BaseRelocKind type   = PE_BaseRelocKindFromEntry(block->entries[ientry]);
        U16              offset = PE_BaseRelocOffsetFromEntry(block->entries[ientry]);
        
        U64 apply_to_voff = block->page_virt_off + offset;
        U64 apply_to_foff = coff_foff_from_voff(sections, section_count, apply_to_voff);
        U64 apply_to      = 0;
        str8_deserial_read(raw_data, apply_to_foff, &apply_to, addr_size, 1);
        U64 addr = image_base + apply_to;
        
        const char *type_str = "???";
        switch (type) {
          case PE_BaseRelocKind_ABSOLUTE: type_str = "ABS";     break;
          case PE_BaseRelocKind_HIGH:     type_str = "HIGH";    break;
          case PE_BaseRelocKind_LOW:      type_str = "LOW";     break;
          case PE_BaseRelocKind_HIGHLOW:  type_str = "HIGHLOW"; break;
          case PE_BaseRelocKind_HIGHADJ:  type_str = "HIGHADJ"; break;
          case PE_BaseRelocKind_DIR64:    type_str = "DIR64";   break;
          default: {
            switch (machine) {
              case COFF_MachineType_Arm:
              case COFF_MachineType_Arm64:
              case COFF_MachineType_ArmNt: {
                switch (type) {
                  case PE_BaseRelocKind_ARM_MOV32:   type_str = "ARM_MOV32";   break;
                  case PE_BaseRelocKind_THUMB_MOV32: type_str = "THUMB_MOV32"; break;
                  default: NotImplemented;
                }
              } break;
              // TODO: mips, loong, risc-v
            }
          } break;
        }
        
        if (type == PE_BaseRelocKind_ABSOLUTE) {
          rd_printf("%-4x %-12s", offset, type_str);
        } else {
          U64     reloc_voff = apply_to - image_base;
          String8 name       = rd_format_proc_line(scratch.arena, rdi, reloc_voff);
          rd_printf("%-4x %-12s %016llx%s%S", offset, type_str, apply_to, name.size ? " " : "", name);
        }
      }
      rd_unindent();
      rd_newline();
    }
    
    rd_unindent();
  }
  
  scratch_end(scratch);
}

internal void
pe_print(Arena *arena, String8List *out, String8 indent, String8 raw_data, RD_Option opts, RDI_Parsed *rdi)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  PE_DosHeader *dos_header = str8_deserial_get_raw_ptr(raw_data, 0, sizeof(*dos_header));
  if (!dos_header) {
    rd_errorf("not enough bytes to read DOS header");
    goto exit;
  }
  Assert(dos_header->magic == PE_DOS_MAGIC);
  
  U32 pe_magic = 0;
  str8_deserial_read_struct(raw_data, dos_header->coff_file_offset, &pe_magic);
  if (pe_magic != PE_MAGIC) {
    rd_errorf("PE magic check failure, input file is not of PE format");
    goto exit;
  }
  
  U64              file_header_off = dos_header->coff_file_offset+sizeof(pe_magic);
  COFF_FileHeader *file_header     = str8_deserial_get_raw_ptr(raw_data, file_header_off, sizeof(*file_header));
  if (!file_header) {
    rd_errorf("not enough bytes to read COFF header");
    goto exit;
  }
  
  U64 opt_header_off   = file_header_off + sizeof(*file_header);
  U16 opt_header_magic = 0;
  str8_deserial_read_struct(raw_data, opt_header_off, &opt_header_magic);
  if (opt_header_magic != PE_PE32_MAGIC && opt_header_magic != PE_PE32PLUS_MAGIC) {
    rd_errorf("unexpected optional header magic %#x", opt_header_magic);
    goto exit;
  }
  
  if (opt_header_magic == PE_PE32_MAGIC && file_header->optional_header_size < sizeof(PE_OptionalHeader32)) {
    rd_errorf("unexpected optional header size in COFF header %m, expected at least %m", file_header->optional_header_size, sizeof(PE_OptionalHeader32));
    goto exit;
  }
  
  if (opt_header_magic == PE_PE32PLUS_MAGIC && file_header->optional_header_size < sizeof(PE_OptionalHeader32Plus)) {
    rd_errorf("unexpected optional header size %m, expected at least %m", file_header->optional_header_size, sizeof(PE_OptionalHeader32Plus));
    goto exit;
  }
  
  U64                 sections_off = file_header_off + sizeof(*file_header) + file_header->optional_header_size;
  COFF_SectionHeader *sections     = str8_deserial_get_raw_ptr(raw_data, sections_off, sizeof(*sections)*file_header->section_count);
  if (!sections) {
    rd_errorf("not enough bytes to read COFF section headers");
    goto exit;
  }
  
  U64     string_table_off = file_header->symbol_table_foff + sizeof(COFF_Symbol16) * file_header->symbol_count;
  String8 raw_string_table = str8_substr(raw_data, rng_1u64(string_table_off, raw_data.size));
  
  COFF_Symbol32Array symbols = coff_symbol_array_from_data_16(scratch.arena, raw_data, file_header->symbol_table_foff, file_header->symbol_count);
  
  U8 *raw_opt_header = push_array(scratch.arena, U8, file_header->optional_header_size);
  str8_deserial_read_array(raw_data, opt_header_off, raw_opt_header, file_header->optional_header_size);
  
  if (opts & RD_Option_Headers) {
    coff_print_file_header(arena, out, indent, file_header);
    rd_newline();
  }
  
  Arch              arch       = arch_from_coff_machine(file_header->machine);
  U64               image_base = 0;
  U64               dir_count  = 0;
  PE_DataDirectory *dirs       = 0;
  
  if (opt_header_magic == PE_PE32_MAGIC) {
    PE_OptionalHeader32 *opt_header = (PE_OptionalHeader32 *)raw_opt_header;
    image_base = opt_header->image_base;
    dir_count  = opt_header->data_dir_count;
    dirs       = str8_deserial_get_raw_ptr(raw_data, opt_header_off+sizeof(*opt_header), sizeof(*dirs) * opt_header->data_dir_count);
    if (!dirs) {
      rd_errorf("unable to read data directories");
      goto exit;
    }
    
    if (opts & RD_Option_Headers) {
      pe_print_optional_header32(arena, out, indent, opt_header, dirs);
    }
  } else if (opt_header_magic == PE_PE32PLUS_MAGIC) {
    PE_OptionalHeader32Plus *opt_header = (PE_OptionalHeader32Plus *)raw_opt_header;
    image_base = opt_header->image_base;
    dir_count  = opt_header->data_dir_count;
    dirs       = str8_deserial_get_raw_ptr(raw_data, opt_header_off+sizeof(*opt_header), sizeof(*dirs) * opt_header->data_dir_count);
    if (!dirs) {
      rd_errorf("unable to read data directories");
      goto exit;
    }
    
    if (opts & RD_Option_Headers) {
      pe_print_optional_header32plus(arena, out, indent, opt_header, dirs);
    }
  }
  
  // map data directory RVA to file offsets
  Rng1U64 *dirs_file_ranges = push_array(scratch.arena, Rng1U64, dir_count);
  Rng1U64 *dirs_virt_ranges = push_array(scratch.arena, Rng1U64, dir_count);
  for (U64 i = 0; i < dir_count; ++i) {
    PE_DataDirectory dir = dirs[i];
    U64 file_off = coff_foff_from_voff(sections, file_header->section_count, dir.virt_off);
    dirs_file_ranges[i] = r1u64(file_off, file_off+dir.virt_size);
    dirs_virt_ranges[i] = r1u64(dir.virt_off, dir.virt_off+dir.virt_size);
  }
  
  if (opts & RD_Option_Sections) {
    coff_print_section_table(arena, out, indent, raw_string_table, symbols, file_header->section_count, sections);
  }
  
  if (opts & RD_Option_Relocs) {
    coff_print_relocs(arena, out, indent, raw_data, raw_string_table, file_header->machine, file_header->section_count, sections, symbols);
  }
  
  if (opts & RD_Option_Symbols) {
    coff_print_symbol_table(arena, out, indent, raw_data, 0, raw_string_table, symbols);
  }
  
  if (opts & RD_Option_Exports) {
    PE_ParsedExportTable exptab = pe_exports_from_data(arena,
                                                       file_header->section_count,
                                                       sections,
                                                       raw_data,
                                                       dirs_file_ranges[PE_DataDirectoryIndex_EXPORT],
                                                       dirs_virt_ranges[PE_DataDirectoryIndex_EXPORT]);
    pe_print_export_table(arena, out, indent, exptab);
  }
  
  if (opts & RD_Option_Imports) {
    B32                        is_pe32       = opt_header_magic == PE_PE32_MAGIC;
    PE_ParsedStaticImportTable static_imptab = pe_static_imports_from_data(arena, is_pe32, file_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_IMPORT]);
    PE_ParsedDelayImportTable  delay_imptab  = pe_delay_imports_from_data(arena, is_pe32, file_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_DELAY_IMPORT]);
    pe_print_static_import_table(arena, out, indent, image_base, static_imptab);
    pe_print_delay_import_table(arena, out, indent, image_base, delay_imptab);
  }
  
  if (opts & RD_Option_Resources) {
    String8         raw_dir  = str8_substr(raw_data, dirs_file_ranges[PE_DataDirectoryIndex_RESOURCES]);
    PE_ResourceDir *dir_root = pe_resource_table_from_directory_data(scratch.arena, raw_dir);
    pe_print_resources(arena, out, indent, dir_root);
  }
  
  if (opts & RD_Option_Exceptions) {
    pe_print_exceptions(arena, out, indent, file_header->machine, file_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_EXCEPTIONS], rdi);
  }
  
  if (opts & RD_Option_Relocs) {
    pe_print_base_relocs(arena, out, indent, file_header->machine, image_base, file_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_BASE_RELOC], rdi);
  }
  
  if (opts & RD_Option_Debug) {
    if (PE_DataDirectoryIndex_DEBUG < dir_count) {
      String8 raw_dir = str8_substr(raw_data, dirs_file_ranges[PE_DataDirectoryIndex_DEBUG]);
      pe_print_debug_diretory(arena, out, indent, raw_data, raw_dir);
    }
  }
  
  if (opts & RD_Option_Tls) {
    if (dim_1u64(dirs_file_ranges[PE_DataDirectoryIndex_TLS])) {
      PE_ParsedTLS tls = pe_tls_from_data(scratch.arena, file_header->machine, image_base, file_header->section_count, sections, raw_data, dirs_file_ranges[PE_DataDirectoryIndex_TLS]);
      pe_print_tls(arena, out, indent, tls);
    }
  }
  
  if (opts & RD_Option_LoadConfig) {
    String8 raw_lc = str8_substr(raw_data, dirs_file_ranges[PE_DataDirectoryIndex_LOAD_CONFIG]);
    if (raw_lc.size) {
      switch (file_header->machine) {
        case COFF_MachineType_Unknown: break;
        case COFF_MachineType_X86: {
          PE_LoadConfig32 *lc = str8_deserial_get_raw_ptr(raw_lc, 0, sizeof(*lc));
          if (lc) {
            pe_print_load_config32(arena, out, indent, lc);
          } else {
            rd_errorf("not enough bytes to parse 32bit load config");
          }
        } break;
        case COFF_MachineType_X64: {
          PE_LoadConfig64 *lc = str8_deserial_get_raw_ptr(raw_lc, 0, sizeof(*lc));
          if (lc) {
            pe_print_load_config64(arena, out, indent, lc);
          } else {
            rd_errorf("not enough bytes to parse 64bit load config");
          }
        } break;
        default: NotImplemented;
      }
    }
  }
  
  RD_MarkerArray *section_markers = 0;
  if (opts & (RD_Option_Disasm|RD_Option_Rawdata)) {
    if (rdi) {
      section_markers = rd_section_markers_from_rdi(scratch.arena, rdi);
    } else {
      section_markers = rd_section_markers_from_coff_symbol_table(scratch.arena, raw_string_table, file_header->section_count, symbols);
    }
  }
  
  if (opts & RD_Option_Rawdata) {
    coff_raw_data_sections(arena, out, indent, raw_data, 0, section_markers, file_header->section_count, sections);
  }
  
  if (opts & RD_Option_Disasm) {
    coff_disasm_sections(arena, out, indent, raw_data, file_header->machine, 0, 1, section_markers, file_header->section_count, sections);
  }
  
  if (opts & RD_Option_Dwarf) {
    DW_Input dwarf_input = dw_input_from_coff_section_table(scratch.arena, raw_data, raw_string_table, file_header->section_count, sections);
    dw_format(arena, out, indent, opts, &dwarf_input, arch, Image_CoffPe);
  }
  
  exit:;
  scratch_end(scratch);
}

#if 0
internal void
elf_print_dwarf_expressions(Arena *arena, String8List *out, String8 indent, String8 raw_data)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  ELF_BinInfo      bin         = elf_bin_from_data(raw_data);
  Arch             arch        = arch_from_elf_machine(bin.hdr.e_machine);
  DW_Input         dwarf_input = dw_input_from_elf_section_table(scratch.arena, raw_data, &bin);
  ELF_Class        elf_class   = bin.hdr.e_ident[ELF_Identifier_Class];
  ImageType        image_type  = elf_class == ELF_Class_32 ? Image_Elf32 : elf_class == ELF_Class_64 ? Image_Elf64 : ELF_Class_None;
  B32              relaxed     = 1;
  Rng1U64List      cu_ranges   = dw_unit_ranges_from_data(scratch.arena, dwarf_input.sec[DW_Section_Info].data);
  DW_ListUnitInput lu_input    = dw_list_unit_input_from_input(scratch.arena, &dwarf_input);
  
  if (bin.hdr.e_type == ELF_Type_Exec || bin.hdr.e_type == ELF_Type_Dyn) {
    U64 cu_idx = 0;
    for (Rng1U64Node *cu_range_n = cu_ranges.first; cu_range_n != 0; cu_range_n = cu_range_n->next, ++cu_idx) {
      Temp comp_temp = temp_begin(scratch.arena);
      
      U64         cu_base   = cu_range_n->v.min;
      Rng1U64     cu_range  = cu_range_n->v;
      DW_CompUnit cu        = dw_cu_from_info_off(comp_temp.arena, &dwarf_input, lu_input, cu_range.min, relaxed);
      
      struct TagNode {
        struct TagNode *next;
        DW_Tag          tag;
      };
      struct TagNode *tag_stack = 0;
      struct TagNode *free_tags = 0;
      
      S32 lexical_block_depth = 0;
      
      for (U64 info_off = cu.first_tag_info_off, tag_size = 0; info_off < cu.info_range.max; info_off += tag_size) {
        DW_Tag tag = {0};
        tag_size = dw_read_tag_cu(comp_temp.arena, &dwarf_input, &cu, info_off, &tag);
        if (tag.has_children) {
          struct TagNode *n = free_tags;
          if (n == 0) {
            n = push_array(comp_temp.arena, struct TagNode, 1);
          } else {
            SLLStackPop(free_tags);
          }
          n->tag = tag;
          SLLStackPush(tag_stack, n);
        }
        
        if (tag.kind == DW_Tag_Null) {
          if (tag_stack) {
            struct TagNode *n = tag_stack;
            if ((n->tag.kind == DW_Tag_SubProgram || n->tag.kind == DW_Tag_LexicalBlock)) {
              Assert(lexical_block_depth > 0);
              --lexical_block_depth;
            }
            SLLStackPop(tag_stack);
            SLLStackPush(free_tags, n);
          }
        } else if (tag.kind == DW_Tag_LexicalBlock || tag.kind == DW_Tag_SubProgram) {
          ++lexical_block_depth;
          if (tag.kind == DW_Tag_SubProgram) {
            String8 expr = dw_exprloc_from_attrib(&dwarf_input, &cu, tag, DW_Attrib_FrameBase);
            if (expr.size > 0) {
              String8 expr_str = dw_format_expression_single_line(comp_temp.arena, expr, cu_base, cu.address_size, arch, cu.version, cu.ext, cu.format);
            }
          }
        } else if (tag.kind == DW_Tag_Variable || tag.kind == DW_Tag_FormalParameter) {
#if 0
          String8         name            = dw_string_from_attrib(&dwarf_input, &cu, tag, DW_Attrib_Name);
          DW_Attrib      *location_attrib = dw_attrib_from_tag(&dwarf_input, &cu, tag, DW_Attrib_Location);
          DW_AttribClass  value_class     = dw_value_class_from_attrib(&cu, location_attrib);
          if (value_class != DW_AttribClass_Null) {
            if (lexical_block_depth == 0) {
              rd_printf("%llx Global: %S", info_off, name);
              is_global_var = 1;
            } else {
              rd_printf("%llx Local: %S", info_off, name);
              is_global_var = 0;
            }
            
            rd_indent();
            if (value_class == DW_AttribClass_LocListPtr || value_class == DW_AttribClass_LocList) {
              DW_LocList location = dw_loclist_from_attrib_ptr(comp_temp.arena, &dwarf_input, &cu, location_attrib);
              for (DW_LocNode *loc_n = location.first; loc_n != 0; loc_n = loc_n->next) {
                String8 expr_str = dw_format_expression_single_line(comp_temp.arena, loc_n->v.expr, cu_base, cu.address_size, arch, cu.version, cu.ext, cu.format);
                rd_printf("[%llx-%llx] %S", loc_n->v.range.min, loc_n->v.range.max, expr_str);
              }
            } else if (value_class == DW_AttribClass_ExprLoc) {
              String8 expr = dw_exprloc_from_attrib_ptr(&dwarf_input, &cu, location_attrib);
              String8 expr_str = dw_format_expression_single_line(comp_temp.arena, expr, cu_base, cu.address_size, arch, cu.version, cu.ext, cu.format);
              rd_printf("%S", expr_str);
            }
            rd_unindent();
          }
#endif
        }
        
#if 0
        if (tag.kind == DW_Tag_LexicalBlock || tag.kind == DW_Tag_CompileUnit || tag.kind == DW_Tag_InlinedSubroutine || tag.kind == DW_Tag_SubProgram) {
          Temp temp = temp_begin(comp_temp.arena);
          DW_Attrib *ranges_attrib = dw_attrib_from_tag(&dwarf_input, &cu, tag, DW_Attrib_Ranges);
          if (ranges_attrib->attrib_kind == DW_Attrib_Ranges) {
            Rng1U64List ranges = dw_rnglist_from_attrib_ptr(temp.arena, &dwarf_input, &cu, ranges_attrib);
          }
          temp_end(temp);
        }
#endif
      }
      
      temp_end(comp_temp);
    }
  } else {
    fprintf(stderr, "Skipping unexpected ELF type %u\n", bin.hdr.e_type);
  }
  
  scratch_end(scratch);
}
#endif


