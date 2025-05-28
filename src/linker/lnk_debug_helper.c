// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
lnk_make_linker_compile3(Arena *arena, COFF_MachineType machine)
{
  String8 comp3_data = cv_make_comp3(arena,
                                     0,
                                     CV_Language_LINK,
                                     cv_arch_from_coff_machine(machine),
                                     /* ver_fe_major */ 0,
                                     /* ver_fe_minor */ 0,
                                     /* ver_fe_build */ 0,
                                     /* ver_feqfe    */ 0,
                                     /* ver_major    */ 14,
                                     /* ver_minor    */ 36,
                                     /* ver_build    */ 32537,
                                     /* ver_qfe      */ 0,
                                     str8_lit(BUILD_TITLE));
  return comp3_data;
}

internal String8
lnk_make_debug_s(Arena *arena, CV_SymbolList symbol_list)
{
  Temp scratch = scratch_begin(&arena, 1);

  CV_DebugS debug_s = {0};
  String8List *symbol_list_ptr = cv_sub_section_ptr_from_debug_s(&debug_s, CV_C13SubSectionKind_Symbols);
  *symbol_list_ptr = cv_data_from_symbol_list(scratch.arena, symbol_list, CV_SymbolAlign);

  String8List debug_s_data_list = cv_data_c13_from_debug_s(scratch.arena, &debug_s, 1);
  String8     debug_s_data      = str8_list_join(arena, &debug_s_data_list, 0);

  scratch_end(scratch);
  return debug_s_data;
}

internal String8
lnk_make_linker_debug_symbols(Arena *arena, COFF_MachineType machine)
{
  Temp scratch = scratch_begin(&arena, 1);
  CV_SymbolList symbol_list = { .signature = CV_Signature_C13 };
  String8       comp3_data  = lnk_make_linker_compile3(scratch.arena, machine);
  cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_COMPILE3, comp3_data);
  String8 debug_symbols = lnk_make_debug_s(arena, symbol_list);
  scratch_end(scratch);
  return debug_symbols;
}

internal String8
lnk_make_dll_import_debug_symbols(Arena *arena, COFF_MachineType machine, String8 dll_name)
{
  Temp scratch = scratch_begin(&arena,1);

  CV_SymbolList symbol_list = { .signature = CV_Signature_C13 };

  // S_OBJ
  String8 obj_data = cv_make_obj_name(scratch.arena, dll_name, 0);
  cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_OBJNAME, obj_data);

  // S_COMPILE3
  String8 comp3_data = lnk_make_linker_compile3(scratch.arena, machine);
  cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_COMPILE3, comp3_data);

  // S_END
  //cv_symbol_list_push_data(scratch.arena, &symbol_list, CV_SymKind_END, str8_zero());

  // TODO: add thunks

  // serialize symbols
  String8 debug_symbols = lnk_make_debug_s(arena, symbol_list);

  scratch_end(scratch);
  return debug_symbols;
}


