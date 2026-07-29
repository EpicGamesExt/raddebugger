// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

Test(eval2_regressions)
{
  E2_ConsTypeMap *cons_types = e2_cons_type_map_alloc();
  e2_select_cons_type_map(cons_types);
  String8 strings[] =
  {
    s("int32(*)(int32, int32)"),
    s("int32 *(*)(int32, int32)"),
    s("int32 **(*)(int32, int32)"),
    s("123(1, 2, 3)"),
    s("int32 (*) [100]"),
    s("(3 * 4) + 2"),
    s("cast (float32) 123"),
    s("int32 *"),
    s("3 * 4 + 2"),
    s("int32[100]"),
    s("3 * 4"),
    s("foo"),
    s("foo.bar"),
    s("[123]"),
    s("123[456]"),
    s("2 + (3 * 4)"),
    s("int32 == int32"),
    s("123, 456"),
    s("222.f"),
    s("123 as float32"),
    s("cast float32 123"),
    s("(int32 *)123"),
    s("bar(a, b) = a + b"),
    s("foo = 123"),
    s("1 > 2"),
    s("1 ? 123 : 456"),
    s("0 ? 123 : 456"),
    s("1 ? \"Test\" : 888"),
    s("'a'"),
    s("'b'"),
    s("(float32)222"),
    s("(float64)222"),
    s("222.0"),
    s("123"),
    s("123 + 456"),
    s("!1"),
    s("!0"),
    s("1 + 1 + 1"),
    s("40 / 0"),
    s("123 | 456"),
    s("123 + "),
    s("-123"),
    s("sizeof 123"),
  };
  for EachElement(idx, strings)
  {
    Temp scratch = scratch_begin(0, 0);
    String8List msgs = {0};
    
    // rjf: string -> expr
    E2_Expr *expr = &e2_expr_nil;
    {
      E2_ParseState state = {0};
      B32 identifier_is_type = 0;
      for(;;)
      {
        E2_Parse parse = e2_parse_from_string(scratch.arena, &state, identifier_is_type, E2_LangKind_CLike, strings[idx]);
        identifier_is_type = 0;
        expr = parse.expr;
        for EachNode(n, E2_Msg, parse.msgs.first)
        {
          str8_list_push(scratch.arena, &msgs, n->string);
        }
        if(parse.status == E2_ParseStatus_CheckIdentifierIsType)
        {
          identifier_is_type = identifier_is_type || str8_match(parse.identifier, s("int32"), 0);
          identifier_is_type = identifier_is_type || str8_match(parse.identifier, s("float32"), 0);
          identifier_is_type = identifier_is_type || str8_match(parse.identifier, s("float64"), 0);
        }
        if(e2_parse_status_is_terminal(parse.status))
        {
          break;
        }
      }
    }
    
    // rjf: expr -> ir tree
    E2_IRNode *irtree = &e2_irnode_nil;
    {
      E2_IRNode *resolve_result = &e2_irnode_nil;
      E2_Val compile_time_eval_result = {0};
      E2_CompileState state = {0};
      for(;;)
      {
        E2_Compile compile = e2_compile_from_expr(scratch.arena, &state, resolve_result, compile_time_eval_result, expr);
        irtree = compile.irtree;
        resolve_result = &e2_irnode_nil;
        if(compile.status == E2_CompileStatus_MissedIdentifierResolution)
        {
          E2_IRNode *irnode = &e2_irnode_nil;
          if(str8_match(compile.identifier, s("float32"), 0))
          {
            irnode = e2_irnode_type(scratch.arena, e2_type_key_basic(E2_TypeKind_F32));
          }
          else if(str8_match(compile.identifier, s("float64"), 0))
          {
            irnode = e2_irnode_type(scratch.arena, e2_type_key_basic(E2_TypeKind_F64));
          }
          else if(str8_match(compile.identifier, s("int32"), 0))
          {
            irnode = e2_irnode_type(scratch.arena, e2_type_key_basic(E2_TypeKind_S32));
          }
          else if(str8_match(compile.identifier, s("int64"), 0))
          {
            irnode = e2_irnode_type(scratch.arena, e2_type_key_basic(E2_TypeKind_S64));
          }
          else
          {
            irnode = e2_irnode_const_u64_or_smaller(scratch.arena, 123);
          }
          resolve_result = irnode;
        }
        if(compile.status == E2_CompileStatus_MemberAccess)
        {
          resolve_result = e2_irnode_const_u64_or_smaller(scratch.arena, 456);
        }
        if(compile.status == E2_CompileStatus_IndexAccess)
        {
          resolve_result = e2_irnode_const_u64_or_smaller(scratch.arena, 111);
        }
        if(compile.status == E2_CompileStatus_Call)
        {
          resolve_result = e2_irnode_const_u64_or_smaller(scratch.arena, 123456);
        }
        if(compile.status == E2_CompileStatus_CompileTimeEval)
        {
          String8 bytecode = e2_bytecode_from_irnode(scratch.arena, irtree);
          E2_InterpState interp_state = {0};
          E2_SpaceMap space_map = {0};
          E2_Interp interp = e2_interp_from_bytecode(scratch.arena, &interp_state, &space_map, bytecode);
          compile_time_eval_result = interp.val;
        }
        if(e2_compile_status_is_terminal(compile.status))
        {
          break;
        }
      }
    }
    
    // rjf: ir tree -> bytecode
    String8 bytecode = e2_bytecode_from_irnode(scratch.arena, irtree);
    
    // rjf: bytecode -> value
    E2_Val val = {0};
    {
      E2_InterpState state = {0};
      E2_SpaceMap space_map = {0};
      for(;;)
      {
        E2_Interp interp = e2_interp_from_bytecode(scratch.arena, &state, &space_map, bytecode);
        val = interp.val;
        if(e2_interp_status_is_terminal(interp.status))
        {
          if(interp.status != E2_InterpStatus_Good)
          {
            str8_list_pushf(scratch.arena, &msgs, "Interpretation error (%i).", interp.status);
          }
          break;
        }
      }
    }
    
    // rjf: message string list -> string
    StringJoin join = {.sep = s(" ")};
    String8 msgs_string = str8_list_join(scratch.arena, &msgs, &join);
    
    // rjf: log
    String8 log = str8f(scratch.arena, "%S -> %I64d (f32: %f) (f64: %f) *%s* (type: %S) %s%S\n",
                        strings[idx],
                        val.s64,
                        val.f32,
                        val.f64,
                        irtree->mode == E2_Mode_Type ? "type" :
                        irtree->mode == E2_Mode_Value ? "value" :
                        "address",
                        e2_string_from_type_key(scratch.arena, irtree->type_key),
                        msgs_string.size != 0 ? " // " : "", msgs_string);
    log_info(log);
    
    scratch_end(scratch);
  }
}
