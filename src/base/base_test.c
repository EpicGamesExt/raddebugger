// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if BUILD_TESTS

internal String8
test_build_exe_path(Arena *arena, String8 name)
{
  String8 folder = get_process_info()->binary_path;
  String8 path = str8f(arena, "%S/%S%S", folder, name, program_ext_postfix_from_os(OperatingSystem_CURRENT, 0));
  return path;
}

internal String8
test_input_path(Arena *arena, TestCtx *ctx, String8 name)
{
  String8 path = str8f(arena, "%S/%S", ctx->input_data_path, name);
  return path;
}

internal String8
test_input_exe_path(Arena *arena, TestCtx *ctx, String8 name)
{
  String8 path = str8f(arena, "%S/%S%S", ctx->input_data_path, name, program_ext_postfix_from_os(OperatingSystem_CURRENT, 1));
  return path;
}

internal String8
test_exemplar_path(Arena *arena, TestCtx *ctx, String8 name)
{
  String8 path = str8f(arena, "%S/%S", ctx->exemplars_path, name);
  return path;
}

internal String8
test_layer_from_file_path(String8 file_path)
{
  U64 src_min = str8_find_needle(file_path, 0, s("src/"), StringMatchFlag_SlashInsensitive);
  if (src_min == file_path.size) { return str8_zero(); }
  U64 src_max = src_min + str8_lit("src/").size;
  U64 layer_slash_max = str8_find_needle(file_path, src_max, s("/"), StringMatchFlag_SlashInsensitive);
  if (layer_slash_max == file_path.size) { return str8_zero(); }
  return str8_substr(file_path, r1u64(src_max, layer_slash_max));
}

internal void
base_register_test(char *func_name_cstr, TestFunctionType *fn, char *file_path_cstr, int line, int skip)
{
  String8 file_path = str8_cstring(file_path_cstr);
  TestInfo *t = &test_infos[test_infos_count++];
  t->layer     = test_layer_from_file_path(file_path);
  t->label     = str8_cstring(func_name_cstr);
  t->decl_line = line;
  t->skip      = skip;
  t->test_fn   = fn;
}

#endif
