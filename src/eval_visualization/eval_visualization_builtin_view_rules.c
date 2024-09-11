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

EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(default)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  ////////////////////////////
  //- rjf: unpack expression type info
  //
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
  E_TypeKey type_key = e_type_unwrap(irtree.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(type_key));
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  
  ////////////////////////////
  //- rjf: do struct/union/class member block generation
  //
  if((type_kind == E_TypeKind_Struct ||
      type_kind == E_TypeKind_Union ||
      type_kind == E_TypeKind_Class) ||
     (e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                   direct_type_kind == E_TypeKind_Union ||
                                                   direct_type_kind == E_TypeKind_Class)))
  {
    // rjf: type -> filtered data members
    E_MemberArray data_members = e_type_data_members_from_key(arena, e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key);
    
    // rjf: build blocks for all members, split by sub-expansions
    EV_Block *last_vb = ev_block_begin(arena, EV_BlockKind_Members, key, ev_key_make(ev_hash_from_key(key), 0), depth);
    {
      last_vb->expr             = expr;
      last_vb->view_rules       = view_rules;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, data_members.count);
      last_vb->members          = data_members;
    }
    for(EV_ExpandNode *child = expand_node->first; child != 0; child = child->next)
    {
      // rjf: unpack expansion info; skip out-of-bounds splits
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      E_Expr *child_expr = ev_expr_from_block_index(arena, last_vb, child_idx);
      if(child_idx >= last_vb->semantic_idx_range.max)
      {
        continue;
      }
      
      // rjf: form split: truncate & complete last block; begin next block
      last_vb = ev_block_split_and_continue(arena, out, last_vb, child_idx);
      
      // rjf: build child view rules
      EV_ViewRuleList *child_view_rules = ev_view_rule_list_from_inheritance(arena, view_rules);
      {
        String8 view_rule_string = ev_view_rule_from_key(view, child->key);
        if(view_rule_string.size != 0)
        {
          ev_view_rule_list_push_string(arena, child_view_rules, view_rule_string);
        }
      }
      
      // rjf: recurse for child
      ev_append_expr_blocks__rec(arena, view, key, child->key, str8_zero(), child_expr, child_view_rules, depth, out);
    }
    ev_block_end(out, last_vb);
  }
  
  ////////////////////////////
  //- rjf: do enum member block generation
  //
  // (just a single block for all enum members; enum members can never be expanded)
  //
  else if(type_kind == E_TypeKind_Enum ||
          (e_type_kind_is_pointer_or_ref(type_kind) && direct_type_kind == E_TypeKind_Enum))
  {
    E_Type *type = e_type_from_key(arena, e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key);
    EV_Block *last_vb = ev_block_begin(arena, EV_BlockKind_EnumMembers, key, ev_key_make(ev_hash_from_key(key), 0), depth);
    {
      last_vb->expr             = expr;
      last_vb->view_rules       = view_rules;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, type->count);
      last_vb->enum_vals.v      = type->enum_vals;
      last_vb->enum_vals.count  = type->count;
    }
    ev_block_end(out, last_vb);
  }
  
  ////////////////////////////
  //- rjf: do array element block generation
  //
  else if(type_kind == E_TypeKind_Array ||
          (e_type_kind_is_pointer_or_ref(type_kind) && direct_type_kind == E_TypeKind_Array))
  {
    // rjf: unpack array type info
    E_Type *array_type = e_type_from_key(scratch.arena, e_type_kind_is_pointer_or_ref(type_kind) ? direct_type_key : type_key);
    U64 array_count = array_type->count;
    B32 need_extra_deref = e_type_kind_is_pointer_or_ref(type_kind);
    
    // rjf: build blocks for all elements, split by sub-expansions
    EV_Block *last_vb = ev_block_begin(arena, EV_BlockKind_Elements, key, ev_key_make(ev_hash_from_key(key), 0), depth);
    {
      last_vb->expr             = need_extra_deref ? e_expr_ref_deref(arena, expr) : expr;
      last_vb->view_rules       = view_rules;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, array_count);
    }
    for(EV_ExpandNode *child = expand_node->first; child != 0; child = child->next)
    {
      // rjf: unpack expansion info; skip out-of-bounds splits
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      E_Expr *child_expr = ev_expr_from_block_index(arena, last_vb, child_idx);
      if(child_idx >= last_vb->semantic_idx_range.max)
      {
        continue;
      }
      
      // rjf: form split: truncate & complete last block; begin next block
      last_vb = ev_block_split_and_continue(arena, out, last_vb, child_idx);
      
      // rjf: build child view rules
      EV_ViewRuleList *child_view_rules = ev_view_rule_list_from_inheritance(arena, view_rules);
      {
        String8 view_rule_string = ev_view_rule_from_key(view, child->key);
        if(view_rule_string.size != 0)
        {
          ev_view_rule_list_push_string(arena, child_view_rules, view_rule_string);
        }
      }
      
      // rjf: recurse for child
      ev_append_expr_blocks__rec(arena, view, key, child->key, str8_zero(), child_expr, child_view_rules, depth, out);
    }
    ev_block_end(out, last_vb);
  }
  
  ////////////////////////////
  //- rjf: do pointer-to-pointer block generation
  //
  else if(e_type_kind_is_pointer_or_ref(type_kind) && e_type_kind_is_pointer_or_ref(direct_type_kind))
  {
    // rjf: compute key
    EV_Key child_key = ev_key_make(ev_hash_from_key(key), 1);
    
    // rjf: build child view rules
    EV_ViewRuleList *child_view_rules = ev_view_rule_list_from_inheritance(arena, view_rules);
    {
      String8 view_rule_string = ev_view_rule_from_key(view, child_key);
      if(view_rule_string.size != 0)
      {
        ev_view_rule_list_push_string(arena, child_view_rules, view_rule_string);
      }
    }
    
    // rjf: recurse for child
    E_Expr *child_expr = e_expr_ref_deref(arena, expr);
    ev_append_expr_blocks__rec(arena, view, key, child_key, str8_zero(), child_expr, child_view_rules, depth, out);
  }
  
  scratch_end(scratch);
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
    E_TypeKey ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, array_type_key);
    expr = e_expr_ref_cast(arena, ptr_type_key, expr);
  }
  scratch_end(scratch);
  return expr;
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
    E_MemberArray members = e_type_data_members_from_key(scratch.arena, irtree.type_key);
    
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
      E_TypeKey sized_base_ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, array_type_key);
      E_MemberList slice_type_members = {0};
      e_member_list_push(scratch.arena, &slice_type_members, count_member);
      e_member_list_push(scratch.arena, &slice_type_members, &(E_Member){.kind = E_MemberKind_DataField, .type_key = sized_base_ptr_type_key, .name = base_ptr_member->name, .off = base_ptr_member->off});
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
      expr = e_expr_ref_cast(arena, e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, slice_type_key), expr);
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