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
  t_invoke(t_radlink_path(), str8_lit("/subsystem:console /out:a.exe /entry:main /DEBUG:FULL /opt:noref /opt:noicf a.obj"), max_U64);
  Assert(g_last_exit_code == 0);
  String8 exe = t_read_file(scratch.arena, str8_lit("a.exe"));
  Assert(exe.size > 0);

  B32 was_pdb_deleted = os_delete_file_at_path(t_make_file_path(scratch.arena, str8_lit("a.pdb")));
  Assert(was_pdb_deleted);

  t_invoke_(t_radbin_path(), str8_lit("-rdi a.exe -out:a.rdi"), max_U64, 0, 0);
  Assert(g_last_exit_code == 0);

  String8 raw_rdi = t_read_file(arena, str8_lit("a.rdi"));
  Assert(raw_rdi.size > 0);

  RDI_Parsed *rdi = push_array(arena, RDI_Parsed, 1);
  RDI_ParseStatus rdi_parse_status = rdi_parse(raw_rdi.str, raw_rdi.size, rdi);
  Assert(rdi_parse_status == RDI_ParseStatus_Good);

  scratch_end(scratch);
  return rdi;
}

internal RDI_TypeNode *
d2rt_type_from_name(RDI_Parsed *rdi, RDI_ParsedNameMap *map, char *name)
{
  String8 s = str8_cstring(name);
  RDI_NameMapNode *node = rdi_name_map_lookup(rdi, map, s.str, s.size);

  U32 id_count = 0;
  U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);

  if (id_count == 1) {
    return rdi_element_from_name_idx(rdi, TypeNodes, ids[0]);
  }
  return 0;
}

T_BeginTest(d2r_types)
{
  DW_Writer *writer = dw_writer_begin(DW_Format_32Bit, DW_Version_5, DW_CompUnitKind_Compile, Arch_x64);
  {
    dw_writer_tag_begin(writer, DW_TagKind_CompileUnit);
    dw_writer_push_attrib_stringf(writer, DW_AttribKind_Producer, "Test");

#define DeclBaseType(tt, n, e, s)                                                  \
        DW_WriterTag *tt = dw_writer_tag_begin(writer, DW_TagKind_BaseType);       \
        dw_writer_push_attrib_sint(writer,    DW_AttribKind_ByteSize, s);          \
        dw_writer_push_attrib_enum(writer,    DW_AttribKind_Encoding, DW_ATE_##e); \
        dw_writer_push_attrib_stringf(writer, DW_AttribKind_Name,     n);          \
        dw_writer_tag_end(writer);
      DeclBaseType(char_type,                    "char",                   SignedChar,   1);
      DeclBaseType(unsigned_char_type,           "unsigned char",          UnsignedChar, 1);
      DeclBaseType(char8_type,                   "char8_t",                Utf,          1);
      DeclBaseType(char16_type,                  "char16_t",               Utf,          2);
      DeclBaseType(char32_type,                  "char32_t",               Utf,          4);
      DeclBaseType(wchar_type,                   "wchar_t",                Signed,       4);
      DeclBaseType(bool_type,                    "_Bool",                  Boolean,      1);
      DeclBaseType(short_type,                   "short",                  Signed,       2);
      DeclBaseType(unsigned_short_type,          "unsigned short",         Unsigned,     2);
      DeclBaseType(short_unsigned_int_type,      "short unsigned int",     Unsigned,     2);
      DeclBaseType(short_int_type,               "short int",              Signed,       2);
      DeclBaseType(unsigned_int_type,            "unsigned int",           Unsigned,     4);
      DeclBaseType(int_type,                     "int",                    Signed,       4);
      DeclBaseType(long_int_type,                "long int",               Signed,       8);
      DeclBaseType(long_unsigned_int_type,       "long unsigned int",      Unsigned,     8);
      DeclBaseType(long_long_int_type,           "long long int",          Signed,       8);
      DeclBaseType(long_long_unsigned_int,       "long long unsigned int", Unsigned,     8);
      DeclBaseType(float_type,                   "float",                  Float,        4);
      DeclBaseType(double_type,                  "double",                 Float,        8);
      DeclBaseType(long_double_type,             "long double",            Float,        16);
      DeclBaseType(int128_type,                  "__int128",               Signed,       16);
      DeclBaseType(uint128_type,                 "__int128 unsigned",      Unsigned,     16);
      DeclBaseType(float16_type,                 "_Float16",               Float,        2);
      DeclBaseType(bfloat16_type,                "__bf16",                 Float,        2);
      DeclBaseType(float80_type,                 "__float80",              Float,        16);
      DeclBaseType(float128_type,                "_float128",              Float,        16);
      DeclBaseType(complex_float_type,           "complex float",          ComplexFloat, 8);
      DeclBaseType(complex_doulbe_type,          "complex double",         ComplexFloat, 16);
      DeclBaseType(complex_long_double_type,     "complex long double",    ComplexFloat, 32);
      DeclBaseType(decimal32_type,               "_Decimal32",             DecimalFloat, 4);
      DeclBaseType(decimal64_type,               "_Decimal64",             DecimalFloat, 8);
      DeclBaseType(decimal128_type,              "_Decimal128",            DecimalFloat, 16);
#undef DeclBaseType

#define DeclStdint(n, a)                                                \
        do {                                                            \
          dw_writer_tag_begin(writer, DW_TagKind_Typedef);              \
          dw_writer_push_attrib_stringf(writer, DW_AttribKind_Name, n); \
          dw_writer_push_attrib_ref(writer,     DW_AttribKind_Type, a); \
          dw_writer_tag_end(writer);                                    \
        } while (0)
      DeclStdint("uint8_t",  unsigned_char_type);
      DeclStdint("uint16_t", unsigned_short_type);
      DeclStdint("uint32_t", unsigned_int_type);
      DeclStdint("uint64_t", long_unsigned_int_type);
      DeclStdint("int8_t",   char_type);
      DeclStdint("int16_t",  short_type);
      DeclStdint("int32_t",  int_type);
      DeclStdint("int64_t",  long_int_type);
#undef DeclStdInt
      // TODO: @native_vector_support
      //
      // typedef int          __attribute__((vector_size(32))) int256
      // typedef unsigned int __attribute__((vector_size(32))) uint256
      // typedef int          __attribute__((vector_size(64))) int512
      // typedef unsigned int __attribute__((vector_size(64))) uint512
      //
#if 0
      dw_writer_tag_begin(writer, DW_TagKind_ArrayType);
        dw_writer_push_attrib_flag(writer, DW_AttribKind_GNU_Vector, 1);
        dw_writer_push_attrib_ref(writer,  DW_AttribKind_Type,       int_type);
        dw_writer_tag_begin(writer, DW_TagKind_SubrangeType);
          dw_writer_push_attrib_uint(writer, DW_AttribKind_UpperBound, 15);
        dw_writer_tag_end(writer);
      dw_writer_tag_end(writer);
#endif

    dw_writer_tag_end(writer);
  }

  RDI_Parsed  *rdi      = d2r_rdi_from_dwarf_writer(scratch.arena, writer);
  RDI_NameMap *types_nm = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Types);
  T_Ok(types_nm);

  RDI_ParsedNameMap types_map = {0};
  rdi_parsed_from_name_map(rdi, types_nm, &types_map);

#define TestBuiltinType(n, bs, r)                                                                                 \
    do {                                                                                                          \
      RDI_TypeNode *alias = d2rt_type_from_name(rdi, &types_map, n);                                              \
      T_Ok(alias);                                                                                                \
      T_Ok(alias->kind == RDI_TypeKind_Alias);                                                                    \
      T_Ok(alias->flags == 0);                                                                                    \
      T_Ok(alias->byte_size == bs);                                                                               \
      RDI_TypeNode *type = rdi_element_from_name_idx(rdi, TypeNodes, alias->user_defined.direct_type_idx);        \
      T_Ok(type);                                                                                                 \
      T_Ok(type->kind == RDI_TypeKind_##r);                                                                       \
      T_Ok(type->flags == 0);                                                                                     \
      T_Ok(type->byte_size == alias->byte_size);                                                                  \
      T_Ok(str8_match(str8_from_rdi_string_idx(rdi, type->built_in.name_string_idx), str8_lit(Stringify(r)), 0)); \
    } while (0)
  TestBuiltinType("char",                1,  Char8);
  TestBuiltinType("char8_t",             1,  UChar8);
  TestBuiltinType("char16_t",            2,  UChar16);
  TestBuiltinType("char32_t",            4,  UChar32);
  TestBuiltinType("unsigned char",       1,  UChar8);
  TestBuiltinType("wchar_t",             4,  Char32);
  TestBuiltinType("_Bool",               1,  Bool);
  TestBuiltinType("short",               2,  S16);
  TestBuiltinType("unsigned short",      2,  U16);
  TestBuiltinType("short unsigned int",  2,  U16);
  TestBuiltinType("short int",           2,  S16);
  TestBuiltinType("unsigned int",        4,  U32);
  TestBuiltinType("int",                 4,  S32);
  TestBuiltinType("long int",            8,  S64);
  TestBuiltinType("long unsigned int",   8,  U64);
  TestBuiltinType("long long int",       8,  S64);
  TestBuiltinType("float",               4,  F32);
  TestBuiltinType("double",              8,  F64);
  TestBuiltinType("long double",         16, F128);
  TestBuiltinType("__int128",            16, S128);
  TestBuiltinType("__int128 unsigned",   16, U128);
  TestBuiltinType("_Float16",            2,  F16);
  TestBuiltinType("__bf16",              2,  BF16);
  TestBuiltinType("__float80",           16, F80);
  TestBuiltinType("_float128",           16, F128);
  TestBuiltinType("complex float",       8,  ComplexF32);
  TestBuiltinType("complex double",      16, ComplexF64);
  TestBuiltinType("complex long double", 32, ComplexF128);
  TestBuiltinType("_Decimal32",          4,  Decimal32);
  TestBuiltinType("_Decimal64",          8,  Decimal64);
  TestBuiltinType("_Decimal128",         16, Decimal128);
#undef TestBuiltinType

#define TestStdint(n, s, t)                                                                             \
    do {                                                                                                \
      RDI_TypeNode *td = d2rt_type_from_name(rdi, &types_map, n);                                       \
      T_Ok(td);                                                                                         \
      T_Ok(td->kind == RDI_TypeKind_Alias);                                                             \
      T_Ok(td->flags == 0);                                                                             \
      T_Ok(td->byte_size == s);                                                                         \
      RDI_TypeNode *type = rdi_element_from_name_idx(rdi, TypeNodes, td->user_defined.direct_type_idx); \
      T_Ok(type);                                                                                       \
      T_Ok(type->kind == RDI_TypeKind_Alias);                                                           \
      T_Ok(type->flags == 0);                                                                           \
      T_Ok(type->byte_size = td->byte_size);                                                            \
      T_Ok(str8_match(str8_from_rdi_string_idx(rdi, type->built_in.name_string_idx), str8_lit(t), 0));  \
    } while (0)
  TestStdint("uint8_t",  1, "unsigned char");
  TestStdint("uint16_t", 2,  "unsigned short");
  TestStdint("uint32_t", 4,  "unsigned int");
  TestStdint("uint64_t", 8,  "long unsigned int");
  TestStdint("int8_t",   1,  "char");
  TestStdint("int16_t",  2,  "short");
  TestStdint("int32_t",  4,  "int");
  TestStdint("int64_t",  8,  "long int");
#undef TestStdint

  dw_writer_end(&writer);
}
T_EndTest;

#if 0
T_BeginTest(d2r_line_table)
{
  DW_Writer *writer = dw_writer_begin(DW_Format_32Bit, DW_Version_5, DW_CompUnitKind_Compile, Arch_x64);
  String8 comp_dir  = str8_lit("c:/devel/");
  String8 comp_name = str8_lit("test.c");

  DW_WriterFile *file = dw_writer_new_file(writer, comp_name);
  file->md5 = *(U128 *)&(U8[]){ 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc };

  struct {
    DW_WriterFile *file; U64 ln; U64 line_size; U64 voff;
  } test_table[] = {
    { file, 6, 1 },
    { file, 4, 2 },
    { file, 2, 3 },
    { file, 1, 4 },
    { file, 8, 5 },
  };

  U64 exe_base = coff_default_exe_base_from_machine(COFF_MachineType_X64);
  U64 voff     = 0x1000;
  for EachElement(i, test_table) {
    test_table[i].voff = voff;
    dw_writer_line_emit(writer, test_table[i].file, test_table[i].ln, 0, exe_base + voff);
    voff += test_table[i].line_size;
  }

  dw_writer_tag_begin(writer, DW_TagKind_CompileUnit);
    dw_writer_push_attrib_string(writer, DW_AttribKind_Producer, str8_lit("RAD DWARF WRITER"));
    dw_writer_push_attrib_string(writer, DW_AttribKind_CompDir, comp_dir);
    dw_writer_push_attrib_string(writer, DW_AttribKind_Name, comp_name);
    dw_writer_push_attrib_address(writer, DW_AttribKind_LowPc, exe_base);
    dw_writer_push_attrib_address(writer, DW_AttribKind_HighPc, exe_base + voff);
    dw_writer_push_attrib_line_ptr(writer, DW_AttribKind_StmtList, 0);
  dw_writer_tag_end(writer);

  d2r_rdi_from_dwarf_writer(scratch.arena, writer);

  for EachElement(i, test_table) {
    for EachIndex(k, test_table[i].line_size) {
      String8 cmd_line = str8f(scratch.arena, "-voff2line -voff:%llx, a.rdi", test_table[i].voff + k);
      String8 output = {0};
      t_invoke_(str8_lit("radbin.exe"), cmd_line, max_U64, scratch.arena, &output);
      T_Ok(g_last_exit_code == 0);
      T_MatchLinef(&output, "%S:/%llu", test_table[i].file->path, test_table[i].ln);
    }
  }

  dw_writer_end(&writer);
}
T_EndTest;
#endif

T_BeginTest(d2r_general)
{
  DW_Writer *writer = dw_writer_begin(DW_Format_32Bit, DW_Version_5, DW_CompUnitKind_Compile, Arch_x64);
  {
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
        dw_writer_push_attrib_expressionv(writer, DW_AttribKind_Location, DW_ExprEnc_Op(Reg7));
        dw_writer_push_attrib_stringf(writer, DW_AttribKind_Name, "TestLocal");
        dw_writer_push_attrib_ref(writer, DW_AttribKind_Type, char_type);
        dw_writer_tag_end(writer);
      dw_writer_tag_end(writer);
  }

  RDI_Parsed *rdi = d2r_rdi_from_dwarf_writer(scratch.arena, writer);

  RDI_Procedure *proc = rdi_procedure_from_name_cstr(rdi, "FooBar");
  T_Ok(proc);
  T_Ok(proc->link_flags == RDI_LinkFlag_External);
  String8 proc_name = str8_from_rdi_string_idx(rdi, proc->name_string_idx);
  T_Ok(str8_match(proc_name, str8_lit("FooBar"), 0));

  RDI_Scope *root_scope = rdi_root_scope_from_procedure(rdi, proc);
  T_Ok(root_scope);
  T_Ok(root_scope->local_count == 1);

  RDI_Local *test_local = rdi_element_from_name_idx(rdi, Locals, root_scope->local_first + 0);
  T_Ok(test_local);
  T_Ok(test_local->kind == RDI_LocalKind_Variable);
  String8 test_local_name = str8_from_rdi_string_idx(rdi, test_local->name_string_idx);
  T_Ok(str8_match(test_local_name, str8_lit("TestLocal"), 0));

  RDI_TypeNode *test_local_type = rdi_element_from_name_idx(rdi, TypeNodes, test_local->type_idx);
  T_Ok(test_local_type);
  T_Ok(test_local_type->kind == RDI_TypeKind_Alias);
  T_Ok(test_local_type->flags == 0);
  String8 alias_name = str8_from_rdi_string_idx(rdi, test_local_type->user_defined.name_string_idx);
  T_Ok(str8_match(alias_name, str8_lit("char"), 0));

  RDI_TypeNode *char_type = rdi_element_from_name_idx(rdi, TypeNodes, test_local_type->user_defined.direct_type_idx);
  T_Ok(char_type);
  T_Ok(char_type->kind == RDI_TypeKind_Char8);
  T_Ok(char_type->flags == 0);
  T_Ok(char_type->byte_size == 1);
  String8 char_type_name = str8_from_rdi_string_idx(rdi, char_type->built_in.name_string_idx);
  T_Ok(str8_match(char_type_name, str8_lit("Char8"), 0));

  dw_writer_end(&writer);
}
T_EndTest;

#undef T_Group
