// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
t_script_test_log_result(Arena *arena, TestCtx *ctx, T_Result result)
{
  for EachNode(diagnostic, T_Diagnostic, result.diagnostics.first) {
    String8 operation = diagnostic->operation.size == 0 ? str8_zero() : str8f(arena, " [%S]", diagnostic->operation);
    test_outf("%S:%lld:%lld%S: %S\n", diagnostic->file_path, diagnostic->location.line, diagnostic->location.column, operation, diagnostic->message);
  }
}

TEST(script_path_identity)
{
  T_Ok(str8_match(test_layer_from_file_path(str8_lit("C:/repo/src/linker/tests/foo.tst")), str8_lit("linker"), 0));
  T_Ok(str8_match(test_layer_from_file_path(str8_lit("C:\\repo\\src\\torture\\tests\\foo.c")), str8_lit("torture"), 0));
  T_Ok(test_layer_from_file_path(str8_lit("C:/repo/tests/foo.tst")).size == 0);
}
