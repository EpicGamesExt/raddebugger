// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#define T_Group "d2r"

internal RDI_Parsed *
d2r_rdi_from_dwarf_writer(Arena *arena, DW_Writer *writer)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8 raw_coff;
  {
    OBJ *obj = obj_alloc(0, Arch_x64);
    OBJ_Section *text_section = obj_push_section(obj, str8_lit(".text"), OBJ_SectionFlag_Read|OBJ_SectionFlag_Exec|OBJ_SectionFlag_Load);
    str8_serial_push_u8(obj->arena, &text_section->data, 0xc3);
    obj_push_symbol(obj, str8_lit("main"), OBJ_SymbolScope_Global, OBJ_RefKind_Section, text_section);
    dw_writer_emit_to_obj(writer, obj);
    raw_coff = coff_from_obj(scratch.arena, obj);
    obj_release(&obj);
  }

  Assert(t_write_file(str8_lit("a.obj"), raw_coff));
  t_invoke(str8_lit("radlink.exe"), str8_lit("/subsystem:console /out:a.exe /entry:main /DEBUG:FULL /opt:noref /opt:noicf a.obj"), max_U64);
  Assert(g_last_exit_code == 0);
  String8 exe = t_read_file(scratch.arena, str8_lit("a.exe"));
  Assert(exe.size > 0);

  t_invoke(str8_lit("radbin.exe"), str8_lit("-rdi a.exe"), max_U64);
  Assert(g_last_exit_code == 0);

  String8 raw_rdi = t_read_file(scratch.arena, str8_lit("a.rdi"));
  Assert(raw_rdi.size > 0);

  RDI_Parsed *rdi = push_array(arena, RDI_Parsed, 1);
  RDI_ParseStatus rdi_parse_status = rdi_parse(raw_rdi.str, raw_rdi.size, rdi);
  Assert(rdi_parse_status == RDI_ParseStatus_Good);

  scratch_end(scratch);
  return rdi;
}

T_BeginTest(d2r_general)
{
  DW_Writer *writer = dw_writer_begin(DW_Format_32Bit, DW_Version_5, DW_CompUnitKind_Compile, Arch_x64);
  dw_writer_tag_begin(writer, DW_TagKind_CompileUnit);
  dw_writer_push_attrib_stringf(writer, DW_AttribKind_Producer, "Test");
    // declare char type
    DW_WriterTag *char_type = dw_writer_tag_begin(writer, DW_TagKind_BaseType);
    dw_writer_push_attrib_sint(writer, DW_AttribKind_ByteSize, 1);
    dw_writer_push_attrib_enum(writer, DW_AttribKind_Encoding, DW_ATE_SignedChar);
    dw_writer_push_attrib_stringf(writer, DW_AttribKind_Name, "char");
    dw_writer_tag_end(writer);
    // declare function
    dw_writer_tag_begin(writer, DW_TagKind_SubProgram);
    dw_writer_push_attrib_address(writer, DW_AttribKind_LowPc, 0x140173f9);
    dw_writer_push_attrib_address(writer, DW_AttribKind_HighPc, 0x14017474b);
    dw_writer_push_attrib_flag(writer, DW_AttribKind_External, 1);
    dw_writer_push_attrib_flag(writer, DW_AttribKind_Prototyped, 1);
    dw_writer_push_attrib_stringf(writer, DW_AttribKind_Name, "FooBar");
      // declare variable
      dw_writer_tag_begin(writer, DW_TagKind_Variable);
      dw_writer_push_attrib_expression(writer, DW_AttribKind_Location, &(DW_ExprEnc)DW_ExprEnc_Op(Reg7), 1);
      dw_writer_push_attrib_stringf(writer, DW_AttribKind_Name, "TestLocal");
      dw_writer_push_attrib_ref(writer, DW_AttribKind_Type, char_type);
      dw_writer_tag_end(writer);
    dw_writer_tag_end(writer);

  RDI_Parsed *rdi = d2r_rdi_from_dwarf_writer(scratch.arena, writer);

  RDI_Procedure *proc_name = rdi_procedure_from_name_cstr(rdi, "FooBar");
  T_Ok(proc_name);

  dw_writer_end(&writer);
}
T_EndTest;

#undef T_Group
