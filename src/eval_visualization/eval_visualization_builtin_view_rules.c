// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: View Rule Tree Info Extraction Helpers

internal U64
ev_base_offset_from_eval(E_Eval eval)
{
  if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(eval.type_key)))
  {
    eval = e_value_eval_from_eval(eval);
  }
  return eval.value.u64;
}

internal E_Value
ev_value_from_params(MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  String8 expr = md_string_from_children(scratch.arena, params);
  E_Eval eval = e_eval_from_string(scratch.arena, expr);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  scratch_end(scratch);
  return value_eval.value;
}

internal E_TypeKey
ev_type_key_from_params(MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  String8 expr = md_string_from_children(scratch.arena, params);
  E_TokenArray tokens = e_token_array_from_text(scratch.arena, expr);
  E_Parse parse = e_parse_type_from_text_tokens(scratch.arena, expr, &tokens);
  E_TypeKey type_key = e_type_from_expr(parse.expr);
  scratch_end(scratch);
  return type_key;
}

internal E_Value
ev_value_from_params_key(MD_Node *params, String8 key)
{
  Temp scratch = scratch_begin(0, 0);
  MD_Node *key_node = md_child_from_string(params, key, 0);
  String8 expr = md_string_from_children(scratch.arena, key_node);
  E_Eval eval = e_eval_from_string(scratch.arena, expr);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  scratch_end(scratch);
  return value_eval.value;
}

internal Rng1U64
ev_range_from_eval_params(E_Eval eval, MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  U64 size = ev_value_from_params_key(params, str8_lit("size")).u64;
  E_TypeKey type_key = e_type_unwrap(eval.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(eval.type_key));
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  if(size == 0 && e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                               direct_type_kind == E_TypeKind_Union ||
                                                               direct_type_kind == E_TypeKind_Class ||
                                                               direct_type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(e_type_direct_from_key(e_type_unwrap(eval.type_key)));
  }
  if(size == 0 && eval.mode == E_Mode_Offset && (type_kind == E_TypeKind_Struct ||
                                                 type_kind == E_TypeKind_Union ||
                                                 type_kind == E_TypeKind_Class ||
                                                 type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(e_type_unwrap(eval.type_key));
  }
  if(size == 0)
  {
    size = 16384;
  }
  Rng1U64 result = {0};
  result.min = ev_base_offset_from_eval(eval);
  result.max = result.min + size;
  scratch_end(scratch);
  return result;
}

internal Arch
ev_arch_from_eval_params(E_Eval eval, MD_Node *params)
{
  Arch arch = Arch_Null;
  MD_Node *arch_node = md_child_from_string(params, str8_lit("arch"), 0);
  String8 arch_kind_string = arch_node->first->string;
  if(str8_match(arch_kind_string, str8_lit("x64"), StringMatchFlag_CaseInsensitive))
  {
    arch = Arch_x64;
  }
  return arch;
}

////////////////////////////////
//~ rjf: "list"

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(list)
{
  EV_ExpandInfo info = {0};
  return info;
}
