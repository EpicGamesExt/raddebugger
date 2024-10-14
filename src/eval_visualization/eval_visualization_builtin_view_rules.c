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
//~ rjf: default

typedef struct EV_DefaultExpandAccel EV_DefaultExpandAccel;
struct EV_DefaultExpandAccel
{
  E_MemberArray members;
  E_EnumValArray enum_vals;
  U64 array_count;
  B32 array_need_extra_deref;
  B32 is_ptr2ptr;
};

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(default)
{
  Temp scratch = scratch_begin(&arena, 1);
  U64 total_row_count = 0;
  EV_DefaultExpandAccel *accel = push_array(arena, EV_DefaultExpandAccel, 1);
  
  ////////////////////////////
  //- rjf: unpack expression type info
  //
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKey type_key = e_type_unwrap(irtree.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(type_key));
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  
  ////////////////////////////
  //- rjf: structs/unions/classes -> expansions generate rows for all members
  //
  if((type_kind == E_TypeKind_Struct ||
      type_kind == E_TypeKind_Union ||
      type_kind == E_TypeKind_Class) ||
     (e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                   direct_type_kind == E_TypeKind_Union ||
                                                   direct_type_kind == E_TypeKind_Class)))
  {
    E_TypeKey struct_type_key = e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key;
    accel->members = e_type_data_members_from_key__cached(struct_type_key);
    total_row_count = accel->members.count;
  }
  
  ////////////////////////////
  //- rjf: enums -> expansions generate rows for all members
  //
  else if(type_kind == E_TypeKind_Enum ||
          (e_type_kind_is_pointer_or_ref(type_kind) && direct_type_kind == E_TypeKind_Enum))
  {
    E_Type *type = e_type_from_key(arena, e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key);
    accel->enum_vals.v = type->enum_vals;
    accel->enum_vals.count = type->count;
    total_row_count = accel->enum_vals.count;
  }
  
  ////////////////////////////
  //- rjf: arrays -> expansions generate rows for all elements
  //
  else if(type_kind == E_TypeKind_Array ||
          (e_type_kind_is_pointer_or_ref(type_kind) && direct_type_kind == E_TypeKind_Array))
  {
    B32 need_extra_deref = e_type_kind_is_pointer_or_ref(type_kind);
    E_Expr *array_expr = need_extra_deref ? e_expr_ref_deref(arena, expr) : expr;
    E_Type *type = e_type_from_key(arena, need_extra_deref ? direct_type_key : type_key);
    total_row_count = type->count;
    accel->array_count = type->count;
    accel->array_need_extra_deref = need_extra_deref;
  }
  
  ////////////////////////////
  //- rjf: pointer-to-pointer -> expansions generate dereference
  //
  else if(e_type_kind_is_pointer_or_ref(type_kind) && e_type_kind_is_pointer_or_ref(direct_type_kind))
  {
    total_row_count = 1;
    accel->is_ptr2ptr = 1;
  }
  
  ////////////////////////////
  //- rjf: package result
  //
  EV_ExpandInfo result = {0};
  {
    result.user_data = accel;
    result.row_count = total_row_count;
  }
  
  scratch_end(scratch);
  return result;
}

EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(default)
{
  EV_DefaultExpandAccel *accel = (EV_DefaultExpandAccel *)user_data;
  EV_ExpandRangeInfo result = {0};
  U64 needed_row_count = dim_1u64(idx_range);
  
  ////////////////////////////
  //- rjf: fill with members
  //
  if(accel->members.count != 0)
  {
    E_MemberArray *members = &accel->members;
    result.row_exprs_count = Min(needed_row_count, members->count);
    result.row_exprs = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      E_Member *member = &members->v[idx_range.min + row_expr_idx];
      result.row_exprs[row_expr_idx] = e_expr_ref_member_access(arena, expr, member->name);
      result.row_members[row_expr_idx] = member;
    }
  }
  
  ////////////////////////////
  //- rjf: fill with enum vals
  //
  else if(accel->enum_vals.count != 0)
  {
    E_EnumValArray *enumvals = &accel->enum_vals;
    result.row_exprs_count = Min(needed_row_count, enumvals->count);
    result.row_exprs = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      E_EnumVal *enumval = &enumvals->v[idx_range.min + row_expr_idx];
      result.row_exprs[row_expr_idx] = e_expr_ref_member_access(arena, expr, enumval->name);
      result.row_members[row_expr_idx] = &e_member_nil;
    }
  }
  
  ////////////////////////////
  //- rjf: fill with array indices
  //
  else if(accel->array_count != 0)
  {
    E_Expr *array_expr = accel->array_need_extra_deref ? e_expr_ref_deref(arena, expr) : expr;
    result.row_exprs_count = Min(needed_row_count, accel->array_count);
    result.row_exprs = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      result.row_exprs[row_expr_idx] = e_expr_ref_array_index(arena, array_expr, idx_range.min + row_expr_idx);
      result.row_members[row_expr_idx] = &e_member_nil;
    }
  }
  
  ////////////////////////////
  //- rjf: fill with ptr-to-ptr deref
  //
  else if(accel->is_ptr2ptr)
  {
    result.row_exprs_count = 1;
    result.row_exprs = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules = push_array(arena, String8, result.row_exprs_count);
    result.row_members = push_array(arena, E_Member *, result.row_exprs_count);
    result.row_exprs[0] = e_expr_ref_deref(arena, expr);
    result.row_members[0] = &e_member_nil;
  }
  
  return result;
}

////////////////////////////////
//~ rjf: "array"

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(array)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKey type_key = irtree.type_key;
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  if(e_type_kind_is_pointer_or_ref(type_kind))
  {
    E_Value count = ev_value_from_params(params);
    E_TypeKey element_type_key = e_type_ptee_from_key(type_key);
    E_TypeKey array_type_key = e_type_key_cons_array(element_type_key, count.u64);
    E_TypeKey ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, array_type_key, 0);
    expr = e_expr_ref_cast(arena, ptr_type_key, expr);
  }
  scratch_end(scratch);
  return expr;
}

////////////////////////////////
//~ rjf: "list"

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(list)
{
  EV_ExpandInfo info = {0};
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(list)
{
  EV_ExpandRangeInfo info = {0};
  return info;
}

////////////////////////////////
//~ rjf: "slice"

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(slice)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKind type_kind = e_type_kind_from_key(irtree.type_key);
  if(type_kind == E_TypeKind_Struct || type_kind == E_TypeKind_Class)
  {
    // rjf: unpack members
    E_MemberArray members = e_type_data_members_from_key__cached(irtree.type_key);
    
    // rjf: choose base pointer & count members
    E_Member *base_ptr_member = 0;
    E_Member *count_member = 0;
    for(U64 idx = 0; idx < members.count; idx += 1)
    {
      E_Member *member = &members.v[idx];
      E_TypeKey member_type = e_type_unwrap(member->type_key);
      E_TypeKind member_type_kind = e_type_kind_from_key(member_type);
      if(count_member == 0 && e_type_kind_is_integer(member_type_kind))
      {
        count_member = member;
      }
      if(base_ptr_member == 0 && e_type_kind_is_pointer_or_ref(member_type_kind))
      {
        base_ptr_member = &members.v[idx];
      }
      if(count_member != 0 && base_ptr_member != 0)
      {
        break;
      }
    }
    
    // rjf: evaluate count member, determine count
    U64 count = 0;
    if(count_member != 0)
    {
      E_Expr *count_member_expr = e_expr_ref_member_access(scratch.arena, expr, count_member->name);
      E_Eval count_member_eval = e_eval_from_expr(scratch.arena, count_member_expr);
      E_Eval count_member_value_eval = e_value_eval_from_eval(count_member_eval);
      count = count_member_value_eval.value.u64;
    }
    
    // rjf: generate new struct slice type
    E_TypeKey slice_type_key = zero_struct;
    if(base_ptr_member != 0 && count_member != 0)
    {
      String8 struct_name = e_type_string_from_key(scratch.arena, irtree.type_key);
      E_TypeKey element_type_key = e_type_ptee_from_key(base_ptr_member->type_key);
      E_TypeKey array_type_key = e_type_key_cons_array(element_type_key, count);
      E_TypeKey sized_base_ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, array_type_key, 0);
      E_MemberList slice_type_members = {0};
      e_member_list_push(scratch.arena, &slice_type_members, count_member);
      e_member_list_push(scratch.arena, &slice_type_members, &(E_Member){.kind = E_MemberKind_DataField, .type_key = sized_base_ptr_type_key, .name = base_ptr_member->name, .pretty_name = base_ptr_member->pretty_name, .off = base_ptr_member->off});
      E_MemberArray slice_type_members_array = e_member_array_from_list(scratch.arena, &slice_type_members);
      slice_type_key = e_type_key_cons(.arch = e_type_state->ctx->primary_module->arch,
                                       .kind = E_TypeKind_Struct,
                                       .name = struct_name,
                                       .members = slice_type_members_array.v,
                                       .count = slice_type_members_array.count);
    }
    
    // rjf: generate new expression tree - addr of struct, cast-to-ptr, deref
    if(base_ptr_member != 0 && count_member != 0)
    {
      expr = e_expr_ref_addr(arena, expr);
      expr = e_expr_ref_cast(arena, e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, slice_type_key, 0), expr);
      expr = e_expr_ref_deref(arena, expr);
    }
  }
  scratch_end(scratch);
  return expr;
}

////////////////////////////////
//~ rjf: "bswap"

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(bswap)
{
  expr = e_expr_ref_bswap(arena, expr);
  return expr;
}

////////////////////////////////
//~ rjf: "cast"

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(cast)
{
  E_TypeKey type_key = ev_type_key_from_params(params);
  expr = e_expr_ref_cast(arena, type_key, expr);
  return expr;
}

////////////////////////////////
//~ rjf: "only"

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(only)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKey type_key = irtree.type_key;
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_direct_from_key(type_key);
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  B32 is_ptr = e_type_kind_is_pointer_or_ref(type_kind);
  E_TypeKey struct_type_key = is_ptr ? direct_type_key : type_key;
  E_TypeKind struct_type_kind = is_ptr ? direct_type_kind : type_kind;
  if(struct_type_kind == E_TypeKind_Struct ||
     struct_type_kind == E_TypeKind_Union ||
     struct_type_kind == E_TypeKind_Class)
  {
    E_MemberArray current_members = e_type_data_members_from_key__cached(struct_type_key);
    E_MemberList new_members = {0};
    for MD_EachNode(node, params->first)
    {
      for EachIndex(idx, current_members.count)
      {
        if(str8_match(node->string, current_members.v[idx].name, 0))
        {
          e_member_list_push(scratch.arena, &new_members, &current_members.v[idx]);
          break;
        }
      }
    }
    E_MemberArray new_members_array = e_member_array_from_list(scratch.arena, &new_members);
    E_TypeKey new_type = {0};
    if(new_members_array.count == 1 && new_members_array.v[0].off == 0)
    {
      new_type = new_members_array.v[0].type_key;
    }
    else
    {
      String8 struct_name = e_type_string_from_key(scratch.arena, struct_type_key);
      new_type = e_type_key_cons(.kind = E_TypeKind_Struct, .name = struct_name, .members = new_members_array.v, .count = new_members_array.count);
    }
    if(!is_ptr)
    {
      expr = e_expr_ref_addr(arena, expr);
    }
    expr = e_expr_ref_cast(arena, e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, new_type, 0), expr);
    if(!is_ptr)
    {
      expr = e_expr_ref_deref(arena, expr);
    }
  }
  scratch_end(scratch);
  return expr;
}

////////////////////////////////
//~ rjf: "omit"

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(omit)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKey type_key = irtree.type_key;
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_direct_from_key(type_key);
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  B32 is_ptr = e_type_kind_is_pointer_or_ref(type_kind);
  E_TypeKey struct_type_key = is_ptr ? direct_type_key : type_key;
  E_TypeKind struct_type_kind = is_ptr ? direct_type_kind : type_kind;
  if(struct_type_kind == E_TypeKind_Struct ||
     struct_type_kind == E_TypeKind_Union ||
     struct_type_kind == E_TypeKind_Class)
  {
    E_MemberArray current_members = e_type_data_members_from_key__cached(struct_type_key);
    E_MemberList new_members = {0};
    for EachIndex(idx, current_members.count)
    {
      B32 include = 1;
      for MD_EachNode(node, params->first)
      {
        if(str8_match(node->string, current_members.v[idx].name, 0))
        {
          include = 0;
          break;
        }
      }
      if(include)
      {
        e_member_list_push(scratch.arena, &new_members, &current_members.v[idx]);
      }
    }
    E_MemberArray new_members_array = e_member_array_from_list(scratch.arena, &new_members);
    E_TypeKey new_type = {0};
    String8 struct_name = e_type_string_from_key(scratch.arena, struct_type_key);
    new_type = e_type_key_cons(.kind = E_TypeKind_Struct, .name = struct_name, .members = new_members_array.v, .count = new_members_array.count);
    if(!is_ptr)
    {
      expr = e_expr_ref_addr(arena, expr);
    }
    expr = e_expr_ref_cast(arena, e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, new_type, 0), expr);
    if(!is_ptr)
    {
      expr = e_expr_ref_deref(arena, expr);
    }
  }
  scratch_end(scratch);
  return expr;
}
