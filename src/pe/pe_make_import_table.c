// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal COFF_ObjSymbol *
pe_make_indirect_jump_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *iat_symbol, String8 thunk_name)
{
  ProfBeginFunction();
  
  static U8 thunk[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 }; // jmp [__imp_<FUNC_NAME>]
  
  // emit chunk
  String8 jmp_data        = push_str8_copy(obj_writer->arena, str8_array_fixed(thunk));
  U64     jmp_data_offset = code_sect->data.total_size;
  str8_list_push(obj_writer->arena, &code_sect->data, jmp_data);
  
  // patch thunk with imports address
  static const U64 JMP_OPERAND_OFFSET = 2;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, jmp_data_offset + JMP_OPERAND_OFFSET, iat_symbol, COFF_Reloc_X64_Rel32); 
  
  COFF_ObjSymbol *jmp_thunk_symbol = coff_obj_writer_push_symbol_extern(obj_writer, thunk_name, jmp_data_offset, code_sect);

  ProfEnd();
  return jmp_thunk_symbol;
}

internal COFF_ObjSymbol *
pe_make_load_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, COFF_ObjSymbol *imp_addr_ptr, COFF_ObjSymbol *tail_merge, String8 func_name)
{
  ProfBeginFunction();
  
  static U8 load_thunk[] = {
    0x48, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00,  // lea rax, [__imp_<FUNC_NAME>]
    0xE9, 0x00, 0x00, 0x00, 0x00               // jmp __tailMerge_<DLL_NAME>
  };
  
  // emit load thunk chunk
  U64     load_thunk_data_offset = code_sect->data.total_size;
  String8 load_thunk_data        = push_str8_copy(obj_writer->arena, str8_array_fixed(load_thunk));
  str8_list_push(obj_writer->arena, &code_sect->data, load_thunk_data);
  
  // patch lea with IAT entry
  static const U64 LEA_OPERAND_OFFSET = 3;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, load_thunk_data_offset + LEA_OPERAND_OFFSET, imp_addr_ptr, COFF_Reloc_X64_Rel32);
  
  // patch jmp __tailMerge_<DLL_NAME>
  static const U64 JMP_OPERAND_OFFSET = 8;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, load_thunk_data_offset + JMP_OPERAND_OFFSET, tail_merge, COFF_Reloc_X64_Rel32);

  // emit symbol
  String8         thunk_name        = push_str8f(obj_writer->arena, "__imp_load_%S", func_name);
  COFF_ObjSymbol *load_thunk_symbol = coff_obj_writer_push_symbol_extern(obj_writer, thunk_name, load_thunk_data_offset, code_sect);
  
  ProfEnd();
  return load_thunk_symbol;
}

internal COFF_ObjSymbol *
pe_make_tail_merge_thunk_x64(COFF_ObjWriter *obj_writer, COFF_ObjSection *code_sect, String8 dll_name, String8 delay_load_helper_name, COFF_ObjSymbol *dll_import_descriptor)
{
  ProfBeginFunction();
  
  static U8 tail_merge[] = {
    0x48, 0x89, 0x4C, 0x24, 0x08,                   // mov         qword ptr [rsp+8],rcx  
    0x48, 0x89, 0x54, 0x24, 0x10,                   // mov         qword ptr [rsp+10h],rdx  
    0x4C, 0x89, 0x44, 0x24, 0x18,                   // mov         qword ptr [rsp+18h],r8  
    0x4C, 0x89, 0x4C, 0x24, 0x20,                   // mov         qword ptr [rsp+20h],r9  
    0x48, 0x83, 0xEC, 0x68,                         // sub         rsp,68h  
    0x66, 0x0F, 0x7F, 0x44, 0x24, 0x20,             // movdqa      xmmword ptr [rsp+20h],xmm0  
    0x66, 0x0F, 0x7F, 0x4C, 0x24, 0x30,             // movdqa      xmmword ptr [rsp+30h],xmm1  
    0x66, 0x0F, 0x7F, 0x54, 0x24, 0x40,             // movdqa      xmmword ptr [rsp+40h],xmm2  
    0x66, 0x0F, 0x7F, 0x5C, 0x24, 0x50,             // movdqa      xmmword ptr [rsp+50h],xmm3  
    0x48, 0x8B, 0xD0,                               // mov         rdx,rax  
    0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00,       // lea         rcx,[__DELAY_IMPORT_DESCRIPTOR_<DLL_NAME>]  
    0xE8, 0x00, 0x00, 0x00, 0x00,                   // call        __delayLoadHelper2
    0x66, 0x0F, 0x6F, 0x44, 0x24, 0x20,             // movdqa      xmm0,xmmword ptr [rsp+20h]  
    0x66, 0x0F, 0x6F, 0x4C, 0x24, 0x30,             // movdqa      xmm1,xmmword ptr [rsp+30h]  
    0x66, 0x0F, 0x6F, 0x54, 0x24, 0x40,             // movdqa      xmm2,xmmword ptr [rsp+40h]  
    0x66, 0x0F, 0x6F, 0x5C, 0x24, 0x50,             // movdqa      xmm3,xmmword ptr [rsp+50h]  
    0x48, 0x8B, 0x4C, 0x24, 0x70,                   // mov         rcx,qword ptr [rsp+70h]  
    0x48, 0x8B, 0x54, 0x24, 0x78,                   // mov         rdx,qword ptr [rsp+78h]  
    0x4C, 0x8B, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, // mov         r8,qword ptr [rsp+80h]  
    0x4C, 0x8B, 0x8C, 0x24, 0x88, 0x00, 0x00, 0x00, // mov         r9,qword ptr [rsp+88h]  
    0x48, 0x83, 0xC4, 0x68,                         // add         rsp,68h  
    0xFF, 0xE0,                                     // jmp         rax
  };
  
  // emit tail merge chunk
  String8 tail_merge_data = push_str8_copy(obj_writer->arena, str8_array_fixed(tail_merge));
  U64     tail_merge_off  = code_sect->data.total_size;
  str8_list_push(obj_writer->arena, &code_sect->data, tail_merge_data);
  
  // patch lea __DELAY_IMPORT_DESCRIPTOR_<DLL_NAME>
  static const U64 LEA_OPERAND_OFFSET = 54;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, tail_merge_off + LEA_OPERAND_OFFSET, dll_import_descriptor, COFF_Reloc_X64_Rel32);

  COFF_ObjSymbol *delay_load_helper = coff_obj_writer_push_symbol_undef(obj_writer, delay_load_helper_name);
  
  // patch call __delayLoadHelper2
  static const U64 CALL_OPERAND_OFFSET = 59;
  coff_obj_writer_section_push_reloc(obj_writer, code_sect, tail_merge_off + CALL_OPERAND_OFFSET, delay_load_helper, COFF_Reloc_X64_Rel32);

  // emit symbol
  String8 tail_merge_name = push_str8f(obj_writer->arena, "__tailMerge_%S", dll_name);
  COFF_ObjSymbol *tail_merge_symbol = coff_obj_writer_push_symbol_extern(obj_writer, tail_merge_name, tail_merge_off, code_sect);
  
  ProfEnd();
  return tail_merge_symbol;
}

internal String8
pe_make_import_entry_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols)
{
  ProfBeginFunction();
  
  Assert(machine == COFF_MachineType_X64);
  Assert(str8_match_lit("dll", str8_skip_last_dot(dll_name), StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy));

  String8 dll_name_no_ext = str8_chop_last_dot(dll_name);

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  String8 dll_name_cstr = push_cstr(obj_writer->arena, dll_name);
  COFF_ObjSection *debugs = coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);
  COFF_ObjSection *idata2 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$2"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_zero());
  COFF_ObjSection *idata6 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$6"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, dll_name_cstr);

  coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("@comp.id"), 0x1018175, COFF_SymStorageClass_Static);
  {
    String8 symbol_name = push_str8f(arena, "__IMPORT_DESCRIPTOR_%S", dll_name_no_ext);
    coff_obj_writer_push_symbol_extern(obj_writer, symbol_name, 0, idata2);
  }
  COFF_ObjSymbol *idata2_symbol = coff_obj_writer_push_symbol_sect(obj_writer, idata2->name, idata2);
  COFF_ObjSymbol *idata6_symbol = coff_obj_writer_push_symbol_static(obj_writer, idata6->name, 0, idata6);
  COFF_ObjSymbol *idata4_symbol = coff_obj_writer_push_symbol_undef_sect(obj_writer, str8_lit(".idata$4"), COFF_SectionFlag_MemWrite|COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData);
  COFF_ObjSymbol *idata5_symbol = coff_obj_writer_push_symbol_undef_sect(obj_writer, str8_lit(".idata$5"), COFF_SectionFlag_MemWrite|COFF_SectionFlag_MemRead|COFF_SectionFlag_CntInitializedData);
  coff_obj_writer_push_symbol_undef(obj_writer, str8_lit("__NULL_IMPORT_DESCRIPTOR"));
  {
    String8 symbol_name = push_str8f(arena, "\x7f%S_NULL_THUNK_DATA", dll_name_no_ext);
    coff_obj_writer_push_symbol_undef(obj_writer, symbol_name);
  }

  {
    PE_ImportEntry *import_entry = push_array(obj_writer->arena, PE_ImportEntry, 1);
    str8_list_push(obj_writer->arena, &idata2->data, str8_struct(import_entry));
    coff_obj_writer_section_push_reloc(obj_writer, idata2, OffsetOf(PE_ImportEntry, name_voff),              idata6_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, idata2, OffsetOf(PE_ImportEntry, lookup_table_voff),      idata4_symbol, COFF_Reloc_X64_Addr32Nb);
    coff_obj_writer_section_push_reloc(obj_writer, idata2, OffsetOf(PE_ImportEntry, import_addr_table_voff), idata5_symbol, COFF_Reloc_X64_Addr32Nb);
  }

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);

  coff_obj_writer_release(&obj_writer);

  ProfEnd();
  return obj;
}

internal String8
pe_make_null_import_descriptor_obj(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols)
{
  ProfBeginFunction();

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  PE_ImportEntry *import_desc = push_array(obj_writer->arena, PE_ImportEntry, 1);
  COFF_ObjSection *debugs = coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);
  COFF_ObjSection *idata3 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$3"), PE_DATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_struct(import_desc));

  coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("@comp.id"), 0x01018175, COFF_SymStorageClass_Static);
  coff_obj_writer_push_symbol_extern(obj_writer, str8_lit("__NULL_IMPORT_DESCRIPTOR"), 0, idata3);

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);

  coff_obj_writer_release(&obj_writer);

  ProfEnd();
  return obj;
}

internal String8
pe_make_null_thunk_data_obj(Arena *arena, String8 dll_name, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 debug_symbols)
{
  ProfBeginFunction();
  
  Assert(str8_match_lit("dll", str8_skip_last_dot(dll_name), StringMatchFlag_CaseInsensitive|StringMatchFlag_RightSideSloppy));

  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  COFF_ObjSection *debugs = coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);
  COFF_ObjSection *idata4 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$4"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite, str8_zero());
  COFF_ObjSection *idata5 = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$5"), COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite, str8_zero());

  U64 import_size = coff_word_size_from_machine(machine);

  U8 *null_thunk  = push_array(obj_writer->arena, U8, import_size);
  U8 *null_lookup = push_array(obj_writer->arena, U8, import_size);

  str8_list_push(obj_writer->arena, &idata5->data, str8_array(null_thunk, import_size));
  str8_list_push(obj_writer->arena, &idata4->data, str8_array(null_lookup, import_size));

  idata4->flags |= coff_section_flag_from_align_size(import_size);
  idata5->flags |= coff_section_flag_from_align_size(import_size);

  coff_obj_writer_push_symbol_abs(obj_writer, str8_lit("@comp.id"), 0x1018175, COFF_SymStorageClass_Static);
  {
    String8 dll_name_no_ext = str8_chop_last_dot(dll_name);
    String8 symbol_name = push_str8f(arena, "\x7f%S_NULL_THUNK_DATA", dll_name_no_ext);
    coff_obj_writer_push_symbol_extern(obj_writer, symbol_name, 0, idata5);
  }

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  
  coff_obj_writer_release(&obj_writer);

  ProfEnd();
  return obj;
}

internal String8
pe_make_import_dll_obj_static(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 dll_name, String8 debug_symbols, String8List import_headers)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  U64 import_size = coff_word_size_from_machine(machine);
  COFF_SectionFlags import_align = coff_section_flag_from_align_size(import_size);

  PE_ImportEntry *impdesc = push_array(obj_writer->arena, PE_ImportEntry, 1);
  String8 dll_name_cstr = push_cstr(obj_writer->arena, dll_name);

  COFF_ObjSection *dll_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$2"), PE_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes,  str8_struct(impdesc));
  COFF_ObjSection *ilt_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$4"), PE_IDATA_SECTION_FLAGS|import_align,                  str8_zero());
  COFF_ObjSection *iat_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$5"), PE_IDATA_SECTION_FLAGS|import_align,                  str8_zero());
  COFF_ObjSection *int_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$6"), PE_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes,  str8_zero());
  COFF_ObjSection *dll_name_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".idata$7"), PE_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes,  dll_name_cstr);
  COFF_ObjSection *code_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".text$i"),  PE_TEXT_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes,   str8_zero());

  COFF_ObjSymbol *ilt_symbol      = coff_obj_writer_push_symbol_static(obj_writer, ilt_sect->name,      0, ilt_sect);
  COFF_ObjSymbol *iat_symbol      = coff_obj_writer_push_symbol_static(obj_writer, iat_sect->name,      0, iat_sect);
  COFF_ObjSymbol *dll_name_symbol = coff_obj_writer_push_symbol_static(obj_writer, dll_name_sect->name, 0, dll_name_sect);

  coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_ImportEntry, lookup_table_voff),      ilt_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_ImportEntry, name_voff),              dll_name_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_ImportEntry, import_addr_table_voff), iat_symbol);

  for (String8Node *import_header_n = import_headers.first; import_header_n != 0; import_header_n = import_header_n->next) {
    COFF_ParsedArchiveImportHeader  import_header = coff_archive_import_from_data(import_header_n->string);

    String8 iat_symbol_name = push_str8f(obj_writer->arena, "__imp_%S", import_header.func_name);
    U64     iat_offset      = iat_sect->data.total_size;
    U64     ilt_offset      = ilt_sect->data.total_size;
    U64     int_offset      = int_sect->data.total_size;

    COFF_ObjSymbol *iat_symbol = 0;
    switch (import_header.import_by) {
    case COFF_ImportBy_Ordinal: {
      String8 ordinal_data = coff_ordinal_data_from_hint(obj_writer->arena, import_header.machine, import_header.hint_or_ordinal);
      str8_list_push(obj_writer->arena, &ilt_sect->data, ordinal_data);
      str8_list_push(obj_writer->arena, &iat_sect->data, ordinal_data);

      iat_symbol = coff_obj_writer_push_symbol_extern(obj_writer, iat_symbol_name, iat_offset, iat_sect);
    } break;
    case COFF_ImportBy_Name: {
      // put together name look up entry
      String8 int_data = coff_make_import_lookup(obj_writer->arena, import_header.hint_or_ordinal, import_header.func_name);
      str8_list_push(obj_writer->arena, &int_sect->data, int_data);

      // create symbol for lookup entry
      COFF_ObjSymbol *int_symbol = coff_obj_writer_push_symbol_static(obj_writer, int_sect->name, int_offset, int_sect);

      // in the file IAT mirrors ILT, dynamic linker later overwrites it with imported function addresses
      U64 import_size  = coff_word_size_from_machine(import_header.machine);
      U8 *import_entry = push_array(obj_writer->arena, U8, import_size);
      str8_list_push(obj_writer->arena, &ilt_sect->data, str8_array(import_entry, import_size));
      str8_list_push(obj_writer->arena, &iat_sect->data, str8_array(import_entry, import_size));

      iat_symbol = coff_obj_writer_push_symbol_extern(obj_writer, iat_symbol_name, iat_offset, iat_sect);

      // patch IAT and ILT
      coff_obj_writer_section_push_reloc_voff(obj_writer, ilt_sect, ilt_offset, int_symbol);
      coff_obj_writer_section_push_reloc_voff(obj_writer, iat_sect, iat_offset, int_symbol);
    } break;
    case COFF_ImportBy_Undecorate: {
      NotImplemented;
    } break;
    case COFF_ImportBy_NameNoPrefix: {
      NotImplemented;
    } break;
    }

    // emit thunks
    COFF_ObjSymbol *jmp_thunk_symbol = 0;
    if (import_header.type == COFF_ImportHeader_Code) {
      switch (import_header.machine) {
      case COFF_MachineType_Unknown: {} break;
      case COFF_MachineType_X64:     { jmp_thunk_symbol = pe_make_indirect_jump_thunk_x64(obj_writer, code_sect, iat_symbol, import_header.func_name); } break;
      default:                       { NotImplemented; } break;
      }
    }
  }

  String8 dll_obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);
  return dll_obj;
}

internal String8
pe_make_import_dll_obj_delayed(Arena *arena, COFF_TimeStamp time_stamp, COFF_MachineType machine, String8 dll_name, String8 delay_load_helper_name, String8 debug_symbols, String8List import_headers, B32 emit_biat, B32 emit_uiat)
{
  COFF_ObjWriter *obj_writer = coff_obj_writer_alloc(time_stamp, machine);

  // import descriptor
  PE_DelayedImportEntry *impdesc = push_array(obj_writer->arena, PE_DelayedImportEntry, 1);
  impdesc->attributes = 1;

  // DLL name cstring
  String8 dll_name_cstr = push_cstr(obj_writer->arena, dll_name);

  // DLL handle
  U64 handle_size = coff_word_size_from_machine(machine);
  U8 *handle = push_array(obj_writer->arena, U8, handle_size);

  // import align
  U64 import_size = coff_word_size_from_machine(machine);
  COFF_SectionFlags import_align = coff_section_flag_from_align_size(import_size);

  // push sections
  COFF_ObjSection *dll_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".didat$2"), PE_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align4Bytes, str8_struct(impdesc));
  COFF_ObjSection *ilt_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".didat$4"), PE_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *iat_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".didat$5"), PE_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *int_sect      = coff_obj_writer_push_section(obj_writer, str8_lit(".didat$6"), PE_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, str8_zero());
  COFF_ObjSection *dll_name_sect = coff_obj_writer_push_section(obj_writer, str8_lit(".didat$7"), PE_IDATA_SECTION_FLAGS|COFF_SectionFlag_Align2Bytes, dll_name_cstr);
  COFF_ObjSection *biat_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".didat$8"), PE_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *uiat_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".didat$9"), PE_IDATA_SECTION_FLAGS|import_align,                 str8_zero());
  COFF_ObjSection *code_sect     = coff_obj_writer_push_section(obj_writer, str8_lit(".text$i"),  PE_TEXT_SECTION_FLAGS,                               str8_zero());
  COFF_ObjSection *handle_sect   = coff_obj_writer_push_section(obj_writer, str8_lit(".data$h"),  PE_DATA_SECTION_FLAGS,                               str8_array(handle, handle_size));
  COFF_ObjSection *debug_sect    = coff_obj_writer_push_section(obj_writer, str8_lit(".debug$S"), PE_DEBUG_SECTION_FLAGS|COFF_SectionFlag_Align1Bytes, debug_symbols);

  // sections symbols
  COFF_ObjSymbol *dll_symbol      = coff_obj_writer_push_symbol_static(obj_writer, dll_sect->name,      0, dll_sect);
  COFF_ObjSymbol *dll_name_symbol = coff_obj_writer_push_symbol_static(obj_writer, dll_name_sect->name, 0, dll_name_sect);
  COFF_ObjSymbol *handle_symbol   = coff_obj_writer_push_symbol_static(obj_writer, handle_sect->name,   0, handle_sect);
  COFF_ObjSymbol *iat_symbol      = coff_obj_writer_push_symbol_static(obj_writer, iat_sect->name,      0, iat_sect);
  COFF_ObjSymbol *ilt_symbol      = coff_obj_writer_push_symbol_static(obj_writer, ilt_sect->name,      0, ilt_sect);

  // patch virutal offsets in import header
  coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, name_voff),          dll_name_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, module_handle_voff), handle_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, iat_voff),           iat_symbol);
  coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, name_table_voff),    ilt_symbol);

  // patch BIAT virtual offset in import header
  if (emit_biat) {
    COFF_ObjSymbol *biat_symbol = coff_obj_writer_push_symbol_static(obj_writer, biat_sect->name, 0, biat_sect);
    coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, bound_table_voff), biat_symbol);
  }

  // patch UIAT virtual offset in import header
  if (emit_uiat) {
    COFF_ObjSymbol *uiat_symbol = coff_obj_writer_push_symbol_static(obj_writer, uiat_sect->name, 0, uiat_sect);
    coff_obj_writer_section_push_reloc_voff(obj_writer, dll_sect, OffsetOf(PE_DelayedImportEntry, unload_table_voff), uiat_symbol);
  }
  
  // emit tail merge
  COFF_ObjSymbol *tail_merge_symbol = 0;
  switch (machine) {
  case COFF_MachineType_Unknown: {} break;
  case COFF_MachineType_X64:     { tail_merge_symbol = pe_make_tail_merge_thunk_x64(obj_writer, code_sect, dll_name, delay_load_helper_name, dll_symbol); } break;
  default:                       { NotImplemented; } break;
  }

  for (String8Node *import_header_n = import_headers.first; import_header_n != 0; import_header_n = import_header_n->next) {
    COFF_ParsedArchiveImportHeader import_header = coff_archive_import_from_data(import_header_n->string);

    // emit thunks
    COFF_ObjSymbol *jmp_thunk_symbol  = 0;
    COFF_ObjSymbol *load_thunk_symbol = 0;
    if (import_header.type == COFF_ImportHeader_Code) {
      switch (machine) {
      case COFF_MachineType_X64: {
        String8 iat_symbol_name = push_str8f(obj_writer->arena, "__imp_%S", import_header.func_name);
        coff_obj_writer_push_symbol_undef(obj_writer, iat_symbol_name);

        // emit jmp thunk
        jmp_thunk_symbol = pe_make_indirect_jump_thunk_x64(obj_writer, code_sect, iat_symbol, import_header.func_name);

        // emit load thunk
        load_thunk_symbol  = pe_make_load_thunk_x64(obj_writer, code_sect, iat_symbol, tail_merge_symbol, import_header.func_name);
      } break;
      default: { NotImplemented; } break;
      }
    }

    switch (import_header.import_by) {
    case COFF_ImportBy_Ordinal: {
      U64     iat_offset   = iat_sect->data.total_size;
      String8 ordinal_data = coff_ordinal_data_from_hint(obj_writer->arena, import_header.machine, import_header.hint_or_ordinal);
      str8_list_push(obj_writer->arena, &ilt_sect->data, ordinal_data);
      str8_list_push(obj_writer->arena, &iat_sect->data, ordinal_data);

      String8 iat_symbol_name = push_str8f(obj_writer->arena, "__imp_%S", import_header.func_name);
      iat_symbol = coff_obj_writer_push_symbol_extern(obj_writer, iat_symbol_name, iat_offset, iat_sect);


      if (emit_biat) {
        U64 import_size = coff_word_size_from_machine(machine);
        U64 biat_offset = biat_sect->data.total_size;
        str8_list_push(obj_writer->arena, &biat_sect->data, str8(0,import_size));
        coff_obj_writer_section_push_reloc_addr(obj_writer, biat_sect, biat_offset, load_thunk_symbol);
      }
      if (emit_uiat) {
        U64 import_size = coff_word_size_from_machine(machine);
        U64 uiat_offset = uiat_sect->data.total_size;
        str8_list_push(obj_writer->arena, &biat_sect->data, str8(0,import_size));
        coff_obj_writer_section_push_reloc_addr(obj_writer, uiat_sect, uiat_offset, load_thunk_symbol);
      }
    } break;
    case COFF_ImportBy_Name: {
      // put together name look up entry
      String8 int_data = coff_make_import_lookup(obj_writer->arena, import_header.hint_or_ordinal, import_header.func_name);
      U64 int_data_offset = int_sect->data.total_size;
      str8_list_push(obj_writer->arena, &int_sect->data, int_data);

      // create symbol for lookup chunk
      String8 int_symbol_name = push_str8f(obj_writer->arena, "%S.%S.name.delayed", dll_name, import_header.func_name);
      COFF_ObjSymbol *int_symbol = coff_obj_writer_push_symbol_static(obj_writer, int_symbol_name, int_data_offset, int_sect);

      U64 import_size = coff_word_size_from_machine(machine);

      // dynamic linker patches this voff on DLL load event
      U64 ilt_data_offset = ilt_sect->data.total_size;
      str8_list_push(obj_writer->arena, &ilt_sect->data, str8(0, import_size));

      // patch-in ILT with import voff
      coff_obj_writer_section_push_reloc_voff(obj_writer, ilt_sect, ilt_data_offset, int_symbol);

      // in the file IAT mirrors ILT, dynamic linker later overwrites it with imported function addresses.
      U64 iat_data_offset = iat_sect->data.total_size;
      str8_list_push(obj_writer->arena, &iat_sect->data, str8(0, import_size));

      // patch-in thunk address
      coff_obj_writer_section_push_reloc_addr(obj_writer, iat_sect, iat_data_offset, load_thunk_symbol);

      if (emit_biat) {
        U64 biat_data_offset = biat_sect->data.total_size;
        str8_list_push(obj_writer->arena, &biat_sect->data, str8(0, import_size));

        // patch-in thunk address
        coff_obj_writer_section_push_reloc_addr(obj_writer, biat_sect, biat_data_offset, load_thunk_symbol);
      }

      if (emit_uiat) {
        U64 uiat_data_offset = uiat_sect->data.total_size;
        str8_list_push(obj_writer->arena, &uiat_sect->data, str8(0, import_size));

        // patch-in thunk address
        coff_obj_writer_section_push_reloc_addr(obj_writer, uiat_sect, uiat_data_offset, load_thunk_symbol);
      }
    } break;
    case COFF_ImportBy_Undecorate: { NotImplemented; } break;
    case COFF_ImportBy_NameNoPrefix: { NotImplemented; } break;
    }
  }

  String8 obj = coff_obj_writer_serialize(arena, obj_writer);
  coff_obj_writer_release(&obj_writer);
  return obj;
}
