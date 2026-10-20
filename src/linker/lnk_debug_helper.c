// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
lnk_make_linker_compile3(Arena *arena, COFF_MachineType machine)
{
  return cv_make_comp3(arena,
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
}

internal String8
lnk_make_debug_s(Arena *arena, String8List symbols)
{
  Temp scratch = scratch_begin(&arena, 1);

  CV_DebugS debug_s = {0};
  if (symbols.total_size) {
    String8 data = cv_debug_s_alloc_subsection(scratch.arena, &debug_s, CV_C13SubSectionKind_Symbols, symbols.total_size);
    U64 cursor = 0;
    for EachNode(n, String8Node, symbols.first) {
      cursor += str8_write(data, cursor, n->string);
    }
  }
  String8List debug_s_data_list = cv_data_from_debug_s_c13(scratch.arena, &debug_s, 1);
  String8     debug_s_data      = str8_list_join(arena, &debug_s_data_list, 0);

  if (symbols.total_size > 0) {
    U64     symbols_offset = sizeof(CV_Signature) + sizeof(CV_C13SubSectionHeader);
    String8 raw_symbols    = str8_substr(debug_s_data, r1u64s(symbols_offset, symbols.total_size));
    cv_patch_symbol_tree_offsets(raw_symbols, sizeof(CV_Signature), CV_SymbolAlign);
  }

  scratch_end(scratch);
  return debug_s_data;
}

internal U64
lnk_size_stripped_symbols(String8 symbols, U64 align)
{
  U64 size = 0;
  for (U64 cursor = 0; cursor < symbols.size; ) {
    CV_Symbol symbol = {0};
    TryReadBreak(cv_read_symbol(symbols, cursor, align, &symbol), cursor);
    if (cv_is_lproc(symbol)) {
      size += cv_size_from_symbol(&symbol, PDB_SYMBOL_ALIGN);
      size += AlignPow2(sizeof(CV_SymbolHeader), PDB_SYMBOL_ALIGN); // S_END
    }
  }
  return size;
}

internal String8
lnk_make_linker_debug_symbols(Arena *arena, COFF_MachineType machine)
{
  Temp scratch = scratch_begin(&arena, 1);

  String8List symbols = {0};
  str8_list_push(scratch.arena, &symbols, cv_make_symbol(scratch.arena, CV_SymKind_COMPILE3, lnk_make_linker_compile3(scratch.arena, machine)));
  String8 debug_symbols = lnk_make_debug_s(arena, symbols);

  scratch_end(scratch);
  return debug_symbols;
}

internal String8
lnk_make_dll_import_debug_symbols(Arena *arena, COFF_MachineType machine, String8 dll_name)
{
  Temp scratch = scratch_begin(&arena,1);

  String8List symbols = {0};
  str8_list_push(scratch.arena, &symbols, cv_make_symbol(scratch.arena, CV_SymKind_OBJNAME, cv_make_obj_name(scratch.arena, dll_name, 0)));
  str8_list_push(scratch.arena, &symbols, cv_make_symbol(scratch.arena, CV_SymKind_COMPILE3, lnk_make_linker_compile3(scratch.arena, machine)));

  // TODO: add thunks

  // serialize symbols
  String8 debug_symbols = lnk_make_debug_s(arena, symbols);

  scratch_end(scratch);
  return debug_symbols;
}
