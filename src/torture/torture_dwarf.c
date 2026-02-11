// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define T_Group "Dwarf"

internal U64
t_dw_test_uleb128(U64 v, U64 expected_length)
{
  Temp scratch = scratch_begin(0, 0);
  B32 is_ok = 0;

  U64 v0 = v;
  String8 e = dw_write_uleb128(scratch.arena, v0);
  if (!(expected_length == e.size)) { goto exit; }

  U64 v1;
  U64 bytes_read = str8_deserial_read_uleb128(e, 0, &v1);
  if (!(bytes_read == e.size)) { goto exit; }
  if (!(v0 == v1)) { goto exit; }

  is_ok = 1;
exit:;
  scratch_end(scratch);
  return is_ok;
}

internal U64
t_dw_test_sleb128(U64 v, U64 expected_length)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = 0;

  U64 v0 = v;
  String8 e = dw_write_sleb128(scratch.arena, v0);
  if (!(expected_length == e.size)) { goto exit; }

  U64 v1;
  U64 bytes_read = str8_deserial_read_sleb128(e, 0, &v1);
  if (!(bytes_read == e.size)) { goto exit; }
  if (!(v0 == v1)) { goto exit; }

  is_ok = 1;
exit:;
  scratch_end(scratch);
  return is_ok;
}

T_BeginTest(test_leb128)
{
  T_Ok(t_dw_test_uleb128(0, 1));
  T_Ok(t_dw_test_sleb128(0, 1));
  T_Ok(t_dw_test_sleb128(-1, 1));

  T_Ok(t_dw_test_uleb128(max_U64, 10));
  T_Ok(t_dw_test_sleb128(min_S64, 10));
  T_Ok(t_dw_test_sleb128(max_S64, 10));

  for EachIndex(i, 64) {
    T_Ok(t_dw_test_uleb128((1ull << i), 1 + (i / 7)));
  }
  for EachIndex(i, 64) {
    T_Ok(t_dw_test_sleb128((1ull << i), 1 + (i + 1) / 7));
  }
}
T_EndTest;

internal B32
dwt_tags_must_match(DW_WriterTag *writer_tag, DW_TagNode *reader_tag)
{
  B32 is_match = 0;

  // match headers
  B32 writer_has_children = writer_tag->first_child != 0;
  if (writer_tag->kind         != reader_tag->tag.kind)          { goto exit; }
  if (writer_tag->abbrev_id    != reader_tag->tag.abbrev_id)     { goto exit; }
  if (writer_tag->attrib_count != reader_tag->tag.attribs.count) { goto exit; }
  if (writer_tag->info_off     != reader_tag->tag.info_off)      { goto exit; }
  if (writer_has_children      != reader_tag->tag.has_children)  { goto exit; }

  // match attribs
  DW_WriterAttrib *writer_attrib = writer_tag->first_attrib;
  DW_AttribNode   *reader_attrib = reader_tag->tag.attribs.first;
  while (writer_attrib) {
    if (writer_attrib->kind != reader_attrib->v.attrib_kind)                 { goto exit; }
    if (writer_attrib->reader.form.kind != reader_attrib->v.form.kind)       { goto exit; }
    if ( ! dw_form_match(writer_attrib->reader.form, reader_attrib->v.form)) { goto exit; }
    writer_attrib = writer_attrib->next;
    reader_attrib = reader_attrib->next;
  }

  // visit children
  DW_WriterTag *writer_child = writer_tag->first_child;
  DW_TagNode   *reader_child = reader_tag->first_child;
  while (writer_child) {
    if ( ! dwt_tags_must_match(writer_child, reader_child)) { goto exit; }
    writer_child = writer_child->next;
    reader_child = reader_child->sibling;
  }

  is_match = 1;
exit:;
  return is_match;
}

internal DW_Writer *
dwt_make_writer(void)
{
  Temp scratch = scratch_begin(0, 0);

  // define debug info layout
  DW_Writer *writer = dw_writer_begin(DW_Format_32Bit, DW_Version_Last, DW_CompUnitKind_Compile, Arch_x64);
  {
    dw_writer_tag_begin(writer, DW_TagKind_CompileUnit);
    dw_writer_push_attrib_string(writer, DW_AttribKind_Producer, str8_lit(BUILD_TITLE_STRING_LITERAL));
    dw_writer_push_attrib_enum(writer, DW_AttribKind_Language, DW_Language_C99);
    dw_writer_push_attrib_flag(writer, DW_AttribKind_UseUtf8, 1);
    {
      DW_WriterTag *char_type = dw_writer_tag_begin(writer, DW_TagKind_BaseType);
      dw_writer_push_attrib_sint(writer, DW_AttribKind_ByteSize, 1);
      dw_writer_push_attrib_enum(writer, DW_AttribKind_Encoding, DW_ATE_SignedChar);
      dw_writer_push_attrib_string(writer, DW_AttribKind_Name, str8_lit("char"));
      dw_writer_tag_end(writer);

      dw_writer_tag_begin(writer, DW_TagKind_ConstType);
      dw_writer_push_attrib_ref(writer, DW_AttribKind_Type, char_type);
      dw_writer_tag_end(writer);

      // test abbrev dedup
      DW_WriterTag *dup_char_type = dw_writer_tag_begin(writer, DW_TagKind_BaseType);
      dw_writer_push_attrib_sint(writer, DW_AttribKind_ByteSize, 1);
      dw_writer_push_attrib_enum(writer, DW_AttribKind_Encoding, DW_ATE_SignedChar);
      dw_writer_push_attrib_string(writer, DW_AttribKind_Name, str8_lit("char"));
      dw_writer_tag_end(writer);

      // simple struct test
      DW_WriterTag *simple_struct_tag = dw_writer_tag_begin(writer, DW_TagKind_StructureType);
      dw_writer_push_attrib_string(writer, DW_AttribKind_Name, str8_lit("FooBar"));
      dw_writer_push_attrib_uint(writer, DW_AttribKind_ByteSize, 90);
      {
        dw_writer_tag_begin(writer, DW_TagKind_Member);
        dw_writer_push_attrib_string(writer, DW_AttribKind_Name, str8_lit("m0"));
        dw_writer_push_attrib_ref(writer, DW_AttribKind_Type, char_type);
        dw_writer_push_attrib_sint(writer, DW_AttribKind_DataMemberLocation, 0);
        dw_writer_tag_end(writer);

        dw_writer_tag_begin(writer, DW_TagKind_Member);
        dw_writer_push_attrib_string(writer, DW_AttribKind_Name, str8_lit("m1"));
        dw_writer_push_attrib_ref(writer, DW_AttribKind_Type, char_type);
        dw_writer_push_attrib_sint(writer, DW_AttribKind_DataMemberLocation, 10);
        dw_writer_push_attrib_implicit(writer, DW_AttribKind_ByteSize, 123);
        dw_writer_tag_end(writer);
      }
      dw_writer_tag_end(writer);

      dw_writer_tag_begin(writer, DW_TagKind_SubProgram);
      dw_writer_push_attrib_flag(writer, DW_AttribKind_External, 1);
      dw_writer_push_attrib_flag(writer, DW_AttribKind_Prototyped, 1);
      dw_writer_push_attrib_address(writer, DW_AttribKind_LowPc, 0x14012f2f0);
      dw_writer_push_attrib_address(writer, DW_AttribKind_HighPc, 0x14012f405);
      dw_writer_push_attrib_string(writer, DW_AttribKind_Name, str8_lit("main"));
      dw_writer_push_attrib_ref(writer, DW_AttribKind_Type, simple_struct_tag);
      dw_writer_push_attrib_expression(writer, DW_AttribKind_FrameBase, &(DW_ExprEnc)DW_ExprEnc_Op(Reg7), 1);
      dw_writer_tag_end(writer);
    }
    dw_writer_tag_end(writer);
  }

  {
    DW_Input input;
    {
      OBJ *obj = obj_alloc(0, Arch_x64);
      OBJ_Section *text_section = obj_push_section(obj, str8_lit(".text"), OBJ_SectionFlag_Read|OBJ_SectionFlag_Exec|OBJ_SectionFlag_Load);
      str8_serial_push_u8(obj->arena, &text_section->data, 0xc3);
      obj_push_symbol(obj, str8_lit("entry"), OBJ_SymbolScope_Global, OBJ_RefKind_Section, text_section);

      dw_writer_emit_to_obj(writer, obj);

      String8              raw_coff        = coff_from_obj(scratch.arena, obj);
      COFF_FileHeaderInfo  obj_coff_header = coff_file_header_info_from_data(raw_coff);
      String8              raw_sections    = str8_substr(raw_coff, obj_coff_header.section_table_range);
      U64                  section_count   = raw_sections.size / sizeof(COFF_SectionHeader);
      COFF_SectionHeader  *section_table   = (COFF_SectionHeader *)raw_sections.str;
      String8              string_table    = str8_substr(raw_coff, obj_coff_header.string_table_range);
      input = dw_input_from_coff_section_table(scratch.arena, raw_coff, string_table, section_count, section_table);

      t_write_file(str8_lit("dwarf.obj"), raw_coff);

      obj_release(&obj);
    }

    DW_ListUnitInput lu_input = dw_list_unit_input_from_input(scratch.arena, &input);
    DW_CompUnit      cu       = dw_cu_from_info_off(scratch.arena, &input, lu_input, 0, 1);
    DW_TagTree       tag_tree = dw_tag_tree_from_cu(scratch.arena, &input, &cu);
    AssertAlways(dwt_tags_must_match(writer->root, tag_tree.root));

  }

  scratch_end(scratch);
  return writer;
}

T_BeginTest(dwarf_writer)
{
  DW_Writer *writer = dwt_make_writer();

  // validate the writer

  T_Ok(writer->current == 0);
  T_Ok(writer->format == DW_Format_32Bit);
  T_Ok(writer->cu_kind == DW_CompUnitKind_Compile);
  T_Ok(writer->version == DW_Version_5);
  T_Ok(writer->address_size == 8);
  T_Ok(writer->abbrev_base_info_off == 8);
  T_Ok(writer->fixups.count == 0);
  T_Ok(writer->abbrev_id_map->count == 7);

  DW_WriterTag *comp_unit_tag  = writer->root;
  {
    T_Ok(comp_unit_tag->kind == DW_TagKind_CompileUnit);
    T_Ok(comp_unit_tag->next == 0);
    T_Ok(comp_unit_tag->parent == 0);
    T_Ok(comp_unit_tag->attrib_count == 3);
    T_Ok(comp_unit_tag->abbrev_id == 1);
    T_Ok(comp_unit_tag->info_off == 0xc);

    DW_WriterAttrib *producer_attrib = comp_unit_tag->first_attrib;
    T_Ok(producer_attrib->kind == DW_AttribKind_Producer);
    T_Ok(producer_attrib->reader.form.kind == DW_Form_String);
    T_Ok(str8_match(producer_attrib->writer.form.string, str8_lit(BUILD_TITLE_STRING_LITERAL), 0));

    DW_WriterAttrib *language_attrib = producer_attrib->next;
    T_Ok(language_attrib->kind == DW_AttribKind_Language);
    T_Ok(language_attrib->reader.form.kind == DW_Form_Data1);
    T_Ok(language_attrib->reader.form.data.size == 1);
    T_Ok(*(U8 *)language_attrib->reader.form.data.str == DW_Language_C99);

    DW_WriterAttrib *use_utf8_attrib = language_attrib->next;
    T_Ok(use_utf8_attrib->kind == DW_AttribKind_UseUtf8);
    T_Ok(use_utf8_attrib->reader.form.kind == DW_Form_Flag);
    T_Ok(use_utf8_attrib->reader.form.flag == 1);
  }

  DW_WriterTag *char_type_tag = comp_unit_tag->first_child;
  {
    T_Ok(char_type_tag->kind == DW_TagKind_BaseType);
    T_Ok(char_type_tag->next != 0);
    T_Ok(char_type_tag->parent == comp_unit_tag);
    T_Ok(char_type_tag->attrib_count == 3);
    T_Ok(char_type_tag->abbrev_id == 2);
    T_Ok(char_type_tag->info_off == 0x4d);
    T_Ok(char_type_tag->first_attrib != char_type_tag->last_attrib);

    DW_WriterAttrib *byte_size_attrib = char_type_tag->first_attrib;
    T_Ok(byte_size_attrib->kind == DW_AttribKind_ByteSize);
    T_Ok(byte_size_attrib->reader.form.kind == DW_Form_Data1);
    T_Ok(byte_size_attrib->reader.form.data.size == 1);
    T_Ok(*(U8 *)byte_size_attrib->reader.form.data.str == 1);

    DW_WriterAttrib *encoding_attrib  = byte_size_attrib->next;
    T_Ok(encoding_attrib->kind == DW_AttribKind_Encoding);
    T_Ok(encoding_attrib->reader.form.kind == DW_Form_Data1);
    T_Ok(encoding_attrib->reader.form.data.size == 1);
    T_Ok(*(U8 *)encoding_attrib->reader.form.data.str == DW_ATE_SignedChar);

    DW_WriterAttrib *name_attrib = encoding_attrib->next;
    T_Ok(name_attrib->kind == DW_AttribKind_Name);
    T_Ok(name_attrib->reader.form.kind == DW_Form_String);
    T_Ok(str8_match(name_attrib->reader.form.string, str8_lit("char"), 0));
  }

  DW_WriterTag *const_type_tag = char_type_tag->next;
  {
    T_Ok(const_type_tag->kind == DW_TagKind_ConstType);
    T_Ok(const_type_tag->next != 0);
    T_Ok(const_type_tag->parent == comp_unit_tag);
    T_Ok(const_type_tag->attrib_count == 1);
    T_Ok(const_type_tag->abbrev_id == 3);
    T_Ok(const_type_tag->info_off == 0x55);
    T_Ok(const_type_tag->first_attrib && const_type_tag->first_attrib == const_type_tag->last_attrib);

    DW_WriterAttrib *type_attrib = const_type_tag->first_attrib;
    T_Ok(type_attrib->kind == DW_AttribKind_Type);
    T_Ok(type_attrib->writer.form.kind == DW_WriterFormKind_Ref);
    T_Ok(type_attrib->writer.form.ref == char_type_tag);
  }

  DW_WriterTag *dup_char_type_tag = const_type_tag->next;
  {
    T_Ok(dup_char_type_tag->kind == DW_TagKind_BaseType);
    T_Ok(dup_char_type_tag->next != 0);
    T_Ok(dup_char_type_tag->parent == comp_unit_tag);
    T_Ok(dup_char_type_tag->attrib_count == 3);
    T_Ok(dup_char_type_tag->abbrev_id == 2);
    T_Ok(dup_char_type_tag->info_off == 0x57);
    T_Ok(dup_char_type_tag->first_attrib != dup_char_type_tag->last_attrib);

    DW_WriterAttrib *byte_size_attrib = dup_char_type_tag->first_attrib;
    T_Ok(byte_size_attrib->kind == DW_AttribKind_ByteSize);
    T_Ok(byte_size_attrib->reader.form.kind == DW_Form_Data1);
    T_Ok(byte_size_attrib->reader.form.data.size == 1);
    T_Ok(*(U8 *)byte_size_attrib->reader.form.data.str == 1);

    DW_WriterAttrib *encoding_attrib  = byte_size_attrib->next;
    T_Ok(encoding_attrib->kind == DW_AttribKind_Encoding);
    T_Ok(encoding_attrib->reader.form.kind == DW_Form_Data1);
    T_Ok(encoding_attrib->reader.form.data.size == 1);
    T_Ok(*(U8 *)encoding_attrib->reader.form.data.str == DW_ATE_SignedChar);

    DW_WriterAttrib *name_attrib = encoding_attrib->next;
    T_Ok(name_attrib->kind == DW_AttribKind_Name);
    T_Ok(name_attrib->reader.form.kind == DW_Form_String);
    T_Ok(str8_match(name_attrib->reader.form.string, str8_lit("char"), 0));
  }

  DW_WriterTag *simple_struct_tag = dup_char_type_tag->next;
  {
    T_Ok(simple_struct_tag->kind == DW_TagKind_StructureType);
    T_Ok(simple_struct_tag->next != 0);
    T_Ok(simple_struct_tag->parent == comp_unit_tag);
    T_Ok(simple_struct_tag->attrib_count == 2);
    T_Ok(simple_struct_tag->abbrev_id == 4);
    T_Ok(simple_struct_tag->info_off == 0x5f);

    DW_WriterAttrib *simple_struct_name = simple_struct_tag->first_attrib;
    T_Ok(simple_struct_name->kind == DW_AttribKind_Name);
    T_Ok(simple_struct_name->reader.form.kind == DW_Form_String);
    T_Ok(str8_match(simple_struct_name->reader.form.string, str8_lit("FooBar"), 0));

    DW_WriterTag *m0_tag = simple_struct_tag->first_child;
    {
      T_Ok(m0_tag->kind == DW_TagKind_Member);
      T_Ok(m0_tag->parent == simple_struct_tag);
      T_Ok(m0_tag->attrib_count == 3);
      T_Ok(m0_tag->info_off == 0x68);
      T_Ok(m0_tag->abbrev_id == 5);

      DW_WriterAttrib *name = m0_tag->first_attrib;
      T_Ok(name->kind == DW_AttribKind_Name);
      T_Ok(name->reader.form.kind == DW_Form_String);
      T_Ok(str8_match(name->reader.form.string, str8_lit("m0"), 0));

      DW_WriterAttrib *type = name->next;
      T_Ok(type->kind == DW_AttribKind_Type);
      T_Ok(type->reader.form.kind == DW_Form_Ref1);
      T_Ok(type->reader.form.ref == 0x4d);

      DW_WriterAttrib *data_loc = type->next;
      T_Ok(data_loc->kind == DW_AttribKind_DataMemberLocation);
      T_Ok(data_loc->reader.form.kind == DW_Form_Data1);
      T_Ok(data_loc->reader.form.data.size == 1);
      T_Ok(*(U8 *)data_loc->reader.form.data.str == 0);
    }

    DW_WriterTag *m1_tag = m0_tag->next;
    {
      T_Ok(m1_tag->kind == DW_TagKind_Member);
      T_Ok(m1_tag->parent == simple_struct_tag);
      T_Ok(m1_tag->attrib_count == 4);
      T_Ok(m1_tag->info_off == 0x6e);
      T_Ok(m1_tag->abbrev_id == 6);

      DW_WriterAttrib *name = m1_tag->first_attrib;
      T_Ok(name->kind == DW_AttribKind_Name);
      T_Ok(name->reader.form.kind == DW_Form_String);
      T_Ok(str8_match(name->reader.form.string, str8_lit("m1"), 0));

      DW_WriterAttrib *type = name->next;
      T_Ok(type->kind == DW_AttribKind_Type);
      T_Ok(type->reader.form.kind == DW_Form_Ref1);
      T_Ok(type->reader.form.ref == 0x4d);

      DW_WriterAttrib *data_loc = type->next;
      T_Ok(data_loc->kind == DW_AttribKind_DataMemberLocation);
      T_Ok(data_loc->reader.form.kind == DW_Form_Data1);
      T_Ok(data_loc->reader.form.data.size == 1);
      T_Ok(*(U8 *)data_loc->reader.form.data.str == 10);

      DW_WriterAttrib *byte_size = data_loc->next;
      T_Ok(byte_size->kind == DW_AttribKind_ByteSize);
      T_Ok(byte_size->reader.form.kind == DW_Form_ImplicitConst);
      T_Ok(byte_size->reader.form.implicit_const == 123);
    }
  }

  DW_WriterTag *main_tag = simple_struct_tag->next;
  T_Ok(main_tag->next == 0);
  T_Ok(main_tag->kind == DW_TagKind_SubProgram);
  T_Ok(main_tag->parent == main_tag->parent);
  T_Ok(main_tag->abbrev_id == 7);
  {
    DW_WriterAttrib *external = main_tag->first_attrib;
    T_Ok(external->kind == DW_AttribKind_External);
    T_Ok(external->reader.form.kind == DW_Form_Flag);
    T_Ok(external->reader.form.flag);

    DW_WriterAttrib *prototyped = external->next;
    T_Ok(prototyped->kind == DW_AttribKind_Prototyped);
    T_Ok(prototyped->reader.form.kind == DW_Form_Flag);
    T_Ok(prototyped->reader.form.flag);

    DW_WriterAttrib *low_pc = prototyped->next;
    T_Ok(low_pc->kind == DW_AttribKind_LowPc);
    T_Ok(low_pc->reader.form.kind == DW_Form_Addr);
    T_Ok(low_pc->reader.form.addr.size == sizeof(U64));
    T_Ok(*(U64 *)low_pc->reader.form.addr.str == 0x14012f2f0);

    DW_WriterAttrib *high_pc = low_pc->next;
    T_Ok(high_pc->kind == DW_AttribKind_HighPc);
    T_Ok(high_pc->reader.form.kind == DW_Form_Addr);
    T_Ok(high_pc->reader.form.addr.size == sizeof(U64));
    T_Ok(*(U64 *)high_pc->reader.form.addr.str == 0x14012f405);

    DW_WriterAttrib *name = high_pc->next;
    T_Ok(name->kind == DW_AttribKind_Name);
    T_Ok(name->reader.form.kind == DW_Form_String);
    T_Ok(str8_match(name->reader.form.string, str8_lit("main"), 0));

    DW_WriterAttrib *type = name->next;
    T_Ok(type->kind == DW_AttribKind_Type);
    T_Ok(type->reader.form.kind == DW_Form_Ref1);
    T_Ok(type->reader.form.ref == simple_struct_tag->info_off);

    DW_WriterAttrib *frame_base = type->next;
    T_Ok(frame_base->kind == DW_AttribKind_FrameBase);
    T_Ok(frame_base->reader.form.kind == DW_Form_ExprLoc);
    DW_Expr frame_base_expr = dw_expr_from_data(scratch.arena, writer->format, writer->address_size, frame_base->reader.form.exprloc);
    T_Ok(frame_base_expr.count == 1);
    T_Ok(frame_base_expr.first->opcode == DW_ExprOp_Reg7);
    T_Ok(frame_base_expr.first->operands == 0);
    T_Ok(frame_base_expr.first->size == 1);
    T_Ok(frame_base_expr.first->next == 0);
    T_Ok(frame_base_expr.first->prev == 0);
  }

  dw_writer_end(&writer);
}
T_EndTest;

T_BeginTest(value_in_register)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_Reg3 - DW_ExprOp_Reg0);
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 value = 0xc0ffee;
  MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));

  // compile a simple program which reads the value from register 3
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Reg3) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);
  
  // evaluate the program
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_U64);
  T_Ok(expr_value.u64 == value);
}
T_EndTest;

T_BeginTest(value_in_x_register)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, DW_RegX64_FsBase);
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 value = 0xc0ffee;
  MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));

  // compile a simple program which reads the value from register 3
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(RegX), DW_ExprEnc_ULEB128(DW_RegX64_FsBase) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  // evaluate the program
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_U64);
  T_Ok(expr_value.u64 == value);
}
T_EndTest;

T_BeginTest(address_of_value)
{
  // compile a simple program which reads address
  U64 addr = 0xdeadbeef;
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Addr), DW_ExprEnc_U64(addr) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  // evaluate the program
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, 0, 0, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == addr);
}
T_EndTest;

T_BeginTest(register_relative_variable)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_BReg11 - DW_ExprOp_BReg0);
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 value = 1;
  MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));

  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(BReg11), DW_ExprEnc_SLEB128(44) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);

  // validate eval result
  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == (1 + 44));
}
T_EndTest;

T_BeginTest(frame_relative_variable)
{
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(FBReg), DW_ExprEnc_SLEB128(-50) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  U64               frame_base = 123;
  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, frame_base, 0, 0, max_U64, expr, 0, 0, 0, 0, &expr_value);

  T_Ok(expr_eval == DW_ExprEvalResult_Ok);
  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == frame_base -50);
}
T_EndTest;

internal
MACHINE_OP_MEM_READ(t_machine_op_mem_read)
{
  MemoryCopy(buffer, PtrFromInt(addr), buffer_size);
  return MachineOpResult_Ok;
}

T_BeginTest(call_by_reference)
{
  U8 *memory = push_array(scratch.arena, U8, 128);
  U64 value = 0xc0ffee;
  MemoryCopy(memory + 32, &value, sizeof(value));

  REGS_RegBlockX64 regs      = {0};
  REGS_RegCode     reg_code  = reg_code_from_dw_reg(Arch_x64, 58); // fsbase
  Rng1U64          reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
  U64 memory_ptr = IntFromPtr(memory);
  MemoryCopy((U8 *)&regs + reg_range.min, &memory_ptr, sizeof(memory_ptr));

  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(BRegX), DW_ExprEnc_ULEB128(58), DW_ExprEnc_SLEB128(32), DW_ExprEnc_Op(Deref) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value = { 0 };
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, t_machine_op_mem_read, 0, &expr_value);

  T_Ok(expr_value.type == DW_ExprValueType_Generic);
  T_Ok(expr_value.generic.size == sizeof(U64));
  T_Ok(*(U64 *)expr_value.generic.str == value);
}
T_EndTest;

T_BeginTest(plus_uconst)
{
  U64 struct_addr = 0x123;
  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Addr), DW_ExprEnc_Addr(struct_addr), DW_ExprEnc_Op(PlusUConst), DW_ExprEnc_ULEB128(4) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, 0, 0, t_machine_op_mem_read, 0, &expr_value);

  T_Ok(expr_value.type == DW_ExprValueType_Addr);
  T_Ok(expr_value.addr == 0x123 + 4);
}
T_EndTest;

#if 0
T_BeginTest(reg_split_spill)
{
  // setup register context
  REGS_RegBlockX64 regs      = {0};
  {
    REGS_RegCode reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_BReg3 - DW_ExprOp_BReg0);
    Rng1U64      reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
    U64          value     = 0xaaaa;
    MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));
  }
  {
    REGS_RegCode reg_code  = reg_code_from_dw_reg(Arch_x64, DW_ExprOp_BReg10 - DW_ExprOp_BReg0);
    Rng1U64      reg_range = regs_range_from_code(Arch_x64, 0, reg_code);
    U64          value     = 0xbbbb;
    MemoryCopy((U8 *)&regs + reg_range.min, &value, sizeof(value));
  }

  DW_ExprEnc expr_encs[] = { DW_ExprEnc_Op(Reg3), DW_ExprEnc_Op(Piece), DW_ExprEnc_ULEB128(4), DW_ExprEnc_Op(Reg10), DW_ExprEnc_Op(Piece), DW_ExprEnc_ULEB128(2) };
  String8    expr_data   = dw_encode_expr(scratch.arena, Arch_x64, DW_Format_64Bit, expr_encs, ArrayCount(expr_encs));
  DW_Expr    expr        = dw_expr_from_data(scratch.arena, DW_Format_64Bit, byte_size_from_arch(Arch_x64), expr_data);

  DW_ExprValue      expr_value;
  DW_ExprEvalResult expr_eval = dw_eval_expr(scratch.arena, Arch_x64, DW_Format_64Bit, 0, 0, 0, max_U64, expr, regs_read_dwarf_x64, &regs, 0, 0, &expr_value);
}
T_EndTest;
#endif

#undef T_Group

