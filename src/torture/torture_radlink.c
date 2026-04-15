// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal T_Linker
t_id_linker(void)
{
  String8 name = str8_chop_last_dot(str8_skip_last_slash(g_linker));
  if (str8_match(name, str8_lit("radlink"),  StringMatchFlag_CaseInsensitive)) { return T_Linker_RAD;  }
  if (str8_match(name, str8_lit("link"),     StringMatchFlag_CaseInsensitive)) { return T_Linker_MSVC; }
  if (str8_match(name, str8_lit("lld-link"), StringMatchFlag_CaseInsensitive)) { return T_Linker_LLVM; }
  return T_Linker_Null;
}

internal COFF_ObjSection *
t_push_text_section(COFF_ObjWriter *obj_writer, String8 data)
{
  return coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_Align1Bytes, data);
}

internal COFF_ObjSection *
t_push_data_section(COFF_ObjWriter *obj_writer, String8 data)
{
  return coff_obj_writer_push_section(obj_writer, str8_lit(".data"), PE_DATA_SECTION_FLAGS, data);
}

internal COFF_ObjSection *
t_push_rdata_section(COFF_ObjWriter *obj_writer, String8 data)
{
  return coff_obj_writer_push_section(obj_writer, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS, data);
}

internal String8
t_make_sec_defn_obj(Arena *arena, String8 payload)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  COFF_ObjSection *mysect_section = coff_obj_writer_push_section(obj_writer, str8_lit(".mysect"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes, payload);
  coff_obj_writer_push_symbol_secdef(obj_writer, mysect_section, COFF_ComdatSelect_Null);
  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);
  return obj;
}

internal String8
t_make_obj_with_directive(Arena *arena, String8 directive)
{
  COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  coff_obj_writer_push_directive(cow, directive);
  String8 obj = coff_obj_writer_serialize(arena, cow);
  coff_obj_writer_release(&cow);
  return obj;
}

internal String8
t_make_entry_obj(Arena *arena)
{
  /*
     "machine": "x64",
     "sectab":  [ ".text": [ "alias: "entry", "data": [ 0xc3 ] ] ]
     "symtab" : [ { "name": "entry", "section": "entry", "offset": "100", "type": "extern" } ]
   */
  
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  U8 text[] = { 0xc3 };
  COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
  coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, text_sect);
  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);
  return obj;
}

internal B32
t_write_entry_obj(void)
{
  Temp scratch = scratch_begin(0,0);
  String8 obj   = t_make_entry_obj(scratch.arena);
  B32     is_ok = t_write_file(str8_lit("entry.obj"), obj);
  scratch_end(scratch);
  return is_ok;
}

////////////////////////////////

#define T_Group "Linker" 

T_BeginTest(machine_compat_check)
{
  // unknown.obj
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_Unknown);
    t_push_data_section(obj_writer, str8_lit("unknown"));
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("unknown.obj"), obj));
  }

  // x64.obj
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    t_push_data_section(obj_writer, str8_lit("x64"));
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("x64.obj"), obj));
  }

  // entry.obj
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  // arm64.obj
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_Arm64);
    t_push_data_section(obj_writer, str8_lit("arm64"));
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("arm64.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe entry.obj unknown.obj x64.obj");
  T_Ok(g_last_exit_code == 0);

  // test objs with conflicting machines
  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe entry.obj unknown.obj x64.obj arm64.obj");
  T_Ok(g_last_exit_code != 0);

  // check /MACHINE switch
  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe /machine:amd64 arm64.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
}

T_BeginTest(simple_link_test)
{
  U8 text_payload[] = { 0xC3 };

  String8 main_obj;
  {
    COFF_ObjWriter  *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect  = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text_payload));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data"), PE_DATA_SECTION_FLAGS, str8_lit("qwe"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".zero"), PE_BSS_SECTION_FLAGS, str8(0, 5));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    main_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 main_obj_name = str8_lit("main.obj");
  T_Ok(t_write_file(main_obj_name, main_obj));

  int file_align = 512;
  int virt_align = 4096;
  String8 out_name = str8_lit("a.exe");
  t_invoke_linkerf("/entry:my_entry /subsystem:console /fixed /filealign:%d /align:%d /out:%S %S", file_align, virt_align, out_name, main_obj_name);
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, out_name);
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  T_Ok(!pe.is_pe32);
  T_Ok(pe.section_count == 3);
  T_Ok(pe.arch == Arch_x64);
  T_Ok(pe.subsystem == PE_WindowsSubsystem_WINDOWS_CUI);
  T_Ok(pe.virt_section_align == virt_align);
  T_Ok(pe.file_section_align == file_align);
  T_Ok(pe.symbol_count == 0);
  T_Ok(pe.data_dir_count == PE_DataDirectoryIndex_COUNT);

  // check section alignment
  for EachIndex(sect_idx, pe.section_count) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];
    T_Ok(AlignPadPow2(sect_header->fsize, file_align) == 0);
    T_Ok(AlignPadPow2(sect_header->voff, virt_align) == 0);
  }

  COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_section != 0);
  T_Ok(text_section->foff == file_align);
  T_Ok(pe.entry_point == text_section->voff);

  COFF_SectionHeader *data_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  T_Ok(data_section != 0);

  COFF_SectionHeader *zero_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".zero"));
  T_Ok(zero_section != 0);

  String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + text_section->vsize));
  T_Ok(str8_match(text_data, str8_array_fixed(text_payload), 0));

  PE_OptionalHeader32Plus *opt = str8_deserial_get_raw_ptr(exe, pe.optional_header_off, sizeof(*opt));
  T_Ok(opt->sizeof_code == text_section->fsize);
  T_Ok(opt->sizeof_inited_data == (text_section->fsize + data_section->fsize));
  T_Ok(opt->sizeof_uninited_data == 0x200);
  T_Ok(opt->code_base == 0x1000);
  T_Ok(opt->image_base == 0x140000000);
  T_Ok(opt->major_os_ver == 6);
  T_Ok(opt->minor_os_ver == 0);
  T_Ok(opt->major_img_ver == 0);
  T_Ok(opt->minor_img_ver == 0);
  T_Ok(opt->major_subsystem_ver == 6);
  T_Ok(opt->minor_subsystem_ver == 0);
  T_Ok(opt->win32_version_value == 0);
  T_Ok(opt->sizeof_image == 0x4000);
  T_Ok(opt->sizeof_headers == 0x200);
  T_Ok(opt->dll_characteristics == 0x8120);
  T_Ok(opt->loader_flags == 0);
}


T_BeginTest(out_of_bounds_section_number)
{
  // bad.obj
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *foo = coff_obj_writer_push_section(obj_writer, str8_lit(".foo"), PE_DATA_SECTION_FLAGS, str8_lit("foo"));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("foo"), 0, foo);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    {
      COFF_FileHeaderInfo header = coff_file_header_info_from_data(obj);
      String8 string_table = str8_substr(obj, header.string_table_range);
      String8 symbol_table = str8_substr(obj, header.symbol_table_range);
      COFF_ParsedSymbol symbol = coff_parse_symbol(header, string_table, symbol_table, 0);
      COFF_Symbol16 *symbol16 = symbol.raw_symbol;
      symbol16->section_number = 123;
    }
    T_Ok(t_write_file(str8_lit("bad.obj"), obj));
  }

  // entry.obj
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("foo"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj bad.obj");
  T_Ok(g_last_exit_code != 0);
}


T_BeginTest(merge)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_section(obj_writer, str8_lit(".test"), PE_DATA_SECTION_FLAGS, str8_lit("hello, world"));
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("test.obj"), obj));
  }

  T_Ok(t_write_entry_obj());

  // circular merge
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.test entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);

  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_CircularMerge);
  }

  // circular merge with extra link
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.data /merge:.data=.test entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_CircularMerge);
  }

  // merge with non-defined section
  {
    g_last_exit_code;

    t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.qwe entry.obj test.obj");
    T_Ok(g_last_exit_code == 0);

    // make sure linker created .qwe and merged .test into it
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".qwe"));
    T_Ok(sect != 0);
    T_Ok(sect->flags == PE_DATA_SECTION_FLAGS);
    String8 qwe = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(qwe, str8_lit("hello, world"),0));
  }

  // illegal merge with .reloc
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.reloc entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  }

  // illegal merge with .rsrc
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.rsrc entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  }

  // merge non-defined section with defined section
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.qwe=.test entry.obj test.obj");
  T_Ok(g_last_exit_code == 0);

  // merge .test -> .qwe -> .data
  {
    t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.qwe /merge:.qwe=.data entry.obj test.obj");
    T_Ok(g_last_exit_code == 0);

    // make sure linker merged .test into .data
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(sect != 0);
    T_Ok(sect->flags == PE_DATA_SECTION_FLAGS);
    String8 data = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("hello, world"),0));
  }
}

T_BeginTest(link_undef)
{
  String8 undef_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("undef"));
    undef_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("undef"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("undef.obj"), undef_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // try linking unresolved symbol and see if linker picks up on that
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj undef.obj");
  T_Ok(g_last_exit_code == LNK_Error_UnresolvedSymbol);
}

T_BeginTest(link_unref_undef)
{
  String8 undef_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("undef"));
    undef_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xc3 };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("undef.obj"), undef_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // try linking unreferenced unresolved symbol, this must link
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj undef.obj");
  T_Ok(g_last_exit_code != 0);
}

T_BeginTest(weak_lib_vs_weak_lib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from a.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x111;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_lib_vs_weak_nolib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_lib_vs_weak_alias)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchAlias, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

T_BeginTest(weak_lib_vs_weak_antidep)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from a.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_alias_vs_weak_alias)
{
  String8 a;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *qwe = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("qwe"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("sym"), COFF_WeakExt_SearchAlias, qwe);
    a = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 b;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *ewq = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("ewq"), 0x222, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("sym"), COFF_WeakExt_SearchAlias, ewq);
    b = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("sym"));
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a));
  T_Ok(t_write_file(str8_lit("b.obj"), b));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

T_BeginTest(weak_alias_vs_weak_lib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchAlias, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_alias_vs_weak_nolib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchAlias, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_alias_vs_weak_antidep)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchAlias, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_nolib_vs_weak_nolib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_nolib_vs_weak_lib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_nolib_vs_weak_alias)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchAlias, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

T_BeginTest(weak_nolib_vs_weak_antidep)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_antidep_vs_weak_antidep)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  // linker must pick weak symbol from a.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x111;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_antidep_vs_weak_nolib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_NoLibrary, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_antidep_vs_weak_lib)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_antidep_vs_weak_alias)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("q"), 0x111, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchAlias, q);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *e = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("e"), 0x222, COFF_SymStorageClass_External);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_AntiDependency, e);
    coff_obj_writer_section_push_reloc_addr32(obj_writer, sect, 3, sym);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x111;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

T_BeginTest(weak_vs_common)
{
  String8 weak_obj;
  {
    COFF_ObjWriter  *obj_writer  = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect        = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS, str8_lit("a"));
    COFF_ObjSymbol  *sect_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("_a"), 0, sect);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("w"), COFF_WeakExt_SearchLibrary, sect_symbol);
    weak_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }
    
  String8 common_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_common(obj_writer, str8_lit("w"), 2);
    common_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("w"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("weak.obj"), weak_obj));
  T_Ok(t_write_file(str8_lit("common.obj"), common_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe common.obj weak.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe weak.obj common.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *bss = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".bss"));
  T_Ok(bss);
  T_Ok(bss->fsize == 0);
  T_Ok(bss->vsize == 2);
}

T_BeginTest(abs_vs_weak)
{
  U32 abs_value   = 0x123;
  U8  text_code[] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3 };

  String8 abs_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("foo"), abs_value, COFF_SymStorageClass_External);
    abs_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 text_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    COFF_ObjSection *mydata = coff_obj_writer_push_section(obj_writer, str8_lit(".mydata"), COFF_SectionFlag_CntCode|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute|COFF_SectionFlag_Align1Bytes, str8_lit("mydata"));
    COFF_ObjSymbol  *tag    = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("mydata"), 0, mydata);
    COFF_ObjSymbol  *foo    = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("foo"), COFF_WeakExt_NoLibrary, tag);

    COFF_ObjSection *text = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), COFF_SectionFlag_CntCode|COFF_SectionFlag_MemExecute|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes, str8_array_fixed(text_code));
    coff_obj_writer_section_push_reloc(obj_writer, text, 2, foo, COFF_Reloc_X64_Addr64);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text);

    text_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("abs.obj"),  abs_obj));
  T_Ok(t_write_file(str8_lit("text.obj"), text_obj));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj text.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe text.obj abs.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_section != 0);

  String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + text_section->fsize));
  String8 inst      = str8_prefix(text_data, 2);
  T_Ok(str8_match(inst, str8_array(text_code, 2), 0));

  String8 imm          = str8_prefix(str8_skip(text_data, 2), 8);
  U64     expected_imm = abs_value;
  T_Ok(str8_match(imm, str8_struct(&expected_imm), 0));
}

T_BeginTest(abs_vs_regular)
{
  String8 shared_symbol_name = str8_lit("foo");

  U8 regular_payload[] = { 0xC0, 0xFF, 0xEE };
  String8 regular_obj_name = str8_lit("regular.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(regular_payload));
    coff_obj_writer_push_symbol_extern(obj_writer, shared_symbol_name, 0, data_sect);
    String8 regular_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(regular_obj_name, regular_obj));
  }

  String8 abs_obj_name = str8_lit("abs.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, shared_symbol_name, 0x1234, COFF_SymStorageClass_External);
    String8 abs_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(abs_obj_name, abs_obj));
  }

  U8 entry_text[] = { 
    0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, $imm
    0xC3 // ret
  };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    COFF_ObjSymbol *shared_symbol = coff_obj_writer_push_symbol_undef(obj_writer, shared_symbol_name);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 3, shared_symbol, COFF_Reloc_X64_Addr32Nb);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  // TODO: validate that linker issues multiply defined symbol error
  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj regular.obj entry.obj");
  // linker should complain about multiply defined symbol
  T_Ok(g_last_exit_code != 0);

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe regular.obj abs.obj entry.obj");
  // linker should complain even in case regular is before abs
  T_Ok(g_last_exit_code != 0);
}

T_BeginTest(abs_vs_common)
{
  String8 shared_symbol_name = str8_lit("foo");

  String8 common_obj_name = str8_lit("common.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_common(obj_writer, shared_symbol_name, 321);
    String8 common_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(common_obj_name, common_obj));
  }

  String8 abs_obj_name = str8_lit("abs.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, shared_symbol_name, 0x1234, COFF_SymStorageClass_External);
    String8 abs_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(abs_obj_name, abs_obj));
  }

  U8 entry_text[] = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj common.obj entry.obj");
  if (g_last_exit_code == 0) {
    // TODO: validate that linker issues multiply defined symbol error
    t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe common.obj abs.obj entry.obj");
    if (t_id_linker() == T_Linker_RAD) {
      T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
    } else {
      T_Ok(g_last_exit_code != 0);
    }
  }
}

T_BeginTest(abs_vs_abs)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("foo"), 'a', COFF_SymStorageClass_External);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 b_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("foo"), 'b', COFF_SymStorageClass_External);
    b_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), b_obj));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

T_BeginTest(undef_weak_lib)
{
  String8 weak;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    COFF_ObjSymbol *b = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("b"), 0xc3000000, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("a"), COFF_WeakExt_SearchLibrary, b);
    weak = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    COFF_ObjSymbol *sym = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("a"));
    U8 text[4] = {0};
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, sym);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    entry = coff_obj_writer_serialize(arena, obj_writer);
  }

  T_Ok(t_write_file(str8_lit("weak.obj"), weak));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry));

  // undefined symbol must always replace weak symbol with search library
  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:entry entry.obj weak.obj");
  T_Ok(g_last_exit_code == LNK_Error_UnresolvedSymbol);
}

T_BeginTest(undef_weak_search_alias)
{
  Temp scratch = scratch_begin(0,0);

  String8 weak_obj;
  {
    U8 weak_payload[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *weak_sect = t_push_data_section(obj_writer, str8_array_fixed(weak_payload));
    COFF_ObjSymbol *tag = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("ptr"));
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("foo"), COFF_WeakExt_SearchAlias, tag);
    weak_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 ptr_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *tag = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("entry"));
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("ptr"), COFF_WeakExt_SearchAlias, tag);
    ptr_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 undef_obj;
  {
    U8 undef_obj_payload[] = { 0x00, 0x00, 0x00, 0x00 };
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *undef_sect = t_push_data_section(obj_writer, str8_array_fixed(undef_obj_payload));
    COFF_ObjSymbol *undef_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("foo"));
    coff_obj_writer_section_push_reloc(obj_writer, undef_sect, 0, undef_symbol, COFF_Reloc_X64_Addr32Nb);
    undef_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    U8 entry_payload[] = {0xC3};
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_payload));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, text_sect);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("weak.obj"), weak_obj));
  T_Ok(t_write_file(str8_lit("ptr.obj"), ptr_obj));
  T_Ok(t_write_file(str8_lit("undef.obj"), undef_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe weak.obj entry.obj ptr.obj undef.obj");
  T_Ok(g_last_exit_code == 0);
}

T_BeginTest(weak_cycle)
{
  String8 ab_obj_name = str8_lit("ab.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *b = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("B"));
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("A"), COFF_WeakExt_SearchAlias, b);
    String8 ab_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(ab_obj_name, ab_obj));
  }

  String8 ba_obj_name = str8_lit("ba.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *a = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("A"));
    coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("B"), COFF_WeakExt_SearchAlias, a);
    String8 ba_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(ba_obj_name, ba_obj));
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  U8 entry_payload[] = { 0xC3 };
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_payload));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  U64 timeout = os_now_microseconds() + 3 * 1000 * 1000; // give a generous 3 seconds
  t_invoke_linker_timeoutf(timeout, "/subsystem:console /entry:my_entry %S %S %S", entry_obj_name, ab_obj_name, ba_obj_name);
}

T_BeginTest(weak_tag)
{
  U32     weak_tag_expected_value = 0x12345678;
  String8 weak_tag_obj_name       = str8_lit("weak_tag.obj");
  {
    COFF_ObjWriter *obj_writer  = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *tag_symbol  = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("abs"), weak_tag_expected_value, COFF_SymStorageClass_Static);
    COFF_ObjSymbol *weak_first  = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("strong_first"), COFF_WeakExt_SearchAlias, tag_symbol);
    COFF_ObjSymbol *weak_second = coff_obj_writer_push_symbol_weak(obj_writer, str8_lit("strong_second"), COFF_WeakExt_SearchAlias, weak_first);

    U8 sect_data[4] = {0};
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_array_fixed(sect_data));
    coff_obj_writer_section_push_reloc(obj_writer, sect, 0, weak_second, COFF_Reloc_X64_Addr32);

    String8 weak_tag_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(weak_tag_obj_name, weak_tag_obj));
  }

  String8 entry_name     = str8_lit("my_entry");
  U8      entry_text[]   = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter  *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect  = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, entry_name, 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe %S %S", weak_tag_obj_name, entry_obj_name);
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *data_section  = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  String8             data          = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
  T_Ok(data_section);
  T_Ok(data_section->vsize == 4);
  T_Ok(str8_match(data, str8_struct(&weak_tag_expected_value), 0));
}

T_BeginTest(undef_section)
{
  String8 main_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    U8 data[] = { 0, 0, 0, 0 };
    COFF_ObjSection *data_section = t_push_data_section(obj_writer, str8_array_fixed(data));
    COFF_ObjSymbol  *foo          = coff_obj_writer_push_symbol_undef_sect(obj_writer, str8_lit(".mysect"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead);
    coff_obj_writer_section_push_reloc(obj_writer, data_section, 0, foo, COFF_Reloc_X64_Addr32Nb);

    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_section = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_section);

    main_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  U8 payload[] = { 1, 2, 3 };
  String8 sec_defn_obj = t_make_sec_defn_obj(arena, str8_array_fixed(payload));

  t_write_file(str8_lit("main.obj"), main_obj);
  t_write_file(str8_lit("sec_defn.obj"), sec_defn_obj);

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj sec_defn.obj");
  if (g_last_exit_code == 0) {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);

    COFF_SectionHeader *data_section   = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    COFF_SectionHeader *mysect_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".mysect"));
    if (data_section && mysect_section) {
      if (data_section->vsize == 4 && mysect_section->vsize == 3) {
        String8 addr32nb = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
        String8 expected_voff = str8_struct(&mysect_section->voff);
        T_Ok(str8_match(addr32nb, expected_voff, 0));
      }
    }
  }
}

T_BeginTest(sect_symbol)
{
  String8 sect_payload = str8_lit("hello, world");
  String8 sect_obj_name = str8_lit("sect.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".mysect$1"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, sect_payload);
    coff_obj_writer_push_directive(obj_writer, str8_lit("/merge:.mysect=.data"));
    String8 sect_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(sect_obj_name, sect_obj));
  }

  String8 main_obj_name = str8_lit("main.obj");
  {
    U8 data[8] = {0};
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_array_fixed(data));
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef_sect(obj_writer, str8_lit(".mysect$2222"), PE_DATA_SECTION_FLAGS);
    coff_obj_writer_section_push_reloc_addr(obj_writer, sect, 0, symbol);

    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);

    String8 main_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);

    T_Ok(t_write_file(main_obj_name, main_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj sect.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *sect          = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));

  T_Ok(sect != 0);

  String8 sect_data = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));

  String8 addr_data = str8_substr(sect_data, rng_1u64(0, sizeof(U64)));
  T_Ok(addr_data.size == sizeof(U64));

  U64 addr = *(U64 *)addr_data.str;
  T_Ok(addr - (pe.image_base + sect->voff) == 8);

  String8 payload_got = str8_substr(sect_data, rng_1u64(8, sect_data.size));
  T_Ok(str8_match(payload_got, sect_payload, 0));
}

T_BeginTest(undef_reloc_section)
{
  String8 main_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_section = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    COFF_ObjSymbol *my_entry_symbol = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_section);

    U8 data[8] = { 0 };
    COFF_ObjSection *data_section = t_push_data_section(obj_writer, str8_array_fixed(data));
    COFF_ObjSymbol  *foo          = coff_obj_writer_push_symbol_undef_sect(obj_writer, str8_lit(".reloc"), PE_RELOC_SECTION_FLAGS);
    coff_obj_writer_section_push_reloc(obj_writer, data_section, 0, foo, COFF_Reloc_X64_Addr64);

    main_obj = coff_obj_writer_serialize(arena, obj_writer);

    coff_obj_writer_release(&obj_writer);
  }

  U8 payload[] = { 1, 2, 3 };
  String8 sec_defn_obj = t_make_sec_defn_obj(arena, str8_array_fixed(payload));

  T_Ok(t_write_file(str8_lit("main.obj"), main_obj));
  T_Ok(t_write_file(str8_lit("sec_defn.obj"), sec_defn_obj));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj sec_defn.obj");
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_SectRefsDiscardedMemory);
  } else {
    T_Ok(g_last_exit_code != 0);
  }
}

T_BeginTest(find_merged_pdata)
{
  U8 foobar_payload[] = {
    0x40, 0x57, 0x48, 0x81, 0xEC, 0x00, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x33, 0xC4, 0x48, 0x89, 0x84, 0x24, 0xF0, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x04, 0x24, 0x48,
    0x8B, 0xF8, 0x33, 0xC0, 0xB9, 0xEC, 0x01, 0x00, 0x00, 0xF3, 0xAA, 0xB8, 0x04, 0x00, 0x00, 0x00,
    0x48, 0x6B, 0xC0, 0x02, 0x8B, 0x04, 0x04, 0x48, 0x8B, 0x8C, 0x24, 0xF0, 0x01, 0x00, 0x00, 0x48,
    0x33, 0xCC, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x81, 0xC4, 0x00, 0x02, 0x00, 0x00, 0x5F, 0xC3,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0x48, 0x83, 0xEC, 0x28, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3      
  };
  U8 xdata_payload[] = {
    0x19, 0x1B, 0x03, 0x00, 0x09, 0x01, 0x40, 0x00, 0x02, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x01, 0x00, 0x00, 0x01, 0x04, 0x01, 0x00, 0x04, 0x42, 0x00, 0x00
  };
  PE_IntelPdata intel_pdata = {0};
  U8 text_payload[]  = { 0xC3 };

  String8 main_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *xdata  = coff_obj_writer_push_section(obj_writer, str8_lit(".xdata"),  COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_Align4Bytes, str8_array_fixed(xdata_payload));
    COFF_ObjSection *pdata  = coff_obj_writer_push_section(obj_writer, str8_lit(".pdata"),  COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_Align4Bytes, str8_struct(&intel_pdata));
    COFF_ObjSection *foobar = coff_obj_writer_push_section(obj_writer, str8_lit(".foobar"), COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute|COFF_SectionFlag_CntCode|COFF_SectionFlag_Align1Bytes, str8_array_fixed(foobar_payload));
    COFF_ObjSection *text   = coff_obj_writer_push_section(obj_writer, str8_lit(".text"),   COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute|COFF_SectionFlag_CntCode|COFF_SectionFlag_Align1Bytes, str8_array_fixed(text_payload));

    COFF_ObjSymbol *foobar_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("foobar"), 0, foobar);

    coff_obj_writer_push_symbol_secdef(obj_writer, xdata, COFF_ComdatSelect_Null);
    COFF_ObjSymbol *unwind_foobar = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("$unwind$foobar"), 0, xdata);

    coff_obj_writer_push_symbol_secdef(obj_writer, pdata, COFF_ComdatSelect_Null);
    coff_obj_writer_push_symbol_static(obj_writer, str8_lit("$pdata$foobar"), 0, pdata);

    coff_obj_writer_section_push_reloc(obj_writer, pdata, OffsetOf(PE_IntelPdata, voff_unwind_info),   unwind_foobar, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, pdata, OffsetOf(PE_IntelPdata, voff_first),         foobar_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, pdata, OffsetOf(PE_IntelPdata, voff_one_past_last), foobar_symbol, COFF_Reloc_X64_Addr32Nb);

    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text);
    main_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("main.obj"), main_obj));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj /merge:.pdata=.rdata");
  T_Ok(g_last_exit_code == 0);

  String8    exe = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo pe  = pe_bin_info_from_data(arena, exe);
  T_Ok(dim_1u64(pe.data_dir_franges[PE_DataDirectoryIndex_EXCEPTIONS]) == 0xC);
}

T_BeginTest(section_sort)
{
  String8 data_obj_name = str8_lit("data.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    COFF_SectionFlags data_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes;
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$z"), data_flags, str8_lit("five"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$a"), data_flags, str8_lit("three"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$bbbbb"), data_flags, str8_lit("four"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data$"), data_flags, str8_lit("two"));
    coff_obj_writer_push_section(obj_writer, str8_lit(".data"), data_flags, str8_lit("one"));

    String8 data_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(data_obj_name, data_obj));
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  U8 entry_text[] = { 0xC3 };
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe data.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *data_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  T_Ok(data_section);

  String8 data = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
  String8 expected_data = str8_lit("onetwothreefourfive");
  T_Ok(str8_match(data, expected_data, 0));
}

T_BeginTest(flag_conf)
{
  COFF_SectionFlags my_sect0_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute;
  COFF_SectionFlags my_sect1_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
  String8 conf_obj_name = str8_lit("conf.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *a_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".mysect"), my_sect0_flags, str8_lit("one"));
    COFF_ObjSection *b_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".mysect"), my_sect1_flags, str8_lit("two"));
    String8 conf_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(conf_obj_name, conf_obj));
  }

  U8 entry_text[] = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe conf.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeaderArray my_sects = coff_section_header_array_from_name(arena, string_table, section_table, pe.section_count, str8_lit(".mysect"));
  T_Ok(my_sects.count == 2);

  COFF_SectionHeader *my_sect0 = &my_sects.v[0];
  COFF_SectionHeader *my_sect1 = &my_sects.v[1];
  T_Ok(my_sect0->flags == my_sect0_flags);
  T_Ok(my_sect1->flags == my_sect1_flags);
}

T_BeginTest(invalid_bss)
{
  COFF_SectionFlags bss_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead;
  String8 bss_obj_name = str8_lit("bss.obj");
  String8 bss_data = str8_lit("Hello, World");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_section(obj_writer, str8_lit(".bss"), bss_flags, bss_data);
    String8 bss_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(bss_obj_name, bss_obj));
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe bss.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *bss_sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".bss"));
  T_Ok(bss_sect != 0);
  T_Ok(bss_sect->vsize == 0xC);
  T_Ok(bss_sect->flags == bss_flags);
  String8 data = str8_substr(exe, rng_1u64(bss_sect->foff, bss_sect->foff + bss_sect->vsize));
  T_Ok(str8_match(data, bss_data, 0));
}

T_BeginTest(common_block)
{
  String8 a_obj_name = str8_lit("a.obj");
  U8 a_data[6] = {0};
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_common(obj_writer, str8_lit("A"), 3);
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(a_data));
    data_sect->flags |= COFF_SectionFlag_Align1Bytes;
    coff_obj_writer_push_section(obj_writer, str8_lit(".bss"), PE_BSS_SECTION_FLAGS, str8(0, 1)); // shift common block's initial position
    coff_obj_writer_section_push_reloc(obj_writer, data_sect, 0, symbol, COFF_Reloc_X64_Addr32);
    String8 a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(a_obj_name, a_obj));
  }

  String8 b_obj_name = str8_lit("b.obj");
  U8 b_data[9] = { 0 };
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(b_data));
    data_sect->flags |= COFF_SectionFlag_Align1Bytes;
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_common(obj_writer, str8_lit("B"), 6);
    coff_obj_writer_section_push_reloc(obj_writer, data_sect, 0, symbol, COFF_Reloc_X64_Addr64);
    String8 b_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(b_obj_name, b_obj));
  }

  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe /fixed /largeaddressaware:no /merge:.bss=.comm a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  COFF_SectionHeader *comm_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".comm"));
  COFF_SectionHeader *data_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  T_Ok(comm_sect != 0);
  T_Ok(data_sect != 0);

  // blocks must be sorted in descending order to reduce alignment padding
  T_Ok(comm_sect->vsize == 0x13);

  // ensure linker correctly patched addresses for symbols pointing into common block
  String8             data      = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->fsize));
  U32                *a_addr    = (U32 *)data.str;
  U64                *b_addr    = (U64 *)(data.str + sizeof(a_data));
  T_Ok(*a_addr == (pe.image_base + comm_sect->voff + 0x10));
  T_Ok(*b_addr == (pe.image_base + comm_sect->voff + 0x8));
}

T_BeginTest(base_relocs)
{
  // main.obj
  String8 entry_name = str8_lit("my_entry");
  U64 mov_func_name64 = 2;
  U64 mov_func_name32 = 16;
  U8 main_text[] = {
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov rax, func_name
    0xff, 0xd0,                                                  // call rax
    0x48, 0x31, 0xc0,                                            // xor rax, rax
    0xb8, 0x00, 0x00, 0x00, 0x00,                                // mov eax, func_name
    0xff, 0xd0,                                                  // call rax
    0xc3                                                         // ret
  };

  // func.obj
  String8 func_name   = str8_lit("foo");
  U8      func_text[] = { 0xc3 };

  String8 main_obj_name = str8_lit("main.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect  = t_push_text_section(obj_writer, str8_array_fixed(main_text));
    COFF_ObjSymbol  *func_undef = coff_obj_writer_push_symbol_undef(obj_writer, func_name);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, mov_func_name64, func_undef, COFF_Reloc_X64_Addr64);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, mov_func_name32, func_undef, COFF_Reloc_X64_Addr32);

    // linker must not produce base relocations for absolute symbol
    U8 data[4] = {0};
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(data));
    COFF_ObjSymbol *abs_symbol = coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("abs"), 0x12345678, COFF_SymStorageClass_Static);
    coff_obj_writer_section_push_reloc(obj_writer, data_sect, 0, abs_symbol, COFF_Reloc_X64_Addr32);

    coff_obj_writer_push_symbol_extern(obj_writer, entry_name, 0, text_sect);
    String8 main_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(main_obj_name, main_obj));
  }

  String8 func_obj_name = str8_lit("func.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(func_text));
    coff_obj_writer_push_symbol_extern(obj_writer, func_name, 0, text_sect);
    String8 func_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(func_obj_name, func_obj));
  }

  String8 out_name = str8_lit("a.exe");
  t_invoke_linkerf("/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe main.obj func.obj");
  T_Ok(g_last_exit_code == 0);

  // it is illegal to merge .reloc with other sections
  t_invoke_linkerf("/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe /merge:.reloc=.rdata main.obj func.obj");
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  } else {
    T_Ok(g_last_exit_code != 0);
  }

  // the other way around is illegal too
  t_invoke_linkerf("/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe /merge:.rdata=.reloc main.obj func.obj");
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  } else {
    T_Ok(g_last_exit_code != 0);
  }
}

T_BeginTest(simple_lib_test)
{
  String8 test_payload = str8_lit("The quick brown fox jumps over the lazy dog");
  String8 test_obj = {0};
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_Unknown);
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8(test_payload.str, test_payload.size+1));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("test"), 0, data_sect);
    test_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 test_lib_name = str8_lit("test.lib");
  {
    COFF_LibWriter *lib_writer = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(lib_writer, str8_lit("test.obj"), test_obj);
    String8 test_lib = coff_lib_writer_serialize(arena, lib_writer, 0, 0, 1);
    coff_lib_writer_release(&lib_writer);
    T_Ok(t_write_file(test_lib_name, test_lib));
  }

  U8 entry_text[] = {
    0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0xC3
  };
  String8 entry_obj_name = str8_lit("entry.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(entry_text));
    COFF_ObjSymbol *test_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("test"));
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 3, test_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 7, text_sect);
    String8 entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(entry_obj_name, entry_obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe entry.obj test.lib");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *text_sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  COFF_SectionHeader *data_sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));

  String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->fsize));
  String8 data_data = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->fsize));

  // was test payload linked?
  String8 data_string = str8_cstring_capped(data_data.str, data_data.str + data_data.size);
  T_Ok(str8_match(data_string, test_payload, 0));

  // do we have enough bytes to read text?
  T_Ok(text_data.size >= sizeof(entry_text));

  // linker must pull-in test.obj and patch relocation for "test" symbol
  U32 *data_addr32nb = (U32 *)(text_data.str+3);
  T_Ok(*data_addr32nb == data_sect->voff);
}

T_BeginTest(import_export)
{
  String8 export_obj;
  {
    String8 export_obj_name    = str8_lit("export.obj");
    String8 export_obj_payload = str8_lit("test");
    U8      export_text[]      = { 
      0x90,                                     // 0: nop
      0x54,                                     // 1: push rsp
      0x48, 0xc7, 0xc0, 0x21, 0x03, 0x00, 0x00, // 2: mov rax, 0x321
      0x5c,                                     // 9: pop rsp
      0xc3                                      // 10: ret
    };

    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, export_obj_payload);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("foo"),  0, data_sect);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("ord"),  1, data_sect);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("ord2"), 2, data_sect);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("ord3"), 9, data_sect);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("ord4"), 10, data_sect);
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:foo=foo"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:bar=foo"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:ord,@5"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:ord2,@6,DATA"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:ord3,@7,NONAME,PRIVATE"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:ord4,@8,NONAME,DATA"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:baz=BAZ.qwe"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:baf=BAZ.#1"));
    export_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 import_obj;
  {
    String8 import_obj_name = str8_lit("import.obj");
    U8 import_payload[1024] = {0};

    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *data_sect = t_push_data_section(obj_writer, str8_array_fixed(import_payload));

    char *import_symbols[] = {
      "__imp_foo",
      "__imp_bar",
      "__imp_baz",
      "__imp_baf",
      "__imp_ord",
      //"__imp_ord2",
      //"__imp_ord4",
      "bar",
      //"baf",
      //"baz",
      "foo",
      "ord",
    };

    for EachElement(i, import_symbols) {
      COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_cstring(import_symbols[i]));
      coff_obj_writer_section_push_reloc_voff(obj_writer, data_sect, i * 4, symbol);
    }

    import_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 baz_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *s1 = coff_obj_writer_push_section(obj_writer, str8_lit(".s1"), PE_DATA_SECTION_FLAGS, str8_lit("s1"));
    COFF_ObjSection *s2 = coff_obj_writer_push_section(obj_writer, str8_lit(".s2"), PE_DATA_SECTION_FLAGS, str8_lit("s2"));
    COFF_ObjSection *text_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed((U8[]){ 0xC3 }));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("_DllMainCRTStartup"), 0, text_sect);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("s1"), 0, s1);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("s2"), 0, s2);
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:baf=s1"));
    coff_obj_writer_push_directive(obj_writer, str8_lit("/export:baz=s2"));
    baz_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  // write objs
  T_Ok(t_write_file(str8_lit("export.obj"), export_obj));
  T_Ok(t_write_file(str8_lit("import.obj"), import_obj));
  T_Ok(t_write_file(str8_lit("baz.obj"), baz_obj));
  T_Ok(t_write_entry_obj());
  
  // link dlls
  t_invoke_linkerf("/dll /out:export.dll libcmt.lib export.obj"); // export.dll
  T_Ok(g_last_exit_code == 0);
  t_invoke_linkerf("/dll /out:baz.dll /export:s1,@1,NONAME /export:qwe=s2 baz.obj"); // baz.dll
  T_Ok(g_last_exit_code == 0);

  // validate export table in export.dll
  if (t_id_linker() == T_Linker_RAD) {
    // validate export table in export.dll
    {
      String8              dll           = t_read_file(arena, str8_lit("export.dll"));
      PE_BinInfo           pe            = pe_bin_info_from_data(arena, dll);
      COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(dll, pe.section_table_range).str;
      PE_ParsedExportTable export_table  = pe_exports_from_data(arena, pe.section_count, section_table, dll, pe.data_dir_franges[PE_DataDirectoryIndex_EXPORT], pe.data_dir_vranges[PE_DataDirectoryIndex_EXPORT]);
      COFF_SectionHeader  *data_sect     = coff_section_header_from_name(str8_zero(), section_table, pe.section_count, str8_lit(".data"));

      // validate header
      T_Ok(export_table.flags == 0);
      T_Ok(export_table.time_stamp == COFF_TimeStamp_Max);
      T_Ok(export_table.major_ver == 0);
      T_Ok(export_table.minor_ver == 0);
      T_Ok(export_table.ordinal_base == 5);
      T_Ok(export_table.export_count == 8);

      // validate names
      T_Ok(str8_match(export_table.exports[0].name, str8_lit("baf"), 0));
      T_Ok(str8_match(export_table.exports[1].name, str8_lit("bar"), 0));
      T_Ok(str8_match(export_table.exports[2].name, str8_lit("baz"), 0));
      T_Ok(str8_match(export_table.exports[3].name, str8_lit("foo"), 0));
      T_Ok(str8_match(export_table.exports[4].name, str8_lit("ord"), 0));
      T_Ok(str8_match(export_table.exports[5].name, str8_lit("ord2"), 0));
      T_Ok(export_table.exports[6].name.size == 0);
      T_Ok(export_table.exports[7].name.size == 0);

      // validate forwarders
      T_Ok(str8_match(export_table.exports[0].forwarder, str8_lit("BAZ.#1"), 0));
      T_Ok(export_table.exports[1].forwarder.size == 0);
      T_Ok(str8_match(export_table.exports[2].forwarder, str8_lit("BAZ.qwe"), 0));
      T_Ok(export_table.exports[3].forwarder.size == 0);
      T_Ok(export_table.exports[4].forwarder.size == 0);
      T_Ok(export_table.exports[5].forwarder.size == 0);
      T_Ok(export_table.exports[6].forwarder.size == 0);
      T_Ok(export_table.exports[7].forwarder.size == 0);

      // validate voffs
      T_Ok(export_table.exports[1].voff == data_sect->voff + 0x0);
      T_Ok(export_table.exports[3].voff == data_sect->voff + 0x0);
      T_Ok(export_table.exports[4].voff == data_sect->voff + 0x1);
      T_Ok(export_table.exports[5].voff == data_sect->voff + 0x2);
      T_Ok(export_table.exports[6].voff == data_sect->voff + 0x9);
      T_Ok(export_table.exports[7].voff == data_sect->voff + 0xa);

      // validate ordinals
      T_Ok(export_table.exports[0].ordinal == 9);
      T_Ok(export_table.exports[1].ordinal == 10);
      T_Ok(export_table.exports[2].ordinal == 11);
      T_Ok(export_table.exports[3].ordinal == 12);
      T_Ok(export_table.exports[4].ordinal == 5);
      T_Ok(export_table.exports[5].ordinal == 6);
      T_Ok(export_table.exports[6].ordinal == 7);
      T_Ok(export_table.exports[7].ordinal == 8);
    }

    // validate export table in baz.dll
    {
      String8              dll           = t_read_file(arena, str8_lit("baz.dll"));
      PE_BinInfo           pe            = pe_bin_info_from_data(arena, dll);
      COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(dll, pe.section_table_range).str;
      PE_ParsedExportTable export_table  = pe_exports_from_data(arena, pe.section_count, section_table, dll, pe.data_dir_franges[PE_DataDirectoryIndex_EXPORT], pe.data_dir_vranges[PE_DataDirectoryIndex_EXPORT]);

      // validate header
      T_Ok(export_table.flags == 0);
      T_Ok(export_table.time_stamp == COFF_TimeStamp_Max);
      T_Ok(export_table.major_ver == 0);
      T_Ok(export_table.minor_ver == 0);
      T_Ok(export_table.ordinal_base == 1);
      T_Ok(export_table.export_count == 4);

      // validate names
      T_Ok(str8_match(export_table.exports[0].name, str8_lit("baf"), 0));
      T_Ok(str8_match(export_table.exports[1].name, str8_lit("baz"), 0));
      T_Ok(str8_match(export_table.exports[2].name, str8_lit("qwe"), 0));
      T_Ok(str8_match(export_table.exports[3].name, str8_zero(), 0));

      // validate forwarders
      T_Ok(str8_match(export_table.exports[0].forwarder, str8_zero(), 0));
      T_Ok(str8_match(export_table.exports[1].forwarder, str8_zero(), 0));
      T_Ok(str8_match(export_table.exports[2].forwarder, str8_zero(), 0));
      T_Ok(str8_match(export_table.exports[3].forwarder, str8_zero(), 0));

      // validate voffs
      T_Ok(export_table.exports[0].voff == 0x3000);
      T_Ok(export_table.exports[1].voff == 0x4000);
      T_Ok(export_table.exports[2].voff == 0x4000);
      T_Ok(export_table.exports[3].voff == 0x3000);

      // validate ordinals
      T_Ok(export_table.exports[0].ordinal == 2);
      T_Ok(export_table.exports[1].ordinal == 3);
      T_Ok(export_table.exports[2].ordinal == 4);
      T_Ok(export_table.exports[3].ordinal == 1);
    }

  }

  #if OS_WINDOWS
  {
    T_Ok(SetDllDirectoryA((LPCSTR)g_wdir.str));
    HANDLE export_dll = LoadLibrary("export.dll");
    T_Ok(export_dll);

    // test query by function name
    //T_Ok(GetProcAddress(export_dll, "baf"));
    T_Ok(GetProcAddress(export_dll, "bar"));
    //T_Ok(GetProcAddress(export_dll, "baz"));
    T_Ok(GetProcAddress(export_dll, "foo"));
    T_Ok(GetProcAddress(export_dll, "ord"));
    T_Ok(GetProcAddress(export_dll, "ord2"));

    // test query by ordinal
    //T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(9)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(10)));
    //T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(11)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(12)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(5)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(6)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(7)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(8)));
  }
  #endif

  //T_Ok(t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /delayload:export.dll /export:entry kernel32.Lib delayimp.lib libcmt.lib export.lib import.obj entry.obj") == 0);
  // TODO: check import table
}

T_BeginTest(image_base)
{
  String8 obj_name = str8_lit("image_base.obj");
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 
      0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00, // lea rcx, [__ImageBase]
      0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, __ImageBase
      0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, __ImageBase
      0xC3 // ret
    };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    COFF_ObjSymbol *image_base_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("__ImageBase"));
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 3, image_base_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 9, image_base_symbol, COFF_Reloc_X64_Addr64);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 18, image_base_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect); 
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(obj_name, obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /base:0x2000000140000000 /out:a.exe image_base.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *text_section  = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_section);

  U8 expected_text[] = {
    0x48, 0x8D, 0x0D, 0xF9, 0xEF, 0xFF, 0xFF,
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x20,
    0xB8, 0x00, 0x00, 0x00, 0x00,
    0xC3
  };
  String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + sizeof(expected_text)));
  T_Ok(str8_match(text_data, str8_array_fixed(expected_text), 0));
}

T_BeginTest(comdat_any)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".test$mn"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("1"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Any);
    COFF_ObjSymbol *test = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    test->type.u.msb = COFF_SymDType_Func;
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("1.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".test$mn"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("2"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Any);
    COFF_ObjSymbol *test = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("2.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  {
    t_invoke_linkerf("/subsystem:console /entry:entry /out:1.exe 1.obj 2.obj entry.obj");
    T_Ok(g_last_exit_code == 0);
    String8             exe           = t_read_file(arena, str8_lit("1.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".test"));
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("1"), 0));
  }

  {
    t_invoke_linkerf("/subsystem:console /entry:entry /out:2.exe 2.obj 1.obj entry.obj");
    T_Ok(g_last_exit_code == 0);
    String8             exe           = t_read_file(arena, str8_lit("2.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".test"));
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("2"), 0));
  }
}

T_BeginTest(comdat_no_duplicates)
{
  String8 test_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *test_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".test"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("a"));
    coff_obj_writer_push_symbol_secdef(obj_writer, test_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("a"), 0, test_sect);
    test_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("a"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"),     test_obj));
  T_Ok(t_write_file(str8_lit("b.obj"),     test_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol); }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe a.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("b.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".test"));
  T_Ok(sect);
  String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
  T_Ok(str8_match(data, str8_lit("a"), 0));
}

T_BeginTest(comdat_same_size)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("a"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_SameSize);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("a.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".b"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("b"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_SameSize);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("b.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".c"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("cc"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_SameSize);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("c.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("a"), 0));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe a.obj b.obj c.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol); }
}

T_BeginTest(comdat_exact_match)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("a"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_ExactMatch);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("a.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".a2"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("a"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_ExactMatch);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("a2.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".b"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("b"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_ExactMatch);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("b.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code != 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe entry.obj a2.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("b.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a2"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("a"), 0));
  }
}

T_BeginTest(comdat_largest)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("a"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("a.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".b"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("bb"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("b.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".c"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("c"));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("c.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:entry entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *discard_sect  = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    T_Ok(discard_sect == 0);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("bb"), 0));
  }

  t_invoke_linkerf("/subsystem:console /out:b.exe /entry:entry entry.obj c.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("b.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".c"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("c"), 0));
  }
}

T_BeginTest(comdat_associative)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    COFF_ObjSection *a = coff_obj_writer_push_section(obj_writer, str8_lit("a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("a"));
    COFF_ObjSection *aa = coff_obj_writer_push_section(obj_writer, str8_lit("aa"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("aa"));
    coff_obj_writer_push_symbol_secdef(obj_writer, a, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, a);
    coff_obj_writer_push_symbol_associative(obj_writer, aa, a);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("a.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    COFF_ObjSection *bb = coff_obj_writer_push_section(obj_writer, str8_lit("bb"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("bb"));
    COFF_ObjSection *b = coff_obj_writer_push_section(obj_writer, str8_lit("b"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("b"));
    COFF_ObjSection *bbb = coff_obj_writer_push_section(obj_writer, str8_lit("bbb"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("bbb"));
    coff_obj_writer_push_symbol_secdef(obj_writer, bb, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_associative(obj_writer, b, bb);
    coff_obj_writer_push_symbol_associative(obj_writer, bbb, bb);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, bb);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("b.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *a   = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("a"));
  COFF_SectionHeader *aa  = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("aa"));
  COFF_SectionHeader *b   = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("b"));
  COFF_SectionHeader *bb  = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("bb"));
  COFF_SectionHeader *bbb = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("bbb"));
  T_Ok(a == 0);
  T_Ok(aa == 0);
  T_Ok(b != 0);
  T_Ok(bb != 0);
  T_Ok(bbb != 0);
  String8 b_data = str8_substr(exe, rng_1u64(b->foff, b->foff + b->vsize));
  String8 bb_data = str8_substr(exe, rng_1u64(bb->foff, bb->foff + bb->vsize));
  String8 bbb_data = str8_substr(exe, rng_1u64(bbb->foff, bbb->foff + bbb->vsize));
  T_Ok(str8_match(b_data, str8_lit("b"), 0));
  T_Ok(str8_match(bb_data, str8_lit("bb"), 0));
  T_Ok(str8_match(bbb_data, str8_lit("bbb"), 0));
}

T_BeginTest(comdat_associative_loop)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *aaaa = coff_obj_writer_push_section(obj_writer, str8_lit(".aaaa"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("aaaa"));
    COFF_ObjSection *aa   = coff_obj_writer_push_section(obj_writer, str8_lit(".aa"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("aa"));
    COFF_ObjSection *a    = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("a"));
    COFF_ObjSection *aaa  = coff_obj_writer_push_section(obj_writer, str8_lit(".aaa"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT|COFF_SectionFlag_Align1Bytes, str8_lit("aaa"));
    coff_obj_writer_push_symbol_associative(obj_writer, aaa, aa);
    coff_obj_writer_push_symbol_associative(obj_writer, aaaa, aaa);
    coff_obj_writer_push_symbol_associative(obj_writer, a, aa);
    coff_obj_writer_push_symbol_associative(obj_writer, aa, a);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("loop.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe loop.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_AssociativeLoop); }
}

T_BeginTest(comdat_associative_non_comdat)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *a = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS, str8_lit("a"));
    COFF_ObjSection *b = coff_obj_writer_push_section(obj_writer, str8_lit(".b"), PE_DATA_SECTION_FLAGS, str8_lit("b"));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, a);
    coff_obj_writer_push_symbol_associative(obj_writer, b, a);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("test.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj test.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *a             = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
  COFF_SectionHeader *b             = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
  T_Ok(a != 0);
  T_Ok(b != 0);
  String8             a_data        = str8_substr(exe, rng_1u64(a->foff, a->foff + a->vsize));
  String8             b_data        = str8_substr(exe, rng_1u64(b->foff, b->foff + b->vsize));
  T_Ok(str8_match(a_data, str8_lit("a"), 0));
  T_Ok(str8_match(b_data, str8_lit("b"), 0));
}

T_BeginTest(comdat_associative_out_of_bounds)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    COFF_ObjSection *a = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("a"));
    COFF_ObjSection *aa = coff_obj_writer_push_section(obj_writer, str8_lit(".aa"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_lit("aa"));
    coff_obj_writer_push_symbol_secdef(obj_writer, a, COFF_ComdatSelect_Any);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, a);
    coff_obj_writer_push_symbol_associative(obj_writer, aa, a);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    {
      COFF_FileHeaderInfo header = coff_file_header_info_from_data(obj);
      String8 string_table = str8_substr(obj, header.string_table_range);
      String8 symbol_table = str8_substr(obj, header.symbol_table_range);
      COFF_ParsedSymbol symbol = coff_parse_symbol(header, string_table, symbol_table, 3);
      AssertAlways(str8_match(symbol.name, str8_lit(".aa"), 0));
      AssertAlways(symbol.aux_symbol_count == 1);
      COFF_Symbol16 *symbol16 = symbol.raw_symbol;
      COFF_SymbolSecDef *secdef = (COFF_SymbolSecDef *)(symbol16 + 1);
      secdef->number_lo = 321;
    }
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("bad.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj bad.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_IllData); }
}

T_BeginTest(comdat_with_offset)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 a[] = "1Hello, World!";
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(a));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 1, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("a.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 a[] = "Hello, World!";
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(a));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 1, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("b.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 3, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == 0);
}

T_BeginTest(reloc_against_removed_comdat)
{
  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 a[] = "1Hello, World!";
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(a));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 1, sect);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 b_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 a[] = "H";
    COFF_ObjSection *comdat_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(a));
    coff_obj_writer_push_symbol_secdef(obj_writer, comdat_sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 1, comdat_sect);
    COFF_ObjSymbol *static_symbol = coff_obj_writer_push_symbol_static(obj_writer, str8_lit("STATIC"), 2, comdat_sect);

    U8 rdata[4] = {0};
    COFF_ObjSection *regular_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".rdata"), PE_RDATA_SECTION_FLAGS, str8_array_fixed(rdata));
    coff_obj_writer_section_push_reloc_voff(obj_writer, regular_sect, 0, static_symbol);

    b_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 3, symbol);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), b_obj));
  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == LNK_Error_RelocationAgainstRemovedSection);
}

T_BeginTest(sect_align)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    COFF_ObjSection *sect_align_shift = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS, str8_lit("q"));
    COFF_ObjSection *sect_align_none = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS, str8_lit("abc"));
    COFF_ObjSection *sect_align_1 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, str8_lit("wr"));
    COFF_ObjSection *sect_align_2 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, str8_lit("e"));
    COFF_ObjSection *sect_align_4 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_lit("ttttt"));
    COFF_ObjSection *sect_align_8 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align8Bytes, str8_lit("g"));
    COFF_ObjSection *sect_align_16 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align16Bytes, str8_lit("o"));
    COFF_ObjSection *sect_align_32 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align32Bytes, str8_lit("p"));
    COFF_ObjSection *sect_align_64 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align64Bytes, str8_lit("f"));
    COFF_ObjSection *sect_align_128 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align128Bytes, str8_lit("x"));
    COFF_ObjSection *sect_align_256 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align256Bytes, str8_lit("c"));
    COFF_ObjSection *sect_align_512 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align512Bytes, str8_lit("v"));
    COFF_ObjSection *sect_align_1024 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align1024Bytes, str8_lit("b"));
    COFF_ObjSection *sect_align_2048 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align2048Bytes, str8_lit("n"));
    COFF_ObjSection *sect_align_4096 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align4096Bytes, str8_lit("m"));
    COFF_ObjSection *sect_align_8192 = coff_obj_writer_push_section(obj_writer, str8_lit(".a"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align8192Bytes, str8_lit("z"));

    U8 text[] = { 0xC3 };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("my_entry"), 0, text_sect);

    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("test.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe /align:8192 test.obj");

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".a"));
  T_Ok(sect);
  String8 sect_data = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));

  String8 shift = str8_substr(sect_data, rng_1u64(0, 1));
  T_Ok(str8_match(shift, str8_lit("q"), 0));
  String8 a_none = str8_substr(sect_data, rng_1u64(16, 16 + 3));
  T_Ok(str8_match(a_none, str8_lit("abc"), 0));
  String8 a_1 = str8_substr(sect_data, rng_1u64(19, 21));
  T_Ok(str8_match(a_1, str8_lit("wr"), 0));
  String8 a_2 = str8_substr(sect_data, rng_1u64(22, 23));
  T_Ok(str8_match(a_2, str8_lit("e"), 0));
  String8 a_4 = str8_substr(sect_data, rng_1u64(24, 29));
  T_Ok(str8_match(a_4, str8_lit("ttttt"), 0));
  String8 a_8 = str8_substr(sect_data, rng_1u64(32, 33));
  T_Ok(str8_match(a_8, str8_lit("g"), 0));
  String8 a_16 = str8_substr(sect_data, rng_1u64(48, 49));
  T_Ok(str8_match(a_16, str8_lit("o"), 0));
  String8 a_32 = str8_substr(sect_data, rng_1u64(64, 65));
  T_Ok(str8_match(a_32, str8_lit("p"), 0));
  String8 a_64 = str8_substr(sect_data, rng_1u64(128, 129));
  T_Ok(str8_match(a_64, str8_lit("f"), 0));
  String8 a_128 = str8_substr(sect_data, rng_1u64(256, 257));
  T_Ok(str8_match(a_128, str8_lit("x"), 0));
  String8 a_256 = str8_substr(sect_data, rng_1u64(512, 513));
  T_Ok(str8_match(a_256, str8_lit("c"), 0));
  String8 a_512 = str8_substr(sect_data, rng_1u64(1024, 1025));
  T_Ok(str8_match(a_512, str8_lit("v"), 0));
  String8 a_1024 = str8_substr(sect_data, rng_1u64(2048, 2049));
  T_Ok(str8_match(a_1024, str8_lit("b"), 0));
  String8 a_2048 = str8_substr(sect_data, rng_1u64(4096, 4097));
  T_Ok(str8_match(a_2048, str8_lit("n"), 0));
  String8 a_4096 = str8_substr(sect_data, rng_1u64(8192, 8193));
  T_Ok(str8_match(a_4096, str8_lit("m"), 0));
  String8 a_8192 = str8_substr(sect_data, rng_1u64(16384, 16385));
  T_Ok(str8_match(a_8192, str8_lit("z"), 0));
}

T_BeginTest(alt_name)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_lit("test"));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("test"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("test.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_lit("foo"));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("foo"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("foo.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("foo"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  // basic alternate name test
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:foo=test test.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  // linker should not chase alt name links
  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe /alternatename:foo=bar /alternatename:bar=test test.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // alt name conflict
  t_invoke_linkerf("/subsystem:console /entry:entry /out:c.exe /alternatename:foo=test /alternatename:foo=qwe test.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:d.exe /alternatename:foo foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  
  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:e.exe /alternatename:foo-oof foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:foo=test=bar foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:foo= foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:= foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename: foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // TODO: check that RAD Linker prints these warnings

  // warn about alt name to self alt name?
  t_invoke_linkerf("/subsystem:console /entry:entry /out:f.exe /alternatename:foo=foo foo.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  // warn about alt name to unknown symbol?
  t_invoke_linkerf("/subsystem:console /entry:entry /out:g.exe /alternatename:qwe=ewq foo.obj entry.obj");
  T_Ok(g_last_exit_code == 0);
}

T_BeginTest(include)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_lit("foo"));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("foo"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("include.obj"), obj));

    COFF_LibWriter *lib_writer = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(lib_writer, str8_lit("include.obj"), obj);
    String8 lib = coff_lib_writer_serialize(arena, lib_writer, 0, 0, 1);
    coff_lib_writer_release(&lib_writer);
    T_Ok(t_write_file(str8_lit("include.lib"), lib));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  // simple include test
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /include:foo entry.obj include.lib");
  T_Ok(g_last_exit_code == 0);

  // validate that linker pulled-in include.obj
  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *foo_sect      = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(foo_sect != 0);
    String8             foo_data      = str8_substr(exe, rng_1u64(foo_sect->foff, foo_sect->foff + foo_sect->vsize));
    T_Ok(str8_match(foo_data, str8_lit("foo"), 0));
  }

  // test unresolved include
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /include:ewq entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_UnresolvedSymbol); }
}

T_BeginTest(communal_var_vs_regular)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_common(obj_writer, str8_lit("TEST"), 1);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("communal.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_lit("test"));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("defn.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  // linker should replace communal TEST with .data TEST

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe communal.obj defn.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe defn.obj communal.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  char *exes[] = { "a.exe", "b.exe" };
  for EachElement(i, exes) {
    String8             exe           = t_read_file(arena, str8_cstring(exes[i]));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *data_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(data_sect);
    String8             data          = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->vsize));
    T_Ok(str8_match(data, str8_lit("test"), 0));
  }
}

T_BeginTest(communal_var_vs_regular_comdat)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_common(obj_writer, str8_lit("TEST"), 1);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("communal.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *sect = t_push_data_section(obj_writer, str8_lit("test"));
    sect->flags |= COFF_SectionFlag_LnkCOMDAT;
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("large.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    COFF_ObjSymbol *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  // linker should replace communal TEST with .data TEST
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe communal.obj large.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe large.obj communal.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  char *exes[] = { "a.exe", "b.exe" };
  for EachElement(i, exes) {
    String8             exe           = t_read_file(arena, str8_cstring(exes[i]));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *data_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(data_sect);
    String8             data          = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->vsize));
    T_Ok(str8_match(data, str8_lit("test"), 0));
  }
}

T_BeginTest(import_kernel32)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 data[] = "test";
    U8 text[] = {
      0x48, 0x83, 0xec, 0x68,                               // sub  rsp,68h                        ; alloc space on stack
      0xc7, 0x44, 0x24, 0x48, 0x18, 0x00, 0x00, 0x00,       // mov  dword ptr [rsp+48h],18h        ; SECURITY_ATTRIBUTES.nLength
      0x48, 0xc7, 0x44, 0x24, 0x50, 0x00, 0x00, 0x00, 0x00, // mov  qword ptr [rsp+50h],0          ; SECURITY_ATTRIBUTES.lpSecurityDescriptor
      0xc7, 0x44, 0x24, 0x58, 0x00, 0x00, 0x00, 0x00,       // mov  dword ptr [rsp+58h],0          ; SECURITY_ATTRIBUTES.bInheritHandle
      0x48, 0xc7, 0x44, 0x24, 0x30, 0x00, 0x00, 0x00, 0x00, // mov  qword ptr [rsp+30h],0          ; hTemplateFile
      0xc7, 0x44, 0x24, 0x28, 0x80, 0x00, 0x00, 0x00,       // mov  dword ptr [rsp+28h],80h        ; dwFlagsAndAttributes
      0xc7, 0x44, 0x24, 0x20, 0x02, 0x00, 0x00, 0x00,       // mov  dword ptr [rsp+20h],2          ; dwCreationDisposition
      0x4c, 0x8d, 0x4c, 0x24, 0x48,                         // lea  r9,[rsp+48h]                   ; lpSecurityAttributes
      0x45, 0x33, 0xc0,                                     // xor  r8d,r8d                        ; dwShareMode
      0xba, 0x00, 0x00, 0x00, 0x40,                         // mov  edx,40000000h                  ; dwDesiredAccess
      0x48, 0x8d, 0x0d, 0x00, 0x00, 0x00, 0x00,             // lea  rcx,[test]                     ; lpFileName
      0xff, 0x15, 0x00, 0x00, 0x00, 0x00,                   // call qword ptr [__imp_CreateFileA]  ; call CreateFileA
      0x48, 0x89, 0xc1,                                     // mov  rcx,rax                        ; hObject
      0xff, 0x15, 0x00, 0x00, 0x00, 0x00,                   // call qword ptr [__imp_CloseHandle]  ; call CloseHandle
      0x33, 0xc0,                                           // xor  eax,eax                        ; clear result
      0x48, 0x83, 0xc4, 0x68,                               // add  rsp,68h                        ; dealloc stack
      0xc3                                                  // ret                                 ; return
    };
    COFF_ObjSection *data_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".data"), PE_DATA_SECTION_FLAGS, str8_array_fixed(data));
    COFF_ObjSection *text_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    COFF_ObjSymbol *data_symbol = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("test"), 0, data_sect);
    COFF_ObjSymbol *entry_symbol = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, text_sect);
    COFF_ObjSymbol *create_file_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("__imp_CreateFileA"));
    COFF_ObjSymbol *close_handle_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("__imp_CloseHandle"));
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 70, data_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 76, create_file_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 85, close_handle_symbol, COFF_Reloc_X64_Rel32);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("import.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /fixed import.obj kernel32.lib");
  T_Ok(g_last_exit_code == 0);

#if OS_WINDOWS
  {
    String8 test_file_path = push_str8f(arena, "%S/test", g_wdir);
    os_delete_file_at_path(test_file_path);

    OS_ProcessLaunchParams launch_opts = {0};
    launch_opts.inherit_env = 0;
    launch_opts.path = g_wdir;
    str8_list_pushf(arena, &launch_opts.cmd_line, "%S/a.exe", g_wdir);
    OS_Handle handle = os_process_launch(&launch_opts);
    AssertAlways(!os_handle_match(handle, os_handle_zero()));
    U64 exit_code = max_U64;
    os_process_join(handle, max_U64, &exit_code);
    os_process_detach(handle);
    T_Ok(exit_code == 0);
    T_Ok(os_file_path_exists(test_file_path));
  }
#endif
}

T_BeginTest(delay_import)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 return_0[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
      0xc3 // ret
    };
    U8 return_1[] = {
      0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
      0xc3 // ret
    };
    U8 return_2[] = {
      0x48, 0xC7, 0xC0, 0x02, 0x00, 0x00, 0x00, // mov rax, 2
      0xc3 // ret
    };
    COFF_ObjSection *return_0_sect = t_push_text_section(obj_writer, str8_array_fixed(return_0));
    COFF_ObjSection *return_1_sect = t_push_text_section(obj_writer, str8_array_fixed(return_1));
    COFF_ObjSection *return_2_sect = t_push_text_section(obj_writer, str8_array_fixed(return_2));
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("return_1"), 0, return_1_sect);
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("return_2"), 0, return_2_sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("a.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 return_0[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
      0xc3 // ret
    };
    U8 return_123[] = {
      0x48, 0xC7, 0xC0, 0x7B, 0x00, 0x00, 0x00, // mov rax, 123
      0xc3 // ret
    };
    U8 return_321[] = {
      0x48, 0xC7, 0xC0, 0x41, 0x01, 0x00, 0x00, // mov rax, 321 
      0xc3 // ret
    };
    COFF_ObjSection *return_0_sect = t_push_text_section(obj_writer, str8_array_fixed(return_0));
    COFF_ObjSection *return_123_sect = t_push_text_section(obj_writer, str8_array_fixed(return_123));
    COFF_ObjSection *return_321_sect = t_push_text_section(obj_writer, str8_array_fixed(return_321));
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("return_123"), 0, return_123_sect);
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("return_321"), 0, return_321_sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("b.obj"), obj));
  }

  {
    U8 text[] = {
      0x56,                         // push  rsi
      0x57,                         // push  rdi
      0x48, 0x83, 0xEC, 0x28,       // sub   rsp,28h
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_1
      0x89, 0xC6,                   // mov   esi,eax
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_2
      0x89, 0xC7,                   // mov   edi,eax
      0x01, 0xF7,                   // add   edi,esi
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_123
      0x89, 0xC6,                   // mov   esi,eax
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_321
      0x01, 0xF0,                   // add   eax,esi
      0x01, 0xF8,                   // add   eax,edi
      0x48, 0x83, 0xC4, 0x28,       // add   rsp,28h
      0x5F,                         // pop   rdi
      0x5E,                         // pop   rsi
      0xC3,                         // ret
    };
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0,COFF_MachineType_X64);
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, text_sect);
    COFF_ObjSymbol *return_1_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("return_1"));
    COFF_ObjSymbol *return_2_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("return_2"));
    COFF_ObjSymbol *return_123_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("return_123"));
    COFF_ObjSymbol *return_321_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("return_321"));
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 7, return_1_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 14, return_2_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 23, return_123_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 30, return_321_symbol, COFF_Reloc_X64_Rel32);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("main.obj"), obj));
  }

  t_invoke_linkerf("/dll /implib:a.lib /export:return_1 /export:return_2 a.obj libcmt.lib");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/dll /implib:b.lib /export:return_123 /export:return_321 b.obj libcmt.lib");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /fixed /debug:full main.obj a.lib b.lib kernel32.lib delayimp.lib libcmt.lib /delayload:a.dll /delayload:b.dll");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  PE_ParsedDelayImportTable delay_import_table = pe_delay_imports_from_data(arena, pe.is_pe32, pe.section_count, section_table, exe, pe.data_dir_franges[PE_DataDirectoryIndex_DELAY_IMPORT]);

  PE_ParsedDelayDLLImport *a_import = &delay_import_table.v[0];
  T_Ok(a_import->attributes == 1);
  T_Ok(str8_match(a_import->name, str8_lit("a.dll"), 0));
  T_Ok(a_import->module_handle_voff != 0);
  T_Ok(a_import->name_table_voff != 0);
  T_Ok(a_import->bound_table_voff != 0);
  T_Ok(a_import->unload_table_voff != 0);
  T_Ok(a_import->time_stamp == 0);
  T_Ok(a_import->bound_table_count == 2);
  T_Ok(a_import->unload_table_count == 2);
  T_Ok(a_import->import_count == 2);

  PE_ParsedImport *return_1 = &a_import->imports[0];
  T_Ok(return_1->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_1->u.name.string, str8_lit("return_1"), 0));
  T_Ok(return_1->u.name.hint == 0);

  PE_ParsedImport *return_2 = &a_import->imports[1];
  T_Ok(return_2->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_2->u.name.string, str8_lit("return_2"), 0));
  T_Ok(return_2->u.name.hint == 1);

  PE_ParsedDelayDLLImport *b_import = &delay_import_table.v[1];
  T_Ok(b_import->attributes == 1);
  T_Ok(str8_match(b_import->name, str8_lit("b.dll"), 0));
  T_Ok(b_import->module_handle_voff != 0);
  T_Ok(b_import->name_table_voff != 0);
  T_Ok(b_import->bound_table_voff != 0);
  T_Ok(b_import->unload_table_voff != 0);
  T_Ok(b_import->time_stamp == 0);
  T_Ok(b_import->bound_table_count == 2);
  T_Ok(b_import->unload_table_count == 2);
  T_Ok(b_import->import_count == 2);

  PE_ParsedImport *return_123 = &b_import->imports[0];
  T_Ok(return_123->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_123->u.name.string, str8_lit("return_123"), 0));
  T_Ok(return_123->u.name.hint == 0);

  PE_ParsedImport *return_321 = &b_import->imports[1];
  T_Ok(return_321->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_321->u.name.string, str8_lit("return_321"), 0));
  T_Ok(return_321->u.name.hint == 1);
}

T_BeginTest(delay_import_user32)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *data_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".str"), PE_DATA_SECTION_FLAGS, str8_zero());
    U64 msg_off = data_sect->data.total_size;
    str8_list_push(obj_writer->arena, &data_sect->data, push_cstr(arena, str8_lit("test")));
    U64 caption_off = data_sect->data.total_size;
    str8_list_push(obj_writer->arena, &data_sect->data, push_cstr(arena, str8_lit("foo")));
    COFF_ObjSymbol *msg_symbol = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("msg"), msg_off, data_sect);
    COFF_ObjSymbol *caption_symbol = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("caption"), caption_off, data_sect);

    U8 text[] = {
      0x48, 0x83, 0xEC, 0x28,                   // sub  rsp,28h
      0x45, 0x33, 0xC9,                         // xor  r9d,r9d
      0x4C, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, // lea  r8,[msg]
      0x48, 0x8D, 0x15, 0x00, 0x00, 0x00, 0x00, // lea  rdx,[caption]
      0x33, 0xC9,                               // xor  ecx,ecx
      0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,       // call qword ptr [__imp_MessageBoxA]
      0x33, 0xC0,                               // xor  eax,eax
      0x48, 0x83, 0xC4, 0x28,                   // add  rsp,28h
      0xC3,                                     // ret
    };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    COFF_ObjSymbol *text_symbol = coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, text_sect);
    COFF_ObjSymbol *message_box_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("__imp_MessageBoxA"));
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 10, msg_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 17, caption_symbol, COFF_Reloc_X64_Rel32);
    coff_obj_writer_section_push_reloc(obj_writer, text_sect, 25, message_box_symbol, COFF_Reloc_X64_Rel32);

    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("delay_import.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:entry /fixed /delayload:user32.dll kernel32.lib user32.lib libcmt.lib delayimp.lib delay_import.obj /debug:full");
  T_Ok(g_last_exit_code == 0);
}

T_BeginTest(empty_section)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *test = coff_obj_writer_push_section(obj_writer, str8_lit(".test"), PE_TEXT_SECTION_FLAGS, str8(0,0));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, test);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("empty_section.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    COFF_ObjSymbol *test_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, text_sect, 3, test_symbol); // relocation against removed section
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, text_sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe empty_section.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
}

T_BeginTest(removed_section)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 test_text[] = { 0xc3 };
    COFF_ObjSection *test_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".test"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkRemove, str8_array_fixed(test_text));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("TEST"), 0, test_sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("test.obj"), obj));
  }

  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *text_sect = t_push_text_section(obj_writer, str8_array_fixed(text));
    COFF_ObjSymbol *test_symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("TEST"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, text_sect, 3, test_symbol); // relocation against removed section
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, text_sect);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
}

T_BeginTest(function_pad_min)
{
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 ret[] = { 0xc3 };
    COFF_ObjSection *text_sect_0 = t_push_text_section(obj_writer, str8_array_fixed(ret));
    COFF_ObjSection *text_sect_1 = t_push_text_section(obj_writer, str8_array_fixed(ret));
    COFF_ObjSection *text_sect_2 = t_push_text_section(obj_writer, str8_array_fixed(ret));
    text_sect_0->flags |= COFF_SectionFlag_Align4Bytes;
    text_sect_1->flags |= COFF_SectionFlag_Align2Bytes;
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("A"), 0, text_sect_0);
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("B"), 0, text_sect_1);
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("C"), 0, text_sect_2);
    String8 obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
    T_Ok(t_write_file(str8_lit("funcs.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:A /functionpadmin:1 /out:a.exe funcs.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_sect != 0);
  String8             text_data     = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));

  U8 expected_text[] = {
    0xcc, 0xcc, 0xcc, 0xcc, 0xc3, 
    0xcc, 0xcc, 0xcc, 0xc3, 
    0xcc, 0xc3, 
  };
  T_Ok(str8_match(text_data, str8_array_fixed(expected_text), 0));
}

T_BeginTest(first_member_header)
{
  String8 obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("8"), 0x8, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("1"), 0x1, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("9"), 0x9, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("7"), 0x7, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("4"), 0x4, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("5"), 0x5, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("2"), 0x2, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("3"), 0x3, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("6"), 0x6, COFF_SymStorageClass_External);
    obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 lib;
  {
    COFF_LibWriter *lib_writer = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(lib_writer, str8_lit("obj.obj"), obj);
    lib = coff_lib_writer_serialize(arena, lib_writer, 0, 0, 0);
    coff_lib_writer_release(&lib_writer);
  }

  T_Ok(t_write_file(str8_lit("test.lib"), lib));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.lib entry.obj /include:1 /include:2 /include:3 /include:4 /include:5 /include:6 /include:7 /include:8 /include:9");
  T_Ok(g_last_exit_code == 0);
}

T_BeginTest(second_member_header)
{
  String8 obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("8"), 0x8, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("1"), 0x1, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("9"), 0x9, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("7"), 0x7, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("4"), 0x4, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("5"), 0x5, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("2"), 0x2, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("3"), 0x3, COFF_SymStorageClass_External);
    coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("6"), 0x6, COFF_SymStorageClass_External);
    obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 lib;
  {
    COFF_LibWriter *lib_writer = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(lib_writer, str8_lit("obj.obj"), obj);
    lib = coff_lib_writer_serialize(arena, lib_writer, 0, 0, 1);
    coff_lib_writer_release(&lib_writer);
  }

  T_Ok(t_write_file(str8_lit("test.lib"), lib));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.lib entry.obj /include:1 /include:2 /include:3 /include:4 /include:5 /include:6 /include:7 /include:8 /include:9");
  T_Ok(g_last_exit_code == 0);
}

T_BeginTest(defer_impl_link_to_second_search_pass)
{
  String8 imp_ref_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xc3 };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("entry"), 0, sect);
    coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("__imp_foo"));
    coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("func")); // force linker to pull obj from the lib
    imp_ref_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 impl_ref_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = { 0xc3 };
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    coff_obj_writer_push_symbol_extern_func(obj_writer, str8_lit("func"), 0, sect);
    coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("foo")); // on a second search pass force linker to look up same import
    impl_ref_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 impl_ref_lib;
  {
    COFF_LibWriter *lib_writer = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(lib_writer, str8_lit("impl_ref.obj"), impl_ref_obj);
    impl_ref_lib = coff_lib_writer_serialize(arena, lib_writer, 0, 0, 1);
    coff_lib_writer_release(&lib_writer);
  }

  String8 foo_lib;
  {
    COFF_LibWriter *lib_writer = coff_lib_writer_alloc();
    coff_lib_writer_push_import(lib_writer, COFF_MachineType_X64, ~0u, str8_lit("foo.dll"), COFF_ImportBy_Name, str8_lit("foo"), 0, COFF_ImportHeader_Code);
    foo_lib = coff_lib_writer_serialize(arena, lib_writer, 0, 0, 1);
    coff_lib_writer_release(&lib_writer);
  }

  T_Ok(t_write_file(str8_lit("imp_ref.obj"), imp_ref_obj));
  T_Ok(t_write_file(str8_lit("impl_ref.lib"), impl_ref_lib));
  T_Ok(t_write_file(str8_lit("foo.lib"), foo_lib));
  T_Ok(t_write_file(str8_lit("foo2.lib"), foo_lib));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe foo.lib foo2.lib imp_ref.obj impl_ref.lib");
  T_Ok(g_last_exit_code == 0);
}

T_BeginTest(opt_ref_dangling_section)
{
  String8 entry_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    U8 text[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,  // mov rax, $imm
      0xC3 // ret
    };
    COFF_ObjSection *sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    COFF_ObjSymbol  *symbol = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("f"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, symbol);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("entry"), 0, sect);
    entry_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 a_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    U8 data[] = "A0000";
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".data"), PE_DATA_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(data));

    COFF_ObjSymbol *q = coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("q"));
    coff_obj_writer_section_push_reloc_voff(obj_writer, sect, 0, q);

    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("f"), 0, sect);
    a_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 b_obj;
  {
    COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    U8 q[] = { 1,2,3,4};
    COFF_ObjSection *q_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".q"), PE_DATA_SECTION_FLAGS, str8_array_fixed(q));
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("q"), 0, q_sect);

    U8 data[] = "BBBBBBBBBBBBBBB";
    COFF_ObjSection *sect = coff_obj_writer_push_section(obj_writer, str8_lit(".data"), PE_DATA_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(data));
    coff_obj_writer_push_symbol_secdef(obj_writer, sect, COFF_ComdatSelect_Largest);
    coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("f"), 0, sect);

    b_obj = coff_obj_writer_serialize(arena, obj_writer);
    coff_obj_writer_release(&obj_writer);
  }

  String8 b_lib;
  {
    COFF_LibWriter *lib_writer = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(lib_writer, str8_lit("b.obj"), b_obj);
    b_lib = coff_lib_writer_serialize(arena, lib_writer, 0, 0, 1);
    coff_lib_writer_release(&lib_writer);
  }

  T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));
  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.lib"), b_lib));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.lib");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".q"));
  T_Ok(sect != 0);
}

T_BeginTest(fail_if_mismatch)
{
  T_Ok(t_write_entry_obj());

  // ------------------------------------------------------------
  // try linking two objs with mismatching directives

  String8 a1 = t_make_obj_with_directive(arena, str8_lit("/FAILIFMISMATCH:a=1"));
  String8 a2 = t_make_obj_with_directive(arena, str8_lit("/FAILIFMISMATCH:a=2"));
  T_Ok(t_write_file(str8_lit("a1.obj"), a1));
  T_Ok(t_write_file(str8_lit("a2.obj"), a2));

  t_invoke_linkerf("entry.obj a1.obj a2.obj /entry:entry /subsystem:console /out:a2.exe");
  if (t_id_linker() == T_Linker_RAD) T_Ok(g_last_exit_code == LNK_Error_FailIfMismatch);
  else                               T_Ok(g_last_exit_code != 0);

  // ------------------------------------------------------------
  // happy case

  T_Ok(t_write_file(str8_lit("a1_copy.obj"), a1));

  t_invoke_linkerf("entry.obj a1.obj a1_copy.obj /entry:entry /subsystem:console /out:a1.exe");
  T_Ok(g_last_exit_code == 0);

  // ------------------------------------------------------------
  // test conflicting directives in obj

  String8 conf_dirs = t_make_obj_with_directive(arena, str8_lit("/FAILIFMISMATCH:a=1 /FAILIFMISMATCH:a=2"));
  T_Ok(t_write_file(str8_lit("conf_dirs.obj"), conf_dirs));

  t_invoke_linkerf("entry.obj conf_dirs.obj /entry:entry /subsystem:console /out:conf_dirs.exe");
  if (t_id_linker() == T_Linker_RAD) T_Ok(g_last_exit_code == LNK_Error_FailIfMismatch);
  else                               T_Ok(g_last_exit_code != 0);

  // ------------------------------------------------------------
  // passing switch on command line

  t_invoke_linkerf("entry.obj a1.obj /FAILIFMISMATCH:a=2 /out:cmddir.exe");
  if (t_id_linker() == T_Linker_RAD) T_Ok(g_last_exit_code == LNK_Error_FailIfMismatch);
  else                               T_Ok(g_last_exit_code != 0);
}

T_BeginTest(long_section_name)
{
  Arch arch = Arch_x64;

  COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  U8 text[] = { 0xc3 };
  COFF_ObjSection *text_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
  coff_obj_writer_push_section(cow, str8_lit(".debug_info"), PE_DATA_SECTION_FLAGS, str8_lit("DEBUG_INFO"));
  coff_obj_writer_push_section(cow, str8_lit(".debug_abbrev"), PE_DATA_SECTION_FLAGS, str8_lit("DEBUG_ABBREV"));
  coff_obj_writer_push_symbol_extern(cow, str8_lit("entry"), 0, text_sect);
  String8 raw_coff = coff_obj_writer_serialize(arena, cow);
  coff_obj_writer_release(&cow);

  // link test.obj
  T_Ok(t_write_file(str8_lit("test.obj"), raw_coff));
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.obj");
  T_Ok(g_last_exit_code == 0);

  // load linked exe
  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *debug_info = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".debug_info"));
  T_Ok(debug_info);

  COFF_SectionHeader *debug_abbrev = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".debug_abbrev"));
  T_Ok(debug_abbrev);
}

T_BeginTest(debug_p_sig_mismatch)
{
  String8 a_obj_name = str8_lit("a.obj");
  String8 b_obj_name = str8_lit("b.obj");
  U32     a_sig      = 0xCAFEBABE;
  U32     b_sig      = 0xDEADBEEF;

  String8 a_debug_s;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
    U64 ss_start_off = srl.total_size;

    CV_SymObjName obj_name = {0};
    obj_name.sig = a_sig;
    String8 obj_name_string = a_obj_name;
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
    str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
    str8_serial_push_struct(arena, &srl, &obj_name);
    str8_serial_push_cstr(arena, &srl, obj_name_string);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    String8 comp3_data = cv_make_comp3(arena,
                                       0,
                                       CV_Language_C,
                                       CV_Arch_X64,
                                       /* ver_fe_major */ 0,
                                       /* ver_fe_minor */ 0,
                                       /* ver_fe_build */ 0,
                                       /* ver_feqfe    */ 0,
                                       /* ver_major    */ 14,
                                       /* ver_minor    */ 36,
                                       /* ver_build    */ 32537,
                                       /* ver_qfe      */ 0,
                                       str8_lit(BUILD_TITLE));
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
    str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
    str8_serial_push_string(arena, &srl, comp3_data);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    ss_header->kind = CV_C13SubSectionKind_Symbols;
    ss_header->size = srl.total_size - ss_start_off;
    str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

    a_debug_s = str8_serial_end(arena, &srl);
  }
  String8 a_debug_p;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);
    
    // signature
    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    // duplicate in a.obj
    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(ptr));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    // unique procedure type
    CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearPascal };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(proc));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
    str8_serial_push_struct(arena, &srl, &proc);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    // PCH ender
    CV_LeafEndPreComp endprecomp = { .sig = a_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(endprecomp));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_ENDPRECOMP);
    str8_serial_push_struct(arena, &srl, &endprecomp);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    a_debug_p = str8_serial_end(arena, &srl);
  }

  String8 b_debug_s;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
    U64 ss_start_off = srl.total_size;

    CV_SymObjName obj_name = {0};
    obj_name.sig = b_sig;
    String8 obj_name_string = a_obj_name;
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
    str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
    str8_serial_push_struct(arena, &srl, &obj_name);
    str8_serial_push_cstr(arena, &srl, obj_name_string);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    String8 comp3_data = cv_make_comp3(arena,
                                       0,
                                       CV_Language_C,
                                       CV_Arch_X64,
                                       /* ver_fe_major */ 0,
                                       /* ver_fe_minor */ 0,
                                       /* ver_fe_build */ 0,
                                       /* ver_feqfe    */ 0,
                                       /* ver_major    */ 14,
                                       /* ver_minor    */ 36,
                                       /* ver_build    */ 32537,
                                       /* ver_qfe      */ 0,
                                       str8_lit(BUILD_TITLE));
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
    str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
    str8_serial_push_string(arena, &srl, comp3_data);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    ss_header->kind = CV_C13SubSectionKind_Symbols;
    ss_header->size = srl.total_size - ss_start_off;
    str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

    b_debug_s = str8_serial_end(arena, &srl);
  }

  String8 b_debug_t;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_LeafPreComp precomp = { .start_index = CV_MinComplexTypeIndex, .count = 2, sig = b_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(precomp) + a_obj_name.size + 1);
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PRECOMP);
    str8_serial_push_struct(arena, &srl, &precomp);
    str8_serial_push_cstr(arena, &srl, a_obj_name);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
    str8_serial_push_struct(arena, &srl, &proc);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    b_debug_t = str8_serial_end(arena, &srl);
  }

  String8 a_obj;
  {
    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_section(cow, str8_lit(".debug$P"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, a_debug_p);
    coff_obj_writer_push_section(cow, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, a_debug_s);
    a_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  String8 b_obj;
  {
    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_section(cow, str8_lit(".debug$T"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, b_debug_t);
    coff_obj_writer_push_section(cow, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, b_debug_s);
    b_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), b_obj));
  T_Ok(t_write_entry_obj());

  String8 output = {0};
  t_invoke_(t_radlink_path(), str8_lit("/subsystem:console /entry:entry /out:a.exe /debug:full a.obj b.obj entry.obj"), max_U64, arena, &output);
  T_Ok(g_last_exit_code == 0);

  B32     found_error = 0;
  String8 a_obj_path    = t_make_file_path(arena, str8_lit("a.obj"));
  String8 b_obj_path    = t_make_file_path(arena, str8_lit("b.obj"));
  String8 expected_line = str8f(arena, "Error(%03d): %S: PCH signature mismatch, expected 0x%x got 0x%x; PCH obj %S", LNK_Error_PrecompSigMismatch, b_obj_path, b_sig, a_sig, a_obj_path);
  while (output.size) {
    String8 line = t_chop_line(&output);
    found_error = str8_match(line, expected_line, StringMatchFlag_CaseInsensitive);
    if (found_error) { break; }
  }
  T_Ok(found_error);
}

T_BeginTest(debug_p_and_debug_t_in_obj)
{
  U32     pch_sig      = 0xCAFEBABE;
  String8 pch_obj_name = str8_lit("pch.obj");

  String8 pch_debug_s;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
    U64 ss_start_off = srl.total_size;

    CV_SymObjName obj_name = {0};
    obj_name.sig = pch_sig;
    String8 obj_name_string = pch_obj_name;
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
    str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
    str8_serial_push_struct(arena, &srl, &obj_name);
    str8_serial_push_cstr(arena, &srl, obj_name_string);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    ss_header->kind = CV_C13SubSectionKind_Symbols;
    ss_header->size = srl.total_size - ss_start_off;
    str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

    pch_debug_s = str8_serial_end(arena, &srl);
  }

  String8 pch_debug_p;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    // signature
    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(ptr));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    // PCH ender
    CV_LeafEndPreComp endprecomp = { .sig = pch_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(endprecomp));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_ENDPRECOMP);
    str8_serial_push_struct(arena, &srl, &endprecomp);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    pch_debug_p = str8_serial_end(arena, &srl);
  }

  String8 pch_debug_t;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_LeafPreComp precomp = { .start_index = CV_MinComplexTypeIndex, .count = 1, sig = pch_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(precomp) + pch_obj_name.size + 1);
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PRECOMP);
    str8_serial_push_struct(arena, &srl, &precomp);
    str8_serial_push_cstr(arena, &srl, pch_obj_name);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
    str8_serial_push_struct(arena, &srl, &proc);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    pch_debug_t = str8_serial_end(arena, &srl);
  }

  COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  coff_obj_writer_push_section(cow, str8_lit(".debug$P"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, pch_debug_p);
  coff_obj_writer_push_section(cow, str8_lit(".debug$T"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, pch_debug_t);
  coff_obj_writer_push_section(cow, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, pch_debug_s);
  String8 raw_coff = coff_obj_writer_serialize(arena, cow);
  coff_obj_writer_release(&cow);

  T_Ok(t_write_file(str8_lit("pch.obj"), raw_coff));
  T_Ok(t_write_entry_obj());
  String8 output = {0};
  t_invoke_(t_radlink_path(), str8_lit("/subsystem:console /entry:entry /out:a.exe /debug:full pch.obj entry.obj"), max_U64, arena, &output);
  T_Ok(g_last_exit_code == 0);

  B32     found_warning = 0;
  String8 pch_obj_path  = t_make_file_path(arena, str8_lit("pch.obj"));
  String8 expected_line = str8f(arena, "Warning(%03d): %S: multiple sections with debug types detected, obj must have either .debug$T or .debug$P; discarding both sections", LNK_Warning_MultipleDebugTAndDebugP, pch_obj_path);
  while (output.size) {
    String8 line = t_chop_line(&output);
    found_warning = str8_match(line, expected_line, StringMatchFlag_CaseInsensitive);
    if (found_warning) { break; }
  }
  T_Ok(found_warning);
}

T_BeginTest(merge_duplicate_types)
{
  {
    U32     pch_sig      = 0xCAFEBABE;
    String8 pch_obj_name = str8_lit("pch.obj");
    String8 a_obj_name   = str8_lit("a.obj");
    String8 c_obj_name   = str8_lit("c.obj");

    String8 pch_debug_s;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
      U64 ss_start_off = srl.total_size;

      CV_SymObjName obj_name = {0};
      obj_name.sig = pch_sig;
      String8 obj_name_string = pch_obj_name;
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
      str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
      str8_serial_push_struct(arena, &srl, &obj_name);
      str8_serial_push_cstr(arena, &srl, obj_name_string);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      String8 comp3_data = cv_make_comp3(arena,
                                         0,
                                         CV_Language_C,
                                         CV_Arch_X64,
                                         /* ver_fe_major */ 0,
                                         /* ver_fe_minor */ 0,
                                         /* ver_fe_build */ 0,
                                         /* ver_feqfe    */ 0,
                                         /* ver_major    */ 14,
                                         /* ver_minor    */ 36,
                                         /* ver_build    */ 32537,
                                         /* ver_qfe      */ 0,
                                         str8_lit(BUILD_TITLE));
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
      str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
      str8_serial_push_string(arena, &srl, comp3_data);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      ss_header->kind = CV_C13SubSectionKind_Symbols;
      ss_header->size = srl.total_size - ss_start_off;
      str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

      pch_debug_s = str8_serial_end(arena, &srl);
    }
    String8 debug_p;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);
      
      // signature
      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      // duplicate in a.obj
      CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(ptr));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
      str8_serial_push_struct(arena, &srl, &ptr);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      // unique procedure type
      CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearPascal };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(proc));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
      str8_serial_push_struct(arena, &srl, &proc);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      // PCH ender
      CV_LeafEndPreComp endprecomp = { .sig = pch_sig };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(endprecomp));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_ENDPRECOMP);
      str8_serial_push_struct(arena, &srl, &endprecomp);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      debug_p = str8_serial_end(arena, &srl);
    }

    String8 a_debug_s;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
      U64 ss_start_off = srl.total_size;

      CV_SymObjName obj_name = {0};
      obj_name.sig = pch_sig;
      String8 obj_name_string = a_obj_name;
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
      str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
      str8_serial_push_struct(arena, &srl, &obj_name);
      str8_serial_push_cstr(arena, &srl, obj_name_string);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      String8 comp3_data = cv_make_comp3(arena,
                                         0,
                                         CV_Language_C,
                                         CV_Arch_X64,
                                         /* ver_fe_major */ 0,
                                         /* ver_fe_minor */ 0,
                                         /* ver_fe_build */ 0,
                                         /* ver_feqfe    */ 0,
                                         /* ver_major    */ 14,
                                         /* ver_minor    */ 36,
                                         /* ver_build    */ 32537,
                                         /* ver_qfe      */ 0,
                                         str8_lit(BUILD_TITLE));
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
      str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
      str8_serial_push_string(arena, &srl, comp3_data);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      ss_header->kind = CV_C13SubSectionKind_Symbols;
      ss_header->size = srl.total_size - ss_start_off;
      str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

      a_debug_s = str8_serial_end(arena, &srl);
    }

    String8 debug_t;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_LeafPreComp precomp = { .start_index = CV_MinComplexTypeIndex, .count = 2, sig = pch_sig };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(precomp) + pch_obj_name.size + 1);
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PRECOMP);
      str8_serial_push_struct(arena, &srl, &precomp);
      str8_serial_push_cstr(arena, &srl, pch_obj_name);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
      str8_serial_push_struct(arena, &srl, &ptr);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
      str8_serial_push_struct(arena, &srl, &proc);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      debug_t = str8_serial_end(arena, &srl);
    }

    String8 c_debug_t;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_LeafPointer ptr = { .itype = CV_BasicType_SHORT };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
      str8_serial_push_struct(arena, &srl, &ptr);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
      str8_serial_push_struct(arena, &srl, &proc);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      c_debug_t = str8_serial_end(arena, &srl);
    }

    String8 c_debug_s;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
      U64 ss_start_off = srl.total_size;

      // S_OBJNAME
      CV_SymObjName obj_name = {0};
      obj_name.sig = pch_sig;
      String8 obj_name_string = c_obj_name;
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
      str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
      str8_serial_push_struct(arena, &srl, &obj_name);
      str8_serial_push_cstr(arena, &srl, obj_name_string);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      // S_COMPILE3
      String8 comp3_data = cv_make_comp3(arena,
                                         0,
                                         CV_Language_C,
                                         CV_Arch_X64,
                                         /* ver_fe_major */ 0,
                                         /* ver_fe_minor */ 0,
                                         /* ver_fe_build */ 0,
                                         /* ver_feqfe    */ 0,
                                         /* ver_major    */ 14,
                                         /* ver_minor    */ 36,
                                         /* ver_build    */ 32537,
                                         /* ver_qfe      */ 0,
                                         str8_lit(BUILD_TITLE));
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
      str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
      str8_serial_push_string(arena, &srl, comp3_data);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      // S_LPROC32
      U16 *foo_size = str8_serial_push_size(arena, &srl, sizeof(*foo_size));
      U64  foo_off  = srl.total_size;
      str8_serial_push_u16(arena, &srl, CV_SymKind_LPROC32);
      CV_SymProc32 *foo_proc = str8_serial_push_size(arena, &srl, sizeof(*foo_proc));
      foo_proc->itype = 0x1001;
      foo_proc->sec = 1;
      foo_proc->len = 1;
      str8_serial_push_cstr(arena, &srl, str8_lit("foo"));
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);
      *foo_size = srl.total_size - foo_off;

      // S_PROC_ID_END
      str8_serial_push_u16(arena, &srl, 2);
      str8_serial_push_u16(arena, &srl, CV_SymKind_END);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      // $$Symbols header
      ss_header->kind = CV_C13SubSectionKind_Symbols;
      ss_header->size = srl.total_size - ss_start_off;
      str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

      c_debug_s = str8_serial_end(arena, &srl);
    }

    String8 pch_obj;
    {
      COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
      coff_obj_writer_push_section(cow, str8_lit(".debug$P"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_p);
      coff_obj_writer_push_section(cow, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, pch_debug_s);
      pch_obj = coff_obj_writer_serialize(arena, cow);
    }

    String8 a_obj;
    {
      COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
      coff_obj_writer_push_section(cow, str8_lit(".debug$T"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_t);
      coff_obj_writer_push_section(cow, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, a_debug_s);
      a_obj = coff_obj_writer_serialize(arena, cow);
    }

    String8 b_obj;
    {
      COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
      coff_obj_writer_push_section(cow, str8_lit(".debug$T"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_t);
      b_obj = coff_obj_writer_serialize(arena, cow);
    }

    String8 c_obj;
    {
      COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
      coff_obj_writer_push_section(cow, str8_lit(".text"),    PE_TEXT_SECTION_FLAGS, str8_lit((U8[]){ 0xc3 }));
      coff_obj_writer_push_section(cow, str8_lit(".debug$T"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, c_debug_t);
      coff_obj_writer_push_section(cow, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, c_debug_s);
      c_obj = coff_obj_writer_serialize(arena, cow);
    }

    String8 entry_obj = t_make_entry_obj(arena);

    T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));
    T_Ok(t_write_file(str8_lit("pch.obj"), pch_obj));
    T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
    T_Ok(t_write_file(str8_lit("b.obj"), b_obj));
    T_Ok(t_write_file(str8_lit("c.obj"), c_obj));

    t_invoke_linkerf("/subsystem:console /entry:entry /debug:full /out:a.exe pch.obj a.obj b.obj c.obj entry.obj");
    T_Ok(g_last_exit_code == 0);
  }

  // load msf
  String8     pdb = t_read_file(arena, str8_lit("a.pdb"));
  MSF_Parsed *msf = msf_parsed_from_data(arena, pdb);
  // find named streams
  String8                info_data     = msf_data_from_stream(msf, PDB_FixedStream_Info);
  PDB_Info              *pdb_info      = pdb_info_from_data(arena, info_data);
  PDB_NamedStreamTable  *named_streams = pdb_named_stream_table_from_info(arena, pdb_info);
  // find string table
  MSF_StreamNumber  strtbl_sn   = named_streams->sn[PDB_NamedStream_StringTable];
  String8           strtbl_data = msf_data_from_stream(msf, strtbl_sn);
  PDB_Strtbl       *strtbl      = pdb_strtbl_from_data(arena, strtbl_data);
  // find TPI
  String8       tpi_data = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
  PDB_TpiParsed *tpi     = pdb_tpi_from_data(arena, tpi_data);

  U64 type_count = tpi->itype_opl - tpi->itype_first;
  T_Ok(type_count == 5);
  
  CV_DebugT debug_t = cv_debug_t_from_data(arena, pdb_leaf_data_from_tpi(tpi), 4);
  T_Ok(debug_t.count == type_count);

  {
    CV_Leaf ptr_leaf  = cv_debug_t_get_leaf(&debug_t, 0);
    T_Ok(ptr_leaf.kind == CV_LeafKind_POINTER);
    T_Ok(ptr_leaf.data.size == sizeof(CV_LeafPointer));

    CV_LeafPointer *ptr = (CV_LeafPointer *)ptr_leaf.data.str;
    T_Ok(ptr->itype == CV_BasicType_VOID);
    T_Ok(ptr->attribs == 0);
  }

  {
    CV_Leaf proc_leaf = cv_debug_t_get_leaf(&debug_t, 1);
    T_Ok(proc_leaf.kind == CV_LeafKind_PROCEDURE);
    T_Ok(proc_leaf.data.size == sizeof(CV_LeafProcedure));

    CV_LeafProcedure *proc = (CV_LeafProcedure *)proc_leaf.data.str;
    T_Ok(proc->ret_itype == 0x1000);
    T_Ok(proc->call_kind == CV_CallKind_NearPascal);
  }

  {
    CV_Leaf proc_leaf = cv_debug_t_get_leaf(&debug_t, 2);
    T_Ok(proc_leaf.kind == CV_LeafKind_PROCEDURE);
    T_Ok(proc_leaf.data.size == sizeof(CV_LeafProcedure));

    CV_LeafProcedure *proc = (CV_LeafProcedure *)proc_leaf.data.str;
    T_Ok(proc->ret_itype == 0x1000);
    T_Ok(proc->call_kind == CV_CallKind_NearC);
  }

  {
    CV_Leaf ptr_leaf  = cv_debug_t_get_leaf(&debug_t, 3);
    T_Ok(ptr_leaf.kind == CV_LeafKind_POINTER);
    T_Ok(ptr_leaf.data.size == sizeof(CV_LeafPointer));

    CV_LeafPointer *ptr = (CV_LeafPointer *)ptr_leaf.data.str;
    T_Ok(ptr->itype == CV_BasicType_SHORT);
    T_Ok(ptr->attribs == 0);
  }

  {
    CV_Leaf proc_leaf = cv_debug_t_get_leaf(&debug_t, 4);
    T_Ok(proc_leaf.kind == CV_LeafKind_PROCEDURE);
    T_Ok(proc_leaf.data.size == sizeof(CV_LeafProcedure));

    CV_LeafProcedure *proc = (CV_LeafProcedure *)proc_leaf.data.str;
    T_Ok(proc->ret_itype == 0x1003);
    T_Ok(proc->call_kind == CV_CallKind_NearC);
  }
}

T_BeginTest(cyclic_type)
{
  String8List *debug_t = push_array(arena, String8List, 1);
  str8_serial_begin(arena, debug_t);
  str8_serial_push_u32(arena, debug_t, CV_Signature_C13);
  str8_serial_push_string(arena, debug_t, cv_make_leaf(arena, CV_LeafKind_POINTER, str8_struct((&(CV_LeafPointer){ .itype = 0x1001 })), CV_LeafAlign));
  str8_serial_push_string(arena, debug_t, cv_make_leaf(arena, CV_LeafKind_POINTER, str8_struct((&(CV_LeafPointer){ .itype = 0x1000 })), CV_LeafAlign));

  CV_DebugS debug_s = {0};
  str8_list_push(arena, &debug_s.data_list[CV_C13SubSectionIdxKind_Symbols], cv_make_symbol(arena, CV_SymKind_GPROC32, cv_make_proc32(arena, (CV_SymProc32){ .itype = 0x1001 }, str8_lit("foo"))));
  String8List debug_s_string = cv_data_from_debug_s_c13(arena, &debug_s, 1);

  COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
  coff_obj_writer_push_section(cow, str8_lit(".debug$T"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, str8_serial_end(arena, debug_t));
  coff_obj_writer_push_section(cow, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, str8_list_join(arena, &debug_s_string, 0));
  String8 raw_coff = coff_obj_writer_serialize(arena, cow);
  coff_obj_writer_release(&cow);

  T_Ok(t_write_entry_obj());
  T_Ok(t_write_file(str8_lit("cycle.obj"), raw_coff));

  String8 cmd_line = str8f(arena, "/subsystem:console /entry:entry /out:a.exe /debug:full /rad_ignore:-%u cycle.obj entry.obj", LNK_Error_InvalidTypeIndex);
  String8 output   = {0};
  t_invoke_(t_radlink_path(), cmd_line, max_U64, arena, &output);
  T_Ok(g_last_exit_code == 0);

  B32 is_cycle_detected = 0;
  while (output.size && !is_cycle_detected) {
    is_cycle_detected = t_match_linef(&output,
                      "Error(043): %S: LF_POINTER(type_index: 0x1000) forward refs member type index 0x1001 (leaf struct offset: 0x0)",
                      t_make_file_path(arena, str8_lit("cycle.obj")));
    t_chop_line(&output);
  }
  T_Ok(is_cycle_detected);
}

T_BeginTest(get_msf_stream_pages)
{
  MSF_Context *msf = msf_alloc(MSF_DEFAULT_PAGE_SIZE, MSF_DEFAULT_FPM);

  {
    U64 stream_size = MB(150) + 1;

    MSF_StreamNumber sn = msf_stream_alloc_ex(msf, stream_size);

    U8 *test = push_array(arena, U8, stream_size);
    MemorySet(test, 0xca, stream_size/2);
    MemorySet(test + stream_size/2, 0xbe, stream_size/2);

    String8List stream_data = msf_data_from_sn(arena, msf, sn);
    T_Ok(stream_data.total_size == stream_size);
    T_Ok(stream_data.node_count == 11);

    String8Array a = str8_array_from_list(arena, &stream_data);
    T_Ok(a.v[0].size  == 0xffd000);
    T_Ok(a.v[1].size  == 0xffe000);
    T_Ok(a.v[2].size  == 0xffe000);
    T_Ok(a.v[3].size  == 0xffe000);
    T_Ok(a.v[4].size  == 0xffe000);
    T_Ok(a.v[5].size  == 0xffe000);
    T_Ok(a.v[6].size  == 0xffe000);
    T_Ok(a.v[7].size  == 0xffd000);
    T_Ok(a.v[8].size  == 0x1000);
    T_Ok(a.v[9].size  == 0xffe000);
    T_Ok(a.v[10].size == 0x613001);

    String8Node buf     = *stream_data.first;
    U64         buf_pos = 0;
    str8_buffer_write(&buf, &buf_pos, str8(test, stream_size));

    String8 cmp = msf_stream_read_block(arena, msf, sn, stream_size);
    T_Ok(cmp.size == stream_size);
    T_Ok(MemoryCompare(cmp.str, test, stream_size) == 0);
  }

  {
    MSF_StreamNumber sn = msf_stream_alloc_ex(msf, 1);
    String8List stream_data = msf_data_from_sn(arena, msf, sn);
    T_Ok(stream_data.node_count == 1);
    T_Ok(stream_data.total_size == 1);
    T_Ok(stream_data.first->string.size == 1);
  }

  msf_release(msf);
}

T_BeginTest(validate_info_stream)
{
  COFF_TimeStamp  time_stamp = 123;
  U32             age        = 1;
  Guid            guid       = { .data1 = max_U32, .data2 = max_U16 - 1, .data3 = max_U16 - 2, .data4 = { 1, 2, 3, 4, 5, 6, 7, 8 } };
  PDB_Context    *pdb        = pdb_alloc(MSF_DEFAULT_PAGE_SIZE, COFF_MachineType_X64, time_stamp, age, guid);

  char *stream_names[] = { "one", "two", "three", "four", "five" };
  MSF_StreamNumber stream_numbers[ArrayCount(stream_names)] = {0};

  for EachElement(i, stream_names) {
    stream_numbers[i] = pdb_push_named_stream(&pdb->info->named_stream_ht, pdb->msf, str8_cstring(stream_names[i]));
    T_Ok(stream_numbers[i] != MSF_INVALID_STREAM_NUMBER);
  }

  TP_Context *tp       = tp_alloc(arena, 1, 1, str8_lit("foo"));
  TP_Arena   *tp_arena = tp_arena_alloc(tp);
  pdb_build(tp, tp_arena, pdb, (CV_StringHashTable){0}, 1, 0);

  T_Ok(msf_build(pdb->msf) == MSF_Error_OK);
  String8List raw_msf_list = msf_get_page_data_nodes(arena, pdb->msf);
  T_Ok(t_write_file_list(str8_lit("test.pdb"), raw_msf_list));
  
  String8     raw_msf    = t_read_file(arena, str8_lit("test.pdb"));
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
  T_Ok(str8_match(info_data, str8_array_fixed(expected_info_data), 0));

  pdb_release(pdb);
  tp_arena_release(&tp_arena);
  tp_release(tp);
}

T_BeginTest(patch_cv_symbol_tree)
{
  String8List raw_symbols = {0};
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_OBJNAME,        cv_make_obj_name(arena, str8_lit("foo.obj"), 123)));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_GPROC32,        cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("Proc"))));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_INLINESITE,     cv_make_inline_site(arena, (CV_SymInlineSite){0}, str8_zero())));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_INLINESITE_END, cv_make_inline_site_end(arena)));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_END,            cv_make_end(arena)));

  U64 tree_size = cv_patch_symbol_tree_offsets(raw_symbols, sizeof(CV_Signature), 4);
  T_Ok(tree_size == 84);

  {
    String8Node buf     = *raw_symbols.first;
    U64         buf_pos = 0;

    CV_SymbolHeader obj_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(obj_header), &obj_header) == sizeof(obj_header));
    T_Ok(obj_header.kind == CV_SymKind_OBJNAME);
    T_Ok(str8_buffer_skip(&buf, &buf_pos, obj_header.size - sizeof(CV_SymKind)));

    CV_SymbolHeader proc_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(proc_header), &proc_header) == sizeof(proc_header));
    T_Ok(proc_header.kind == CV_SymKind_GPROC32);

    CV_SymProc32 proc;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(proc), &proc) == sizeof(proc));
    T_Ok(proc.end == 0x54);
    T_Ok(str8_buffer_skip(&buf, &buf_pos, proc_header.size - sizeof(CV_SymKind) - sizeof(proc)));

    CV_SymbolHeader inline_site_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(inline_site_header), &inline_site_header) == sizeof(inline_site_header));
    T_Ok(inline_site_header.kind == CV_SymKind_INLINESITE);

    CV_SymInlineSite inline_site;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(inline_site), &inline_site));
    T_Ok(inline_site.parent == 0x14);
    T_Ok(inline_site.end == 0x50);
    T_Ok(str8_buffer_skip(&buf, &buf_pos, inline_site_header.size - sizeof(CV_SymKind) - sizeof(inline_site)));

    CV_SymbolHeader inline_end_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(inline_end_header), &inline_end_header) == sizeof(inline_end_header));
    T_Ok(inline_end_header.kind == CV_SymKind_INLINESITE_END);

    CV_SymbolHeader proc_end_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(proc_end_header), &proc_end_header) == sizeof(proc_end_header));
    T_Ok(proc_end_header.kind == CV_SymKind_END);

    T_Ok(buf.string.size == 0);
    T_Ok(buf.string.str == 0);
    T_Ok(buf_pos == 0);
  }
}

T_BeginTest(whole_archive)
{
  String8 a_obj;
  {
    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_section(cow, str8_lit(".a"), PE_DATA_SECTION_FLAGS, str8_lit("a"));
    a_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  String8 b_obj;
  {
    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    coff_obj_writer_push_section(cow, str8_lit(".b"), PE_DATA_SECTION_FLAGS, str8_lit("b"));
    b_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  String8 a_lib;
  {
    COFF_LibWriter *ciw = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(ciw, str8_lit("a.obj"), a_obj);
    a_lib = coff_lib_writer_serialize(arena, ciw, 0, 0, 1);
    coff_lib_writer_release(&ciw);
  }
  
  String8 b_lib;
  {
    COFF_LibWriter *ciw = coff_lib_writer_alloc();
    coff_lib_writer_push_obj(ciw, str8_lit("b.obj"), b_obj);
    b_lib = coff_lib_writer_serialize(arena, ciw, 0, 0, 1);
    coff_lib_writer_release(&ciw);
  }

  T_Ok(t_write_entry_obj());
  T_Ok(t_write_file(str8_lit("a.lib"), a_lib));
  T_Ok(t_write_file(str8_lit("b.lib"), b_lib));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:all_libs.exe entry.obj /wholearchive a.lib b.lib");
  T_Ok(g_last_exit_code == 0);
  {
    String8              exe           = t_read_file(arena, str8_lit("all_libs.exe"));
    PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8              string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader  *a_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    COFF_SectionHeader  *b_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(a_sect != 0);
    T_Ok(b_sect != 0);
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:only_a.exe entry.obj /wholearchive:a.lib a.lib b.lib");
  T_Ok(g_last_exit_code == 0);
  {
    String8              exe           = t_read_file(arena, str8_lit("only_a.exe"));
    PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8              string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader  *a_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    COFF_SectionHeader  *b_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(a_sect != 0);
    T_Ok(b_sect == 0);
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:only_b.exe /wholearchive:b.lib a.lib b.lib entry.obj");
  T_Ok(g_last_exit_code == 0);
  {
    String8              exe           = t_read_file(arena, str8_lit("only_b.exe"));
    PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8              string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader  *a_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    COFF_SectionHeader  *b_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(a_sect == 0);
    T_Ok(b_sect != 0);
  }
}
#if OS_WINDOWS

internal B32
t_radlink_validate_asan_out(String8 obj_name)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = 0;

  t_invoke_(t_radlink_path(), str8f(scratch.arena, "%S /debug:full", obj_name), max_U64, 0, 0);
  if (g_last_exit_code != 0) { goto exit; }

  String8 exe_path = t_make_file_path(scratch.arena, str8f(scratch.arena, "%S.exe", str8_chop_last_dot(obj_name)));
  String8 out = {0};

  char *old_path_cstr = getenv("PATH");
  String8List env = {0};
  str8_list_pushf(scratch.arena, &env, "PATH=%S;%S", str8_chop_last_slash(t_cl_path()), str8_cstring(old_path_cstr));
  t_invoke_env(exe_path, str8_zero(), env, max_U64, scratch.arena, &out);

  String8 s = out;

  String8 header = t_chop_line(&s);
  if ( ! str8_match(header, str8_lit("================================================================="), 0)) {
    goto exit;
  }

  String8 cause = t_chop_line(&s);
  if ( str8_find_needle(cause, 0, str8_lit("AddressSanitizer: heap-use-after-free on address"), 0) >= cause.size) {
    goto exit;
  }

  is_ok = 1;
  exit:;
  scratch_end(scratch);
  return is_ok;
}

T_BeginTest(infer_asan)
{
  char *program = 
    "#include <stdlib.h>\n"
    " int main(void) {\n"
    "int *foo = malloc(sizeof(*foo));\n"
    "free(foo);\n"
    "*foo = 1;\n"
    "}\n"
    ;

  // /MD
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    String8 cl_output = {0};
    t_invoke_(t_cl_path(), str8_lit("/MD /fsanitize=address /Z7 /c /Fo:main_md.obj main.c"), max_U64, arena, &cl_output);
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_md.obj")));
  }

  // /MDd
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    String8 cl_output = {0};
    t_invoke_(t_cl_path(), str8_lit("/MDd /fsanitize=address /Z7 /c /Fo:main_mdd.obj main.c"), max_U64, arena, &cl_output);
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_mdd.obj")));
  }

  // /MT
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    String8 cl_output = {0};
    t_invoke_(t_cl_path(), str8_lit("/MT /fsanitize=address /Z7 /c /Fo:main_mt.obj main.c"), max_U64, arena, &cl_output);
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_mt.obj")));
  }

  // /MTd
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    String8 cl_output = {0};
    t_invoke_(t_cl_path(), str8_lit("/MT /fsanitize=address /Z7 /c /Fo:main_mtd.obj main.c"), max_U64, arena, &cl_output);
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_mtd.obj")));
  }
}

#endif

#if OS_WINDOWS
T_BeginTest(determ_test)
{
  // compile the test target (torture)
  String8 cl_line = str8f(arena, "/fsanitize=address /c /Z7 /Fo:test.obj -I%S /Zc:preprocessor %S/torture/torture_main.c", t_src_path(), t_src_path());
  String8 cl_out = {0};
  t_invoke_(t_cl_path(), cl_line, max_U64, arena, &cl_out);
  T_Ok(g_last_exit_code == 0);

  // single-threaded link
  String8 refs_path = t_make_file_path(arena, str8_lit("b.types"));
  t_invoke_linkerf("test.obj /debug:full /rad_time_stamp:0 /rad_workers:1 /rad_store_types:%S /out:a.exe", refs_path);
  T_Ok(g_last_exit_code == 0);

  // rename a -> b
  T_Ok(os_move_file_path(t_make_file_path(arena, str8_lit("b.exe")), t_make_file_path(arena, str8_lit("a.exe"))));
  T_Ok(os_move_file_path(t_make_file_path(arena, str8_lit("b.pdb")), t_make_file_path(arena, str8_lit("a.pdb"))));

  // read b
  String8 b_exe = t_read_file(arena, str8_lit("b.exe"));
  String8 b_pdb = t_read_file(arena, str8_lit("b.pdb"));

  // multi-threaded links
  for EachIndex(i, 25) {
    Temp temp = temp_begin(arena);

    t_delete_file(str8_lit("a.exe"));
    t_delete_file(str8_lit("a.pdb"));

    t_invoke_linkerf("test.obj /debug:full /rad_time_stamp:0 /out:a.exe");
    T_Ok(g_last_exit_code == 0);

    String8 a_exe = t_read_file(temp.arena, str8_lit("a.exe"));
    String8 a_pdb = t_read_file(temp.arena, str8_lit("a.pdb"));
    T_Ok(str8_match(a_exe, b_exe, 0));
    T_Ok(str8_match(a_pdb, b_pdb, 0));

    temp_end(temp);
  }
}

#endif

#if 0

T_BeginTest(fold_two_funcs)
{
  String8 ident_funcs_obj;
  {
    U8 same_text[] = {
      0x48, 0x31, 0xc0, // xor rax, rax
      0xc3              // ret
    };
    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    COFF_ObjSection *a_sect = coff_obj_writer_push_section(cow, str8_lit(".text$mn"), PE_TEXT_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(same_text));
    COFF_ObjSection *b_sect = coff_obj_writer_push_section(cow, str8_lit(".text$mb"), PE_TEXT_SECTION_FLAGS|COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(same_text));
    coff_obj_writer_push_symbol_secdef(cow, a_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, b_sect, COFF_ComdatSelect_NoDuplicates);
    COFF_ObjSymbol *a_symbol = coff_obj_writer_push_symbol_extern_func(cow, str8_lit("a"), 0, a_sect);
    COFF_ObjSymbol *b_symbol = coff_obj_writer_push_symbol_extern_func(cow, str8_lit("b"), 0, b_sect);

    U8 entry_text[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00, // call a
      0xe8, 0x00, 0x00, 0x00, 0x00, // call b
      0xc3,                         // ret
    };
    COFF_ObjSection *entry_sect = t_push_text_section(cow, str8_array_fixed(entry_text));
    coff_obj_writer_push_symbol_extern_func(cow, str8_lit("entry"), 0, entry_sect);
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 1, a_symbol);
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 6, b_symbol);

    ident_funcs_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }
  
  T_Ok(t_write_file(str8_lit("ident_funcs.obj"), ident_funcs_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /opt:icf ident_funcs.obj");
  T_Ok(g_last_exit_code == 0);

  String8 exe = t_read_file(arena, str8_lit("ident_funcs.exe"));
  T_Ok(exe.size);

  PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8              string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader  *text_sect     = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".text"));

  // validate .text header
  T_Ok(text_sect->voff  == 0x1000);
  T_Ok(text_sect->vsize >= 0x14);
  T_Ok(text_sect->fsize == 0x200);

  T_Ok(text_sect->foff + text_sect->vsize <= exe.size);
  String8 text_data = str8_substr(exe, r1u64(text_sect->foff, text_sect->foff + 0x14));

  U8 expected_text[] = {
    // entry
    0xe8, 0x0b, 0x00, 0x00, 0x00,
    0xe8, 0x06, 0x00, 0x00, 0x00,
    0xc3,

    // pad
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

    // a and b folded
    0x48, 0x31, 0xc0,
    0xc3,
  };
  T_Ok(str8_match(text_data, str8_array_fixed(expected_text), 0));
}


T_BeginTest(same_but_different)
{
  String8 obj;
  {
    U8 text[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00, // call $
      0xc3
    };

    U8 return_1[] = {
      0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
      0xc3                                      // ret
    };

    U8 return_2[] = {
      0x48, 0xc7, 0xc0, 0x02, 0x00, 0x00, 0x00, // mov rax, 2
      0xc3                                      // ret
    };

    U8 call_a_and_b[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xc3
    };

    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    COFF_ObjSection *entry_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(call_a_and_b));
    COFF_ObjSection *a_sect     = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(text));
    COFF_ObjSection *b_sect     = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(text));
    COFF_ObjSection *c_sect     = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(return_1));
    COFF_ObjSection *d_sect     = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(return_2));

    coff_obj_writer_push_symbol_secdef(cow, a_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, b_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, c_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, d_sect, COFF_ComdatSelect_NoDuplicates);

    coff_obj_writer_push_symbol_extern(cow, str8_lit("entry"), 0, entry_sect);

    COFF_ObjSymbol *a_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("a"), 0, a_sect);
    COFF_ObjSymbol *b_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("b"), 0, b_sect);
    COFF_ObjSymbol *c_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("c"), 0, c_sect);
    COFF_ObjSymbol *d_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("d"), 0, d_sect);

    // a -> c
    coff_obj_writer_section_push_reloc_rel32(cow, a_sect, 1, c_symbol);

    // b -> d
    coff_obj_writer_section_push_reloc_rel32(cow, b_sect, 1, d_symbol);

    // entry -> { a | b }
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 1, a_symbol);
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 6, b_symbol);

    obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /opt:icf a.obj");
  T_Ok(g_last_exit_code == 0);

  // validate output
  {
    U8 expected_text[] = {
      0xe8, 0x0b, 0x00, 0x00, 0x00, // call a
      0xe8, 0x16, 0x00, 0x00, 0x00, // call b
      0xc3,                        
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 
      0xe8, 0x1b, 0x00, 0x00, 0x00, // call c
      0xc3,                         
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0xe8, 0x1b, 0x00, 0x00, 0x00, // call d
      0xc3,
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
      0xc3,
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0x48, 0xc7, 0xc0, 0x02, 0x00, 0x00, 0x00, // mov rax, 2
      0xc3,
    };

    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}


T_BeginTest(fold_diamond)
{
  String8 a_obj;
  {
    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);

    U8 call_b_and_c[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xc3 
    };

    U8 call_and_return[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xc3 
    };

    U8 clear_and_return[] = {
      0x48, 0x31, 0xc0,
      0xc3
    };

    COFF_ObjSection *a_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(call_b_and_c));
    COFF_ObjSection *b_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(call_and_return));
    COFF_ObjSection *c_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(call_and_return));
    COFF_ObjSection *d_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT, str8_array_fixed(clear_and_return));

    coff_obj_writer_push_symbol_secdef(cow, a_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, b_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, c_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, d_sect, COFF_ComdatSelect_NoDuplicates);

    coff_obj_writer_push_symbol_extern(cow, str8_lit("a"), 0, a_sect);
    COFF_ObjSymbol *b_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("b"), 0, b_sect);
    COFF_ObjSymbol *c_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("c"), 0, c_sect);
    COFF_ObjSymbol *d_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("d"), 0, d_sect);

    // a -> { b | c }
    coff_obj_writer_section_push_reloc_rel32(cow, a_sect, 1, b_symbol);
    coff_obj_writer_section_push_reloc_rel32(cow, a_sect, 6, c_symbol);

    // b -> d
    coff_obj_writer_section_push_reloc_rel32(cow, b_sect, 1, d_symbol);

    // c -> d
    coff_obj_writer_section_push_reloc_rel32(cow, c_sect, 1, d_symbol);

    a_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  t_invoke_linkerf("/subsystem:console /entry:a /out:a.exe /opt:icf a.obj");
  T_Ok(g_last_exit_code == 0);

  // validate output
  {
    U8 expected_text[] = {
      0xe8, 0x0b, 0x00, 0x00, 0x00,
      0xe8, 0x06, 0x00, 0x00, 0x00,
      0xc3,

      0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

      0xe8, 0x0b, 0x00, 0x00, 0x00,
      0xc3,

      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

      0x48, 0x31, 0xc0,
      0xc3
    };

    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}


T_BeginTest(cyclic_icf)
{
  String8 a_obj;
  {
    U8 text[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xc3
    };
    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *a_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    COFF_ObjSection *b_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(text));
    COFF_ObjSymbol *a_symbol = coff_obj_writer_push_symbol_extern(cow, str8_lit("a"), 0, a_sect);
    COFF_ObjSymbol *b_symbol = coff_obj_writer_push_symbol_static(cow, str8_lit("b"), 0, b_sect);

    // a <-> b
    coff_obj_writer_section_push_reloc_rel32(cow, a_sect, 1, b_symbol);
    coff_obj_writer_section_push_reloc_rel32(cow, b_sect, 1, a_symbol);

    a_obj = coff_obj_writer_serialize(arena, cow);

    coff_obj_writer_release(&cow);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));

  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:a /opt:icf a.obj");
  T_Ok(g_last_exit_code == 0);

  // validate output
  {
    U8 expected_text[] = {
      0xe8, 0x0b, 0x00, 0x00, 0x00, // a
      0xc3,
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0xe8, 0xeb, 0xff, 0xff, 0xff, // b
      0xc3,            
    };

    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}


T_BeginTest(fold_with_largest_align)
{
  String8 a_obj;
  {
    U8 text[] = {
      0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
      0xc3
    };

    U8 call_a_and_b[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xc3 
    };

    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *entry_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(call_a_and_b));
    COFF_ObjSection *a_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_Align4Bytes, str8_array_fixed(text));
    COFF_ObjSection *b_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_Align8Bytes, str8_array_fixed(text));

    coff_obj_writer_push_symbol_secdef(cow, a_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, b_sect, COFF_ComdatSelect_NoDuplicates);

    coff_obj_writer_push_symbol_extern(cow, str8_lit("entry"), 0, entry_sect);
    COFF_ObjSymbol *a_symbol = coff_obj_writer_push_symbol_static(cow, str8_lit("a"), 0, a_sect);
    COFF_ObjSymbol *b_symbol = coff_obj_writer_push_symbol_static(cow, str8_lit("b"), 0, b_sect);
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 1, a_symbol);
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 6, b_symbol);

    a_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  // swap sections for a and b
  String8 b_obj;
  {
    U8 text[] = {
      0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
      0xc3
    };

    U8 call_a_and_b[] = {
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xc3 
    };

    COFF_ObjWriter *cow = coff_obj_writer_alloc(0, COFF_MachineType_X64);
    COFF_ObjSection *entry_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS, str8_array_fixed(call_a_and_b));
    COFF_ObjSection *a_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_Align8Bytes, str8_array_fixed(text));
    COFF_ObjSection *b_sect = coff_obj_writer_push_section(cow, str8_lit(".text"), PE_TEXT_SECTION_FLAGS | COFF_SectionFlag_LnkCOMDAT | COFF_SectionFlag_Align4Bytes, str8_array_fixed(text));

    coff_obj_writer_push_symbol_secdef(cow, a_sect, COFF_ComdatSelect_NoDuplicates);
    coff_obj_writer_push_symbol_secdef(cow, b_sect, COFF_ComdatSelect_NoDuplicates);

    coff_obj_writer_push_symbol_extern(cow, str8_lit("entry"), 0, entry_sect);
    COFF_ObjSymbol *a_symbol = coff_obj_writer_push_symbol_static(cow, str8_lit("a"), 0, a_sect);
    COFF_ObjSymbol *b_symbol = coff_obj_writer_push_symbol_static(cow, str8_lit("b"), 0, b_sect);
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 1, a_symbol);
    coff_obj_writer_section_push_reloc_rel32(cow, entry_sect, 6, b_symbol);

    b_obj = coff_obj_writer_serialize(arena, cow);
    coff_obj_writer_release(&cow);
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), b_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe b.obj");
  T_Ok(g_last_exit_code == 0);

  U8 expected_text[] = {
    0xe8, 0x0b, 0x00, 0x00, 0x00,   
    0xe8, 0x06, 0x00, 0x00, 0x00,   
    0xc3,               
    0xcc,
    0xcc,
    0xcc,
    0xcc,
    0xcc,
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00,
    0x00,
    0xc3,
  };

  // validate output in a.exe
  {
    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }

  // validate output in b.exe
  {
    String8 exe = t_read_file(arena, str8_lit("b.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}

#endif

#undef T_Group

