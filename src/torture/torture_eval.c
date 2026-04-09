#define T_Group "eval"

T_BeginTest(eval_compiler_basics)
{
  E_Cache *eval_cache = e_cache_alloc();
  e_select_cache(eval_cache);
  E_BaseCtx *base_ctx = push_array(arena, E_BaseCtx, 1);
  e_select_base_ctx(base_ctx);
  E_IRCtx *ir_ctx = push_array(arena, E_IRCtx, 1);
  e_select_ir_ctx(ir_ctx);
  E_InterpretCtx *interpret_ctx = push_array(arena, E_InterpretCtx, 1);
  e_select_interpret_ctx(interpret_ctx, 0, 0);

  String8 exprs[] =
  {
    str8_lit("123"),
    str8_lit("1 + 2"),
    str8_lit("foo"),
    str8_lit("foo(bar)"),
    str8_lit("foo(bar(baz))"),
  };
  String8List logs = {0};
  for EachElement(idx, exprs)
  {
    String8 log = e_debug_log_from_expr_string(arena, exprs[idx]);
    str8_list_push(arena, &logs, log);
  }
  String8 log = str8_list_join(arena, &logs, 0);

  String8 expected_log = str8_lit(
"`123`\n"
"    tokens:\n"
"        Numeric: `123`\n"
"    expr:\n"
"        LeafU64 (123)\n"
"    type:\n"
"        int32\n"
"    ir_tree:\n"
"        ConstU8 (123)\n"
"\n"
"`1 + 2`\n"
"    tokens:\n"
"        Numeric: `1`\n"
"        Symbol: `+`\n"
"        Numeric: `2`\n"
"    expr:\n"
"        Add\n"
"            LeafU64 (1)\n"
"            LeafU64 (2)\n"
"    type:\n"
"        int32\n"
"    ir_tree:\n"
"        Add (1026)\n"
"            ConstU8 (1)\n"
"            ConstU8 (2)\n"
"\n"
"`foo`\n"
"    tokens:\n"
"        Identifier: `foo`\n"
"    expr:\n"
"        LeafIdentifier (`foo`)\n"
"    type:\n"
"    ir_tree:\n"
"        Stop\n"
"\n"
"`foo(bar)`\n"
"    tokens:\n"
"        Identifier: `foo`\n"
"        Symbol: `(`\n"
"        Identifier: `bar`\n"
"        Symbol: `)`\n"
"    expr:\n"
"        Call\n"
"            LeafIdentifier (`foo`)\n"
"            LeafIdentifier (`bar`)\n"
"    type:\n"
"    ir_tree:\n"
"        Stop\n"
"\n"
"`foo(bar(baz))`\n"
"    tokens:\n"
"        Identifier: `foo`\n"
"        Symbol: `(`\n"
"        Identifier: `bar`\n"
"        Symbol: `(`\n"
"        Identifier: `baz`\n"
"        Symbol: `)`\n"
"        Symbol: `)`\n"
"    expr:\n"
"        Call\n"
"            LeafIdentifier (`foo`)\n"
"            Call\n"
"                LeafIdentifier (`bar`)\n"
"                LeafIdentifier (`baz`)\n"
"    type:\n"
"    ir_tree:\n"
"        Stop\n"
"\n"
);

  T_Ok(str8_match(log, expected_log, 0));
}

#undef T_Group

