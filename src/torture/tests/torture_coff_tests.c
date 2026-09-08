// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
t_coff_test_encode(Arena *arena, TestCtx *ctx, String8 file_name, String8 source, T_Result *result_out)
{
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, file_name, source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  T_Artifact *value = t_artifact_from_name(&script, str8_lit("value"));
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  *result_out = result;
  return value != 0 ? value->data : str8_zero();
}

TEST(coff_writer_bigobj)
{
  // Exercise the standard-COFF boundary and an associative parent above 16 bits.
  U32 section_counts[] = {3, 0xfeff, 0xff00, 0x10002};
  for EachElement(case_idx, section_counts) {
    U32 section_count = section_counts[case_idx];
    COFF_ObjWriter *writer = coff_obj_writer_alloc(0x12345678, COFF_MachineType_X64);
    for (U32 i = 0; i < section_count - 2; i += 1) {
      coff_obj_writer_push_section(writer, str8_lit(".empty"), COFF_SectionFlag_LnkRemove, str8_zero());
    }
    COFF_SectionFlags flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_LnkCOMDAT;
    COFF_ObjSection *head = coff_obj_writer_push_section(writer, str8_lit(".long_head_section"), flags, str8_lit("head"));
    COFF_ObjSection *assoc = coff_obj_writer_push_section(writer, str8_lit(".assoc"), flags, str8(push_array(arena, U8, 8), 8));
    coff_obj_writer_push_symbol_secdef(writer, head, COFF_ComdatSelect_Any);
    COFF_ObjSymbol *target = coff_obj_writer_push_symbol_extern(writer, str8_lit("long_target_symbol"), 0, head);
    COFF_ObjSymbol *weak = coff_obj_writer_push_symbol_weak(writer, str8_lit("weak"), COFF_WeakExt_SearchAlias, target);
    COFF_ObjSymbol *absolute = coff_obj_writer_push_symbol_abs(writer, str8_lit("absolute"), 17, COFF_SymStorageClass_External);
    COFF_ObjSymbol *assoc_def = coff_obj_writer_push_symbol_associative(writer, assoc, head);
    COFF_ObjSymbol *undef = coff_obj_writer_push_symbol_undef(writer, str8_lit("undef"));
    COFF_ObjSymbol *common = coff_obj_writer_push_symbol_common(writer, str8_lit("common"), 32);
    coff_obj_writer_section_push_reloc_addr(writer, assoc, 0, weak);

    String8 data = coff_obj_writer_serialize(arena, writer);
    COFF_FileHeaderInfo info = coff_file_header_info_from_data(data);
    T_Ok(info.is_big_obj == (section_count > 0xfeff));
    T_Ok(info.section_count_no_null == section_count);
    T_Ok(info.symbol_size == (info.is_big_obj ? sizeof(COFF_Symbol32) : sizeof(COFF_Symbol16)));
    T_Ok(info.symbol_count == 10);
    String8 symbols = str8_substr(data, info.symbol_table_range);
    String8 strings = str8_substr(data, info.string_table_range);
    COFF_ParsedSymbol parsed_target = coff_parse_symbol(info, strings, symbols, target->idx);
    T_Ok(parsed_target.section_number == section_count - 1);
    T_Ok(str8_match(parsed_target.name, target->name, 0));
    COFF_ParsedSymbol parsed_assoc = coff_parse_symbol(info, strings, symbols, assoc_def->idx);
    U32 parent = 0;
    COFF_ComdatSelectType selection = 0;
    coff_parse_secdef(parsed_assoc, info.is_big_obj, &selection, &parent, 0, 0);
    T_Ok(parsed_assoc.section_number == section_count);
    T_Ok(selection == COFF_ComdatSelect_Associative && parent == section_count - 1);
    COFF_ParsedSymbol parsed_weak = coff_parse_symbol(info, strings, symbols, weak->idx);
    COFF_SymbolWeakExt *weak_aux = coff_parse_weak_tag(parsed_weak, info.is_big_obj);
    T_Ok(weak_aux->tag_index == target->idx && weak_aux->characteristics == COFF_WeakExt_SearchAlias);
    COFF_ParsedSymbol parsed_absolute = coff_parse_symbol(info, strings, symbols, absolute->idx);
    T_Ok(parsed_absolute.section_number == COFF_Symbol_AbsSection32 && parsed_absolute.value == 17);
    T_Ok(coff_parse_symbol(info, strings, symbols, undef->idx).section_number == COFF_Symbol_UndefinedSection);
    COFF_ParsedSymbol parsed_common = coff_parse_symbol(info, strings, symbols, common->idx);
    T_Ok(parsed_common.section_number == COFF_Symbol_UndefinedSection && parsed_common.value == 32);
    COFF_SectionHeader *sections = (COFF_SectionHeader *)(data.str + info.section_table_range.min);
    COFF_Reloc *reloc = (COFF_Reloc *)(data.str + sections[section_count - 1].relocs_foff);
    T_Ok(sections[section_count - 1].reloc_count == 1 && reloc->isymbol == weak->idx);
    T_Ok(t_write_file(str8f(arena, "writer_%u.obj", section_count), data));
    coff_obj_writer_release(&writer);
  }
}

TEST(coff_codec_object_parity)
{
  String8 source = str8_lit(
      "test: { artifacts: { value: { coff: { object: { machine: x64, sections: { text: { name: \".text\", permissions: (read, execute), content: code, "
      "data: { hex: \"00000000c3\" }, relocations: { ref: { type: Addr32Nb, offset: 0, symbol: undef } } }, head: { name: \".head\", permissions: (read), content: "
      "initialized_data, "
      "flags: (link_comdat), data: { text: \"head\" } }, assoc: { name: \".assoc\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"assoc\" "
      "} } }, symbols: { "
      "fallback: { kind: absolute, name: \"fallback\", value: 17, storage: external }, entry: { kind: external, name: \"entry\", section: text, value: 0 }, func: { kind: "
      "external_function, "
      "name: \"func\", section: text, value: 1 }, local: { kind: static, name: \"local\", section: text, value: 2 }, head_def: { kind: section_definition, section: head, "
      "selection: Any }, "
      "assoc_def: { kind: section_definition, section: assoc, selection: Associative, associate: head }, weak: { kind: weak, name: \"weak\", fallback: fallback, search: alias }, "
      "abs_static: { kind: absolute, "
      "name: \"abs_static\", value: 34, storage: static }, undef: { kind: undefined, name: \"undef\" }, undef_func: { kind: undefined_function, name: \"undef_func\" }, "
      "undef_section: { kind: undefined_section, "
      "name: \"undef_section\", value: 51 }, section_symbol: { kind: section, name: \"section_symbol\", section: text }, common: { kind: common, name: \"common\", size: 64 } }, "
      "directives: { directive: \"/export:entry\" } } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("object_parity.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  T_Artifact *value = t_artifact_from_name(&script, str8_lit("value"));
  String8 expected =
      t_coff_from_def_obj(arena, (T_COFF_DefObj){
                                     .machine = T_COFF_DefSetMachine(X64),
                                     .sections = (T_COFF_DefSection[]){{"text", ".text", str8_lit_comp("\0\0\0\0\xc3"), .flags = "rx:code",
                                                                        .relocs = (T_COFF_DefReloc[]){T_COFF_DefReloc(X64_Addr32Nb, 0, "undef"), {0}}},
                                                                       {"head", ".head", str8_lit_comp("head"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
                                                                       {"assoc", ".assoc", str8_lit_comp("assoc"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
                                                                       {0}},
                                     .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_AbsExtern("fallback", 17),
                                                                     T_COFF_DefSymbol_Extern("entry", "text", 0),
                                                                     T_COFF_DefSymbol_ExternFunc("func", "text", 1),
                                                                     T_COFF_DefSymbol_Static("local", "text", 2),
                                                                     T_COFF_DefSymbol_Secdef("head", COFF_ComdatSelect_Any),
                                                                     T_COFF_DefSymbol_Associative("assoc", "head"),
                                                                     T_COFF_DefSymbol_Weak("weak", COFF_WeakExt_SearchAlias, "fallback"),
                                                                     T_COFF_DefSymbol_AbsStatic("abs_static", 34),
                                                                     T_COFF_DefSymbol_Undef("undef"),
                                                                     T_COFF_DefSymbol_UndefFunc("undef_func"),
                                                                     T_COFF_DefSymbol_UndefSec("undef_section", 51),
                                                                     T_COFF_DefSymbol_Sect("section_symbol", "text"),
                                                                     T_COFF_DefSymbol_Common("common", 64),
                                                                     {0}},
                                     .directives = (char *[]){"/export:entry", 0},
                                 });
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
  T_Ok(value != 0 && str8_match(value->data, expected, 0));
  T_Ok(make_directory(t_make_file_path(arena, str8_lit("legacy"))));
  T_Ok(make_directory(t_make_file_path(arena, str8_lit("script"))));
  T_Ok(t_write_file(str8_lit("legacy/value.obj"), expected));
  T_Ok(t_write_file(str8_lit("script/value.obj"), value->data));
  T_Ok(t_match_folders(t_make_file_path(arena, str8_lit("legacy")), t_make_file_path(arena, str8_lit("script"))));
  MD_Node *semantic = 0;
  result = value->codec->decode(&script, value, &semantic);
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
  if (semantic != 0) {
    MD_Node *object = t_child_from_string(semantic, "object");
    MD_Node *sections = t_child_from_string(object, "sections");
    MD_Node *symbols = t_child_from_string(object, "symbols");
    T_Ok(str8_match(t_scalar_string_from_node(t_child_from_string(object, "machine")), str8_lit("Amd64"), 0));
    T_Ok(str8_match(t_scalar_string_from_node(t_child_from_string(t_child_from_string(sections, "section_1"), "data")), str8_lit("00000000c3"), 0));
    T_Ok(str8_match(t_scalar_string_from_node(t_child_from_string(t_child_from_string(symbols, "symbol_0"), "name")), str8_lit("fallback"), 0));
  }
}

TEST(coff_codec_library_parity)
{
  String8 source = str8_lit("test: { artifacts: { value: { coff: { library: { timestamp: 0, mode: 0, second_linker_member: true, members: { implementation: "
                            "{ path: \"member.obj\", object: { machine: x64, "
                            "sections: { text: { name: \".text\", permissions: (read, execute), content: code, data: { hex: \"c3\" } } }, symbols: { func: { kind: "
                            "external_function, name: \"func\", section: text, value: 0 } } } }, "
                            "imported: { import: { dll: \"foo.dll\", name: \"foo\", machine: x64, timestamp: 4294967295, type: code, lookup: name, hint: 0 } }, scaffold: { "
                             "dll_import: { name: \"bar.dll\", machine: x64, timestamp: 0 } } } } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("library_parity.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  T_Artifact *value = t_artifact_from_name(&script, str8_lit("value"));
  String8 expected = t_coff_from_def_lib(
      arena, (T_COFF_DefLib){
                 .emit_second_member = 1,
                 .members = (T_COFF_DefLibMember[]){{.type = T_COFF_DefLibMember_Obj,
                                                     .obj = {.machine = T_COFF_DefSetMachine(X64),
                                                             .path = str8_lit_comp("member.obj"),
                                                             .sections = (T_COFF_DefSection[]){{"text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code"}, {0}},
                                                             .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_ExternFunc("func", "text", 0), {0}}}},
                                                    {.type = T_COFF_DefLibMember_Import,
                                                     .import = {.dll = "foo.dll",
                                                                .name = "foo",
                                                                .import_by = COFF_ImportBy_Name,
                                                                .type = COFF_ImportHeader_Code,
                                                                .hit_or_ordinal = 0,
                                                                .time_stamp = T_COFF_DefSetTimeStamp(~0u),
                                                                .machine = T_COFF_DefSetMachine(X64)}},
                                                    {.type = T_COFF_DefLibMember_DllImportStatic, .dll_import = {.name = "bar.dll", .machine = T_COFF_DefSetMachine(X64)}},
                                                    {0}},
             });
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
  T_Ok(value != 0 && str8_match(value->data, expected, 0));
  T_Ok(make_directory(t_make_file_path(arena, str8_lit("legacy"))));
  T_Ok(make_directory(t_make_file_path(arena, str8_lit("script"))));
  T_Ok(t_write_file(str8_lit("legacy/value.lib"), expected));
  T_Ok(t_write_file(str8_lit("script/value.lib"), value->data));
  T_Ok(t_match_folders(t_make_file_path(arena, str8_lit("legacy")), t_make_file_path(arena, str8_lit("script"))));
  MD_Node *semantic = 0;
  result = value->codec->decode(&script, value, &semantic);
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
  if (semantic != 0) {
    MD_Node *library = t_child_from_string(semantic, "library");
    T_Ok(str8_match(t_scalar_string_from_node(t_child_from_string(library, "member_count")), str8_lit("5"), 0));
    T_Ok(!md_node_is_nil(t_child_from_string(t_child_from_string(t_child_from_string(library, "members"), "member_1"), "import")));
  }
}

TEST(coff_codec_object_layout_parity)
{
  String8 source = str8_lit(
      "test: { artifacts: { value: { coff: { object: { machine: x64, timestamp: 305419896, sections: { "
      "write: { name: \".write\", permissions: (read, write), content: initialized_data, alignment: 32, data: { text: \"write\" } }, "
      "bss: { name: \".bss\", permissions: (read, write), content: uninitialized_data, data: { zero: 4 } }, "
      "long: { name: \".very_long_section_name\", permissions: (read), content: initialized_data, raw_flags: 8, data: { text: \"long\" } }, "
      "empty: { name: \".empty\", permissions: (read), content: initialized_data, data: { zero: 0 } } }, "
      "directives: { directive: \"/defaultlib:first\", directive: \"/include:middle\", directive: \"/export:last\" } } } } } }");
  T_Result result = {0};
  String8 actual = t_coff_test_encode(arena, ctx, str8_lit("object_layout_parity.tst"), source, &result);
  String8 expected = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .time_stamp = T_COFF_DefSetTimeStamp(305419896),
      .sections = (T_COFF_DefSection[]){
          {"write", ".write", str8_lit_comp("write"), .flags = "rw:data@32"},
          {"bss", ".bss", str8_lit_comp("\0\0\0\0"), .flags = "rw:bss"},
          {"long", ".very_long_section_name", str8_lit_comp("long"), .flags = "r:data", .raw_flags = COFF_SectionFlag_TypeNoPad},
          {"empty", ".empty", str8_zero(), .flags = "r:data"},
          {0}},
      .directives = (char *[]){"/defaultlib:first", "/include:middle", "/export:last", 0},
  });
  T_Ok(t_result_is_ok(result));
  T_Ok(str8_match(actual, expected, 0));
}

TEST(coff_codec_comdat_weak_aux_parity)
{
  String8 source = str8_lit(
      "test: { artifacts: { value: { coff: { object: { machine: x64, sections: { "
      "null: { name: \".null\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"0\" } }, "
      "nodup: { name: \".nodup\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"1\" } }, "
      "any: { name: \".any\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"2\" } }, "
      "same: { name: \".same\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"3\" } }, "
      "exact: { name: \".exact\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"4\" } }, "
      "assoc: { name: \".assoc\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"5\" } }, "
      "largest: { name: \".largest\", permissions: (read), content: initialized_data, flags: (link_comdat), data: { text: \"6\" } }, "
      "relocs: { name: \".relocs\", permissions: (read), content: initialized_data, data: { zero: 4 }, relocations: { "
      "late: { type: Addr32Nb, offset: 0, symbol: late } } } }, symbols: { "
      "fallback: { kind: absolute, name: \"fallback\", value: 1, storage: external }, "
      "null_def: { kind: section_definition, section: null, selection: Null }, "
      "nodup_def: { kind: section_definition, section: nodup, selection: NoDuplicates }, "
      "any_def: { kind: section_definition, section: any, selection: Any }, "
      "same_def: { kind: section_definition, section: same, selection: SameSize }, "
      "exact_def: { kind: section_definition, section: exact, selection: ExactMatch }, "
      "assoc_def: { kind: section_definition, section: assoc, selection: Associative, associate: any }, "
      "largest_def: { kind: section_definition, section: largest, selection: Largest }, "
      "weak_no_library: { kind: weak, name: \"weak_no_library\", fallback: fallback, search: no_library }, "
      "weak_search_library: { kind: weak, name: \"weak_search_library\", fallback: fallback, search: search_library }, "
      "weak_alias: { kind: weak, name: \"weak_alias\", fallback: fallback, search: alias }, "
      "weak_anti_dependency: { kind: weak, name: \"weak_anti_dependency\", fallback: fallback, search: anti_dependency }, "
      "late: { kind: undefined, name: \"relocation_target_after_aux_records\" } } } } } } }");
  T_Result result = {0};
  String8 actual = t_coff_test_encode(arena, ctx, str8_lit("comdat_weak_aux_parity.tst"), source, &result);
  String8 expected = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
          {"null", ".null", str8_lit_comp("0"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
          {"nodup", ".nodup", str8_lit_comp("1"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
          {"any", ".any", str8_lit_comp("2"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
          {"same", ".same", str8_lit_comp("3"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
          {"exact", ".exact", str8_lit_comp("4"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
          {"assoc", ".assoc", str8_lit_comp("5"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
          {"largest", ".largest", str8_lit_comp("6"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT},
          {"relocs", ".relocs", str8_lit_comp("\0\0\0\0"), .flags = "r:data",
              .relocs = (T_COFF_DefReloc[]){T_COFF_DefReloc(X64_Addr32Nb, 0, "relocation_target_after_aux_records"), {0}}},
          {0}},
      .symbols = (T_COFF_DefSymbol[]){
          T_COFF_DefSymbol_AbsExtern("fallback", 1),
          T_COFF_DefSymbol_Secdef("null", COFF_ComdatSelect_Null),
          T_COFF_DefSymbol_Secdef("nodup", COFF_ComdatSelect_NoDuplicates),
          T_COFF_DefSymbol_Secdef("any", COFF_ComdatSelect_Any),
          T_COFF_DefSymbol_Secdef("same", COFF_ComdatSelect_SameSize),
          T_COFF_DefSymbol_Secdef("exact", COFF_ComdatSelect_ExactMatch),
          T_COFF_DefSymbol_Associative("assoc", "any"),
          T_COFF_DefSymbol_Secdef("largest", COFF_ComdatSelect_Largest),
          T_COFF_DefSymbol_Weak("weak_no_library", COFF_WeakExt_NoLibrary, "fallback"),
          T_COFF_DefSymbol_Weak("weak_search_library", COFF_WeakExt_SearchLibrary, "fallback"),
          T_COFF_DefSymbol_Weak("weak_alias", COFF_WeakExt_SearchAlias, "fallback"),
          T_COFF_DefSymbol_Weak("weak_anti_dependency", COFF_WeakExt_AntiDependency, "fallback"),
          T_COFF_DefSymbol_Undef("relocation_target_after_aux_records"),
          {0}},
  });
  T_Ok(t_result_is_ok(result));
  T_Ok(str8_match(actual, expected, 0));
}

TEST(coff_codec_relocation_variants_parity)
{
  struct RelocationCase
  {
    String8 machine_name;
    COFF_MachineType machine;
  } cases[] = {
      {str8_lit_comp("x64"), COFF_MachineType_X64},
      {str8_lit_comp("x86"), COFF_MachineType_X86},
      {str8_lit_comp("arm"), COFF_MachineType_Arm},
      {str8_lit_comp("arm64"), COFF_MachineType_Arm64},
  };
  for EachElement(case_idx, cases)
  {
    U64 relocation_count = 0;
    for EachIndex(type, 256) { relocation_count += coff_string_from_reloc(cases[case_idx].machine, (COFF_RelocType)type).size != 0; }
    COFF_RelocType *types = push_array(arena, COFF_RelocType, relocation_count);
    T_COFF_DefReloc *relocations = push_array(arena, T_COFF_DefReloc, relocation_count + 1);
    String8List source = {0};
    str8_list_pushf(arena, &source,
                    "test: { artifacts: { value: { coff: { object: { machine: %S, sections: { data: { name: \".data\", "
                    "permissions: (read), content: initialized_data, data: { zero: %llu }, relocations: {",
                    cases[case_idx].machine_name, relocation_count);
    U64 relocation_idx = 0;
    for EachIndex(type, 256)
    {
      String8 type_name = coff_string_from_reloc(cases[case_idx].machine, (COFF_RelocType)type);
      if (type_name.size != 0)
      {
        types[relocation_idx] = (COFF_RelocType)type;
        relocations[relocation_idx].type = &types[relocation_idx];
        relocations[relocation_idx].apply_off = relocation_idx;
        relocations[relocation_idx].symbol = "target";
        str8_list_pushf(arena, &source, " r%llu: { type: %S, offset: %llu, symbol: target }", relocation_idx, type_name, relocation_idx);
        relocation_idx += 1;
      }
    }
    str8_list_pushf(arena, &source, " } } }, symbols: { target: { kind: undefined, name: \"target\" } } } } } } }");
    String8 file_name = push_str8f(arena, "relocation_variants_%S.tst", cases[case_idx].machine_name);
    T_Result result = {0};
    String8 actual = t_coff_test_encode(arena, ctx, file_name, str8_list_join(arena, &source, 0), &result);
    String8 expected = t_coff_from_def_obj(arena, (T_COFF_DefObj){
        .machine = &cases[case_idx].machine,
        .sections = (T_COFF_DefSection[]){{"data", ".data", str8(push_array(arena, U8, relocation_count), relocation_count), .flags = "r:data", .relocs = relocations}, {0}},
        .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_Undef("target"), {0}},
    });
    T_Ok(t_result_is_ok(result));
    T_Ok(str8_match(actual, expected, 0));
  }
}

TEST(coff_codec_library_object_paths_parity)
{
  String8 source = str8_lit(
      "test: { artifacts: { value: { coff: { library: { timestamp: 0, mode: 420, second_linker_member: false, members: { "
      "short: { path: \"a.obj\", object: { machine: x64, sections: { text: { name: \".text\", permissions: (read, execute), content: code, data: { hex: \"c3\" } } }, "
      "symbols: { a: { kind: external_function, name: \"a\", section: text, value: 0 } } } }, "
      "middle: { path: \"middle.obj\", object: { machine: x64, sections: { data: { name: \".data\", permissions: (read, write), content: initialized_data, data: { text: \"middle\" } } }, "
      "symbols: { middle: { kind: external, name: \"middle\", section: data, value: 0 } } } }, "
      "long: { path: \"objects/this_is_a_long_member_name.obj\", object: { machine: x64, sections: { text: { name: \".text\", permissions: (read, execute), content: code, data: { hex: \"c3\" } } }, "
      "symbols: { z: { kind: external_function, name: \"z\", section: text, value: 0 } } } } } } } } } }");
  T_Result result = {0};
  String8 actual = t_coff_test_encode(arena, ctx, str8_lit("library_object_paths_parity.tst"), source, &result);
  String8 expected = t_coff_from_def_lib(arena, (T_COFF_DefLib){
      .mode = 420,
      .emit_second_member = 0,
      .members = (T_COFF_DefLibMember[]){
          {.type = T_COFF_DefLibMember_Obj,
              .obj = {.machine = T_COFF_DefSetMachine(X64), .path = str8_lit_comp("a.obj"),
                  .sections = (T_COFF_DefSection[]){{"text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code"}, {0}},
                  .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_ExternFunc("a", "text", 0), {0}}}},
          {.type = T_COFF_DefLibMember_Obj,
              .obj = {.machine = T_COFF_DefSetMachine(X64), .path = str8_lit_comp("middle.obj"),
                  .sections = (T_COFF_DefSection[]){{"data", ".data", str8_lit_comp("middle"), .flags = "rw:data"}, {0}},
                  .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_Extern("middle", "data", 0), {0}}}},
          {.type = T_COFF_DefLibMember_Obj,
              .obj = {.machine = T_COFF_DefSetMachine(X64), .path = str8_lit_comp("objects/this_is_a_long_member_name.obj"),
                  .sections = (T_COFF_DefSection[]){{"text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code"}, {0}},
                  .symbols = (T_COFF_DefSymbol[]){T_COFF_DefSymbol_ExternFunc("z", "text", 0), {0}}}},
          {0}},
  });
  T_Ok(t_result_is_ok(result));
  T_Ok(str8_match(actual, expected, 0));
}

TEST(coff_codec_import_variants_parity)
{
  struct ImportTypeCase
  {
    String8 name;
    COFF_ImportType type;
  } type_cases[] = {
      {str8_lit_comp("code"), COFF_ImportHeader_Code},
      {str8_lit_comp("data"), COFF_ImportHeader_Data},
      {str8_lit_comp("const"), COFF_ImportHeader_Const},
  };
  struct ImportByCase
  {
    String8 name;
    COFF_ImportByType type;
  } import_by_cases[] = {
      {str8_lit_comp("ordinal"), COFF_ImportBy_Ordinal},
      {str8_lit_comp("name"), COFF_ImportBy_Name},
      {str8_lit_comp("name_no_prefix"), COFF_ImportBy_NameNoPrefix},
      {str8_lit_comp("undecorate"), COFF_ImportBy_Undecorate},
  };
  struct ImportMachineCase
  {
    String8 name;
    COFF_MachineType type;
  } machine_cases[] = {
      {str8_lit_comp("x86"), COFF_MachineType_X86},
      {str8_lit_comp("x64"), COFF_MachineType_X64},
  };
  U64 member_count = ArrayCount(type_cases) * ArrayCount(import_by_cases) * ArrayCount(machine_cases);
  T_COFF_DefLibMember *members = push_array(arena, T_COFF_DefLibMember, member_count + 1);
  COFF_MachineType *machines = push_array(arena, COFF_MachineType, member_count);
  COFF_TimeStamp *timestamps = push_array(arena, COFF_TimeStamp, member_count);
  String8List source = {0};
  str8_list_pushf(arena, &source,
                  "test: { artifacts: { value: { coff: { library: { timestamp: 0, mode: 0, second_linker_member: false, members: {");
  U64 member_idx = 0;
  for EachElement(machine_idx, machine_cases)
  {
    for EachElement(type_idx, type_cases)
    {
      for EachElement(import_by_idx, import_by_cases)
      {
        U16 hint_or_ordinal = (U16)(100 + member_idx);
        COFF_TimeStamp timestamp = (COFF_TimeStamp)(1000 + member_idx);
        String8 name = machine_cases[machine_idx].type == COFF_MachineType_X86 ? push_str8f(arena, "_codec_import_%llu@4", member_idx)
                                                                              : push_str8f(arena, "codec_import_%llu", member_idx);
        char *number_field = import_by_cases[import_by_idx].type == COFF_ImportBy_Ordinal ? "ordinal" : "hint";
        str8_list_pushf(arena, &source,
                        " m%llu: { import: { dll: \"imports.dll\", name: \"%S\", machine: %S, timestamp: %u, type: %S, lookup: %S, %s: %u } }",
                        member_idx, name, machine_cases[machine_idx].name, timestamp, type_cases[type_idx].name, import_by_cases[import_by_idx].name, number_field,
                        hint_or_ordinal);
        machines[member_idx] = machine_cases[machine_idx].type;
        timestamps[member_idx] = timestamp;
        members[member_idx].type = T_COFF_DefLibMember_Import;
        members[member_idx].import.dll = "imports.dll";
        members[member_idx].import.name = (char *)name.str;
        members[member_idx].import.import_by = import_by_cases[import_by_idx].type;
        members[member_idx].import.type = type_cases[type_idx].type;
        members[member_idx].import.hit_or_ordinal = hint_or_ordinal;
        members[member_idx].import.time_stamp = &timestamps[member_idx];
        members[member_idx].import.machine = &machines[member_idx];
        member_idx += 1;
      }
    }
  }
  str8_list_pushf(arena, &source, " } } } } } }");
  T_Result result = {0};
  String8 actual = t_coff_test_encode(arena, ctx, str8_lit("import_variants_parity.tst"), str8_list_join(arena, &source, 0), &result);
  String8 expected = t_coff_from_def_lib(arena, (T_COFF_DefLib){.emit_second_member = 0, .members = members});
  T_Ok(t_result_is_ok(result));
  T_Ok(str8_match(actual, expected, 0));
}

TEST(coff_codec_validation_precedes_encoding)
{
  String8 source = str8_lit("test: { artifacts: { value: { coff: { object: { machine: x64, sections: { text: { name: \".text\", permissionz: (read), content: "
                            "code, data: { hex: \"c3\" } } } } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("invalid_coff.tst"), source, &script);
  T_Artifact *value = t_artifact_from_name(&script, str8_lit("value"));
  T_Ok(result.code == T_ResultCode_ValidationError);
  T_Ok(value != 0 && value->data.size == 0 && value->state == T_ArtifactState_Failed);
}

TEST(coff_codec_const_import)
{
  String8 source = str8_lit("test: { artifacts: { value: { coff: { library: { members: { imported: { import: { dll: \"foo.dll\", name: \"foo\", machine: x64, "
                            "type: const, lookup: undecorate } } } } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("const_import.tst"), source, &script);
  if (t_result_is_ok(result)) { result = t_script_execute(&script); }
  T_Artifact *value = t_artifact_from_name(&script, str8_lit("value"));
  MD_Node *semantic = 0;
  if (t_result_is_ok(result)) { result = value->codec->decode(&script, value, &semantic); }
  if (!t_result_is_ok(result)) { t_script_test_log_result(arena, ctx, result); }
  T_Ok(t_result_is_ok(result));
  if (semantic != 0) {
    MD_Node *members = t_child_from_string(t_child_from_string(semantic, "library"), "members");
    MD_Node *import = t_child_from_string(t_child_from_string(members, "member_0"), "import");
    T_Ok(str8_match(t_scalar_string_from_node(t_child_from_string(import, "type")), str8_lit("Const"), 0));
    T_Ok(str8_match(t_scalar_string_from_node(t_child_from_string(import, "lookup")), str8_lit("3"), 0));
  }
}

TEST(coff_codec_rejects_variant_fields)
{
  String8 source =
      str8_lit("test: { artifacts: { value: { coff: { object: { machine: x64, sections: { text: { name: \".text\", permissions: (read), content: code, "
               "data: { hex: \"c3\" } } }, "
                "symbols: { bad_external: { kind: external, name: \"bad\", section: text, size: 4 }, bad_absolute: { kind: absolute, name: \"abs\", value: 1 } } } } } } }");
  T_Context script = {0};
  T_Result result = t_script_parse(arena, ctx, &t_codec_script_suite, str8_lit("invalid_variant.tst"), source, &script);
  T_Artifact *value = t_artifact_from_name(&script, str8_lit("value"));
  T_Ok(result.code == T_ResultCode_ValidationError);
  T_Ok(result.diagnostics.count >= 2);
  T_Ok(value != 0 && value->data.size == 0 && value->state == T_ArtifactState_Failed);
}

TEST(coff_codec_rejects_invalid_section_name_offset)
{
  String8 data = t_coff_from_def_obj(arena, (T_COFF_DefObj){
                                                .machine = T_COFF_DefSetMachine(X64),
                                                .sections = (T_COFF_DefSection[]){{"text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code"}, {0}},
                                            });
  COFF_FileHeaderInfo header = coff_file_header_info_from_data(data);
  COFF_SectionHeader *section = (COFF_SectionHeader *)(data.str + header.section_table_range.min);
  MemoryZeroArray(section->name);
  MemoryCopy(section->name, "/999999", 7);
  T_Context decode = {.arena = arena};
  T_Artifact artifact = {.data = data, .definition = &md_nil_node};
  MD_Node *semantic = 0;
  T_Result result = t_coff_decode(&decode, &artifact, &semantic);
  T_Ok(result.code == T_ResultCode_ValidationError);
  T_Ok(semantic == 0);
}

TEST(coff_codec_rejects_truncated_import)
{
  COFF_LibWriter *writer = coff_lib_writer_alloc();
  coff_lib_writer_push_import(writer, COFF_MachineType_X64, 0, str8_lit("foo.dll"), COFF_ImportBy_Name, str8_lit("foo"), 0, COFF_ImportHeader_Code);
  String8 data = coff_lib_writer_serialize(arena, writer, 0, 0, 1);
  coff_lib_writer_release(&writer);
  U64 offset = coff_regular_archive_member_iter_init(data);
  COFF_ArchiveMember member = {0};
  while (coff_regular_archive_member_iter_next(data, &offset, &member)) {
    if (coff_is_import(member.data)) {
      COFF_ImportHeader *header = (COFF_ImportHeader *)member.data.str;
      header->data_size += 1;
      break;
    }
  }
  T_Context decode = {.arena = arena};
  T_Artifact artifact = {.data = data, .definition = &md_nil_node};
  MD_Node *semantic = 0;
  T_Result result = t_coff_decode(&decode, &artifact, &semantic);
  T_Ok(result.code == T_ResultCode_ValidationError);
  T_Ok(semantic == 0);
}
