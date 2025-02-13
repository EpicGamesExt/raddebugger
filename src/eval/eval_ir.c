// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Expr Kind Enum Functions

internal RDI_EvalOp
e_opcode_from_expr_kind(E_ExprKind kind)
{
  RDI_EvalOp result = RDI_EvalOp_Stop;
  switch(kind)
  {
    case E_ExprKind_Neg:    result = RDI_EvalOp_Neg;    break;
    case E_ExprKind_LogNot: result = RDI_EvalOp_LogNot; break;
    case E_ExprKind_BitNot: result = RDI_EvalOp_BitNot; break;
    case E_ExprKind_Mul:    result = RDI_EvalOp_Mul;    break;
    case E_ExprKind_Div:    result = RDI_EvalOp_Div;    break;
    case E_ExprKind_Mod:    result = RDI_EvalOp_Mod;    break;
    case E_ExprKind_Add:    result = RDI_EvalOp_Add;    break;
    case E_ExprKind_Sub:    result = RDI_EvalOp_Sub;    break;
    case E_ExprKind_LShift: result = RDI_EvalOp_LShift; break;
    case E_ExprKind_RShift: result = RDI_EvalOp_RShift; break;
    case E_ExprKind_Less:   result = RDI_EvalOp_Less;   break;
    case E_ExprKind_LsEq:   result = RDI_EvalOp_LsEq;   break;
    case E_ExprKind_Grtr:   result = RDI_EvalOp_Grtr;   break;
    case E_ExprKind_GrEq:   result = RDI_EvalOp_GrEq;   break;
    case E_ExprKind_EqEq:   result = RDI_EvalOp_EqEq;   break;
    case E_ExprKind_NtEq:   result = RDI_EvalOp_NtEq;   break;
    case E_ExprKind_BitAnd: result = RDI_EvalOp_BitAnd; break;
    case E_ExprKind_BitXor: result = RDI_EvalOp_BitXor; break;
    case E_ExprKind_BitOr:  result = RDI_EvalOp_BitOr;  break;
    case E_ExprKind_LogAnd: result = RDI_EvalOp_LogAnd; break;
    case E_ExprKind_LogOr:  result = RDI_EvalOp_LogOr;  break;
  }
  return result;
}

internal B32
e_expr_kind_is_comparison(E_ExprKind kind)
{
  B32 result = 0;
  switch(kind)
  {
    default:{}break;
    case E_ExprKind_EqEq:
    case E_ExprKind_NtEq:
    case E_ExprKind_Less:
    case E_ExprKind_Grtr:
    case E_ExprKind_LsEq:
    case E_ExprKind_GrEq:
    {
      result = 1;
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Context Selection Functions (Selection Required For All Subsequent APIs)

internal E_IRCtx *
e_selected_ir_ctx(void)
{
  return e_ir_state->ctx;
}

internal void
e_select_ir_ctx(E_IRCtx *ctx)
{
  if(e_ir_state == 0)
  {
    Arena *arena = arena_alloc();
    e_ir_state = push_array(arena, E_IRState, 1);
    e_ir_state->arena = arena;
    e_ir_state->arena_eval_start_pos = arena_pos(arena);
  }
  arena_pop_to(e_ir_state->arena, e_ir_state->arena_eval_start_pos);
  e_ir_state->ctx = ctx;
  e_ir_state->used_tag_map       = push_array(e_ir_state->arena, E_UsedTagMap, 1);
  e_ir_state->used_tag_map->slots_count = 64;
  e_ir_state->used_tag_map->slots = push_array(e_ir_state->arena, E_UsedTagSlot, e_ir_state->used_tag_map->slots_count);
  e_ir_state->type_auto_hook_cache_map = push_array(e_ir_state->arena, E_TypeAutoHookCacheMap, 1);
  e_ir_state->type_auto_hook_cache_map->slots_count = 256;
  e_ir_state->type_auto_hook_cache_map->slots = push_array(e_ir_state->arena, E_TypeAutoHookCacheSlot, e_ir_state->type_auto_hook_cache_map->slots_count);
  e_ir_state->irtree_and_type_cache_slots_count = 1024;
  e_ir_state->irtree_and_type_cache_slots = push_array(e_ir_state->arena, E_IRTreeAndTypeCacheSlot, e_ir_state->irtree_and_type_cache_slots_count);
}

////////////////////////////////
//~ rjf: Lookups

internal E_LookupRuleMap
e_lookup_rule_map_make(Arena *arena, U64 slots_count)
{
  E_LookupRuleMap map = {0};
  map.slots_count = slots_count;
  map.slots = push_array(arena, E_LookupRuleSlot, map.slots_count);
  return map;
}

internal void
e_lookup_rule_map_insert(Arena *arena, E_LookupRuleMap *map, E_LookupRule *rule)
{
  U64 hash = e_hash_from_string(5381, rule->name);
  U64 slot_idx = hash%map->slots_count;
  E_LookupRuleNode *n = push_array(arena, E_LookupRuleNode, 1);
  SLLQueuePush(map->slots[slot_idx].first, map->slots[slot_idx].last, n);
  MemoryCopyStruct(&n->v, rule);
  if(n->v.info == 0)       { n->v.info = E_LOOKUP_INFO_FUNCTION_NAME(default); }
  if(n->v.access == 0)     { n->v.access = E_LOOKUP_ACCESS_FUNCTION_NAME(default); }
  if(n->v.range == 0)      { n->v.range = E_LOOKUP_RANGE_FUNCTION_NAME(default); }
  if(n->v.id_from_num == 0){ n->v.id_from_num = E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(default); }
  if(n->v.num_from_id == 0){ n->v.num_from_id = E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(default); }
  n->v.name = push_str8_copy(arena, n->v.name);
}

internal E_LookupRule *
e_lookup_rule_from_string(String8 string)
{
  E_LookupRule *result = &e_lookup_rule__nil;
  if(e_ir_state->ctx->lookup_rule_map != 0 && e_ir_state->ctx->lookup_rule_map->slots_count != 0)
  {
    U64 hash = e_hash_from_string(5381, string);
    U64 slot_idx = hash%e_ir_state->ctx->lookup_rule_map->slots_count;
    for(E_LookupRuleNode *n = e_ir_state->ctx->lookup_rule_map->slots[slot_idx].first;
        n != 0;
        n = n->next)
    {
      if(str8_match(n->v.name, string, 0))
      {
        result = &n->v;
        break;
      }
    }
  }
  return result;
}

E_LOOKUP_INFO_FUNCTION_DEF(default)
{
  E_LookupInfo lookup_info = {0};
  {
    E_TypeKey lhs_type_key = e_type_unwrap(lhs->type_key);
    E_TypeKind lhs_type_kind = e_type_kind_from_key(lhs_type_key);
    if(e_type_kind_is_pointer_or_ref(lhs_type_kind))
    {
      E_Type *type = e_type_from_key__cached(lhs_type_key);
      lookup_info.idxed_expr_count = type->count;
      if(type->count == 1)
      {
        E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(lhs->type_key));
        E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
        if(direct_type_kind == E_TypeKind_Struct ||
           direct_type_kind == E_TypeKind_Class ||
           direct_type_kind == E_TypeKind_Union ||
           direct_type_kind == E_TypeKind_Enum)
        {
          E_Type *direct_type = e_type_from_key__cached(direct_type_key);
          lookup_info.named_expr_count = direct_type->count;
        }
      }
    }
    else if(lhs_type_kind == E_TypeKind_Struct ||
            lhs_type_kind == E_TypeKind_Class ||
            lhs_type_kind == E_TypeKind_Union ||
            lhs_type_kind == E_TypeKind_Enum)
    {
      E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
      lookup_info.named_expr_count = lhs_type->count;
    }
    else if(lhs_type_kind == E_TypeKind_Array)
    {
      E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
      lookup_info.idxed_expr_count = lhs_type->count;
    }
  }
  return lookup_info;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(default)
{
  //
  // TODO(rjf): need to define what it means to access a set expression
  // whose type *does not* define its IR generation rules, BUT it does
  // define specific child expressions. so e.g. `watches`, does not
  // define `watches[0]`, because it has defined that `watches[0]`
  // maps to another expression, which is whatever the first watch
  // expression is (e.g. `basics`). so, in that case, we can just use
  // the lookup-range rule, grab the Nth expression, and IR-ify *that*.
  //
  E_LookupAccess result = {{&e_irnode_nil}};
  switch(kind)
  {
    default:{}break;
    
    //- rjf: member accessing
    case E_ExprKind_MemberAccess:
    {
      // rjf: unpack left/right expressions
      E_Expr *exprl = lhs;
      E_Expr *exprr = rhs;
      E_IRTreeAndType l = e_irtree_and_type_from_expr(arena, exprl);
      E_TypeKey l_restype = e_type_unwrap(l.type_key);
      E_TypeKind l_restype_kind = e_type_kind_from_key(l_restype);
      E_TypeKey check_type_key = l_restype;
      E_TypeKind check_type_kind = l_restype_kind;
      if(l_restype_kind == E_TypeKind_Ptr ||
         l_restype_kind == E_TypeKind_LRef ||
         l_restype_kind == E_TypeKind_RRef)
      {
        check_type_key = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(l_restype)));
        check_type_kind = e_type_kind_from_key(check_type_key);
      }
      e_msg_list_concat_in_place(&result.irtree_and_type.msgs, &l.msgs);
      
      // rjf: look up member
      B32 r_found = 0;
      E_TypeKey r_type = zero_struct;
      U64 r_value = 0;
      B32 r_is_constant_value = 0;
      {
        Temp scratch = scratch_begin(&arena, 1);
        E_Member match = e_type_member_from_key_name__cached(check_type_key, exprr->string);
        if(match.kind != E_MemberKind_Null)
        {
          r_found = 1;
          r_type = match.type_key;
          r_value = match.off;
        }
        if(match.kind == E_MemberKind_Null)
        {
          E_Type *type = e_type_from_key__cached(check_type_key);
          if(type->enum_vals != 0)
          {
            String8 lookup_string = exprr->string;
            String8 lookup_string_append_1 = push_str8f(scratch.arena, "%S_%S", type->name, lookup_string);
            String8 lookup_string_append_2 = push_str8f(scratch.arena, "%S%S", type->name, lookup_string);
            E_EnumVal *enum_val_match = 0;
            for EachIndex(idx, type->count)
            {
              if(str8_match(type->enum_vals[idx].name, lookup_string, 0) ||
                 str8_match(type->enum_vals[idx].name, lookup_string_append_1, 0) ||
                 str8_match(type->enum_vals[idx].name, lookup_string_append_2, 0))
              {
                enum_val_match = &type->enum_vals[idx];
                break;
              }
            }
            if(enum_val_match != 0)
            {
              r_found = 1;
              r_type = check_type_key;
              r_value = enum_val_match->val;
              r_is_constant_value = 1;
            }
          }
        }
        scratch_end(scratch);
      }
      
      // rjf: bad conditions? -> error if applicable, exit
      if(e_type_key_match(e_type_key_zero(), check_type_key))
      {
        break;
      }
      else if(exprr->kind != E_ExprKind_LeafMember)
      {
        e_msgf(arena, &result.irtree_and_type.msgs, E_MsgKind_MalformedInput, exprl->location, "Expected member name.");
        break;
      }
      else if(!r_found)
      {
        e_msgf(arena, &result.irtree_and_type.msgs, E_MsgKind_MalformedInput, exprr->location, "Could not find a member named `%S`.", exprr->string);
        break;
      }
      else if(check_type_kind != E_TypeKind_Struct &&
              check_type_kind != E_TypeKind_Class &&
              check_type_kind != E_TypeKind_Union &&
              check_type_kind != E_TypeKind_Enum)
      {
        e_msgf(arena, &result.irtree_and_type.msgs, E_MsgKind_MalformedInput, exprl->location, "Cannot perform member access on this type.");
        break;
      }
      
      // rjf: generate
      {
        // rjf: build tree
        E_IRNode *new_tree = l.root;
        E_Mode mode = l.mode;
        if(l_restype_kind == E_TypeKind_Ptr ||
           l_restype_kind == E_TypeKind_LRef ||
           l_restype_kind == E_TypeKind_RRef)
        {
          new_tree = e_irtree_resolve_to_value(arena, l.mode, new_tree, l_restype);
          mode = E_Mode_Offset;
        }
        if(r_value != 0 && !r_is_constant_value)
        {
          E_IRNode *const_tree = e_irtree_const_u(arena, r_value);
          new_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Add, new_tree, const_tree);
        }
        else if(r_is_constant_value)
        {
          new_tree = e_irtree_const_u(arena, r_value);
          mode = E_Mode_Value;
        }
        
        // rjf: fill
        result.irtree_and_type.root     = new_tree;
        result.irtree_and_type.type_key = r_type;
        result.irtree_and_type.mode     = mode;
      }
    }break;
    
    //- rjf: array indexing
    case E_ExprKind_ArrayIndex:
    {
      // rjf: unpack left/right expressions
      E_Expr *exprl = lhs;
      E_Expr *exprr = rhs;
      E_IRTreeAndType l = e_irtree_and_type_from_expr(arena, exprl);
      E_IRTreeAndType r = e_irtree_and_type_from_expr(arena, exprr);
      E_TypeKey l_restype = e_type_unwrap(l.type_key);
      E_TypeKey r_restype = e_type_unwrap(r.type_key);
      E_TypeKind l_restype_kind = e_type_kind_from_key(l_restype);
      E_TypeKind r_restype_kind = e_type_kind_from_key(r_restype);
      if(e_type_kind_is_basic_or_enum(r_restype_kind))
      {
        r_restype = e_type_unwrap_enum(r_restype);
        r_restype_kind = e_type_kind_from_key(r_restype);
      }
      E_TypeKey direct_type = e_type_unwrap(l_restype);
      direct_type = e_type_direct_from_key(direct_type);
      direct_type = e_type_unwrap(direct_type);
      U64 direct_type_size = e_type_byte_size_from_key(direct_type);
      e_msg_list_concat_in_place(&result.irtree_and_type.msgs, &l.msgs);
      e_msg_list_concat_in_place(&result.irtree_and_type.msgs, &r.msgs);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(r.root->op == 0)
      {
        break;
      }
      else if(l_restype_kind != E_TypeKind_Ptr && l_restype_kind != E_TypeKind_Array)
      {
        e_msgf(arena, &result.irtree_and_type.msgs, E_MsgKind_MalformedInput, exprl->location, "Cannot index into this type.");
        break;
      }
      else if(!e_type_kind_is_integer(r_restype_kind))
      {
        e_msgf(arena, &result.irtree_and_type.msgs, E_MsgKind_MalformedInput, exprr->location, "Cannot index with this type.");
        break;
      }
      else if(l_restype_kind == E_TypeKind_Ptr && direct_type_size == 0)
      {
        e_msgf(arena, &result.irtree_and_type.msgs, E_MsgKind_MalformedInput, exprr->location, "Cannot index into pointers of zero-sized types.");
        break;
      }
      else if(l_restype_kind == E_TypeKind_Array && direct_type_size == 0)
      {
        e_msgf(arena, &result.irtree_and_type.msgs, E_MsgKind_MalformedInput, exprr->location, "Cannot index into arrays of zero-sized types.");
        break;
      }
      
      // rjf: generate
      E_IRNode *new_tree = &e_irnode_nil;
      E_Mode mode = l.mode;
      {
        // rjf: reading from an array value -> read from stack value
        if(l.mode == E_Mode_Value && l_restype_kind == E_TypeKind_Array)
        {
          // rjf: ops to compute the offset
          E_IRNode *offset_tree = e_irtree_resolve_to_value(arena, r.mode, r.root, r_restype);
          if(direct_type_size > 1)
          {
            E_IRNode *const_tree = e_irtree_const_u(arena, direct_type_size);
            offset_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Mul, offset_tree, const_tree);
          }
          
          // rjf: ops to push stack value, push offset, + read from stack value
          new_tree = e_push_irnode(arena, RDI_EvalOp_ValueRead);
          new_tree->value.u64 = direct_type_size;
          e_irnode_push_child(new_tree, offset_tree);
          e_irnode_push_child(new_tree, l.root);
        }
        
        // rjf: all other cases -> read from base offset
        else
        {
          // rjf: ops to compute the offset
          E_IRNode *offset_tree = e_irtree_resolve_to_value(arena, r.mode, r.root, r_restype);
          if(direct_type_size > 1)
          {
            E_IRNode *const_tree = e_irtree_const_u(arena, direct_type_size);
            offset_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Mul, offset_tree, const_tree);
          }
          
          // rjf: ops to compute the base offset (resolve to value if addr-of-pointer)
          E_IRNode *base_tree = l.root;
          if(l_restype_kind == E_TypeKind_Ptr && l.mode != E_Mode_Value)
          {
            base_tree = e_irtree_resolve_to_value(arena, l.mode, base_tree, l_restype);
          }
          
          // rjf: ops to compute the final address
          new_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Add, offset_tree, base_tree);
          mode = E_Mode_Offset;
        }
      }
      
      // rjf: fill
      result.irtree_and_type.root     = new_tree;
      result.irtree_and_type.type_key = direct_type;
      result.irtree_and_type.mode     = mode;
    }break;
  }
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(default)
{
  Temp scratch = scratch_begin(&arena, 1);
  {
    //- rjf: unpack type of expression
    E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(scratch.arena, lhs);
    E_TypeKey lhs_type_key = lhs_irtree.type_key;
    E_TypeKind lhs_type_kind = e_type_kind_from_key(lhs_type_key);
    E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(lhs_type_key));
    E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
    
    //- rjf: pull out specific kinds of types
    B32 do_struct_range = 0;
    B32 do_index_range = 0;
    E_TypeKey struct_type_key = zero_struct;
    E_TypeKind struct_type_kind = E_TypeKind_Null;
    if(e_type_kind_is_pointer_or_ref(lhs_type_kind))
    {
      E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
      if(lhs_type->count == 1 &&
         (direct_type_kind == E_TypeKind_Struct ||
          direct_type_kind == E_TypeKind_Union ||
          direct_type_kind == E_TypeKind_Class ||
          direct_type_kind == E_TypeKind_Enum))
      {
        struct_type_key = direct_type_key;
        struct_type_kind = direct_type_kind;
        do_struct_range = 1;
      }
      else
      {
        do_index_range = 1;
      }
    }
    else if(lhs_type_kind == E_TypeKind_Struct ||
            lhs_type_kind == E_TypeKind_Union ||
            lhs_type_kind == E_TypeKind_Class ||
            lhs_type_kind == E_TypeKind_Enum)
    {
      struct_type_key = lhs_type_key;
      struct_type_kind = lhs_type_kind;
      do_struct_range = 1;
    }
    else if(lhs_type_kind == E_TypeKind_Set)
    {
      do_index_range = 1;
    }
    else if(lhs_type_kind == E_TypeKind_Array)
    {
      do_index_range = 1;
    }
    
    //- rjf: struct case -> the lookup-range will return a range of members
    if(do_struct_range)
    {
      E_Type *struct_type = e_type_from_key__cached(struct_type_key);
      Rng1U64 legal_idx_range = r1u64(0, struct_type->count);
      Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
      U64 read_range_count = dim_1u64(read_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        U64 member_idx = idx + read_range.min;
        String8 member_name = (struct_type->members   ? struct_type->members[member_idx].name :
                               struct_type->enum_vals ? struct_type->enum_vals[member_idx].name : str8_lit(""));
        exprs[idx] = e_expr_irext_member_access(arena, lhs, &lhs_irtree, member_name);
      }
    }
    
    //- rjf: ptr case -> the lookup-range will return a range of dereferences
    else if(do_index_range)
    {
      U64 read_range_count = dim_1u64(idx_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        exprs[idx] = e_expr_ref_array_index(arena, lhs, idx_range.min + idx);
      }
    }
  }
  scratch_end(scratch);
}

E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(default)
{
  return num;
}

E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(default)
{
  return id;
}

////////////////////////////////
//~ rjf: IR Gen Rules

E_IRGEN_FUNCTION_DEF(cast)
{
  E_Expr *type_expr = tag->first->next;
  E_TypeKey type_key = e_type_from_expr(type_expr);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(arena, expr);
  E_IRTreeAndType result = {irtree.root, type_key, irtree.mode, irtree.msgs};
  return result;
}

E_IRGEN_FUNCTION_DEF(bswap)
{
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(arena, expr);
  E_IRNode *root = e_push_irnode(arena, RDI_EvalOp_ByteSwap);
  e_irnode_push_child(root, irtree.root);
  E_IRTreeAndType result = {root, irtree.type_key, irtree.mode, irtree.msgs};
  return result;
}

E_IRGEN_FUNCTION_DEF(array)
{
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(arena, expr);
  E_TypeKey type_key = irtree.type_key;
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  if(e_type_kind_is_pointer_or_ref(type_kind))
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_Eval count_eval = e_eval_from_expr(scratch.arena, tag->first->next);
    E_TypeKey element_type_key = e_type_ptee_from_key(type_key);
    E_TypeKey ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, element_type_key, count_eval.value.u64, 0);
    irtree.type_key = ptr_type_key;
    scratch_end(scratch);
  }
  return irtree;
}

E_IRGEN_FUNCTION_DEF(slice)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(arena, expr);
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
      E_Expr *count_member_expr = e_expr_irext_member_access(arena, expr, &irtree, count_member->name);
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
      E_TypeKey sized_base_ptr_type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, element_type_key, count, 0);
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
    
    // rjf: overwrite type
    if(!e_type_key_match(slice_type_key, e_type_key_zero()))
    {
      irtree.type_key = slice_type_key;
    }
  }
  scratch_end(scratch);
  return irtree;
}

E_IRGEN_FUNCTION_DEF(wrap)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_Expr *expr_to_irify = expr;
  E_Expr *wrap_expr_src = tag->first->next;
  if(wrap_expr_src != &e_expr_nil)
  {
    expr_to_irify = e_expr_copy(scratch.arena, wrap_expr_src);
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E_Expr *parent;
      E_Expr *expr;
    };
    Task start_task = {0, &e_expr_nil, expr_to_irify};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      if(t->expr->kind == E_ExprKind_LeafIdent && str8_match(t->expr->string, str8_lit("$expr"), 0))
      {
        E_Expr *original_expr_ref = e_expr_ref(arena, expr);
        if(t->parent != &e_expr_nil)
        {
          e_expr_insert_child(t->parent, t->expr, original_expr_ref);
          e_expr_remove_child(t->parent, t->expr);
        }
      }
      else for(E_Expr *child = t->expr->first; child != &e_expr_nil; child = child->next)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        SLLQueuePush(first_task, last_task, task);
        task->parent = t->expr;
        task->expr = child;
      }
    }
  }
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(arena, expr_to_irify);
  scratch_end(scratch);
  return irtree;
}

internal E_IRGenRuleMap
e_irgen_rule_map_make(Arena *arena, U64 slots_count)
{
  E_IRGenRuleMap map = {0};
  map.slots_count = slots_count;
  map.slots = push_array(arena, E_IRGenRuleSlot, map.slots_count);
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("cast"),  .irgen = E_IRGEN_FUNCTION_NAME(cast));
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("bswap"), .irgen = E_IRGEN_FUNCTION_NAME(bswap));
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("array"), .irgen = E_IRGEN_FUNCTION_NAME(array));
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("slice"), .irgen = E_IRGEN_FUNCTION_NAME(slice));
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("wrap"),  .irgen = E_IRGEN_FUNCTION_NAME(wrap));
  return map;
}

internal void
e_irgen_rule_map_insert(Arena *arena, E_IRGenRuleMap *map, E_IRGenRule *rule)
{
  U64 hash = e_hash_from_string(5381, rule->name);
  U64 slot_idx = hash%map->slots_count;
  E_IRGenRuleNode *n = push_array(arena, E_IRGenRuleNode, 1);
  MemoryCopyStruct(&n->v, rule);
  n->v.name = push_str8_copy(arena, n->v.name);
  SLLQueuePush(map->slots[slot_idx].first, map->slots[slot_idx].last, n);
}

internal E_IRGenRule *
e_irgen_rule_from_string(String8 string)
{
  E_IRGenRule *rule = &e_irgen_rule__default;
  if(e_ir_state != 0 && e_ir_state->ctx != 0 && e_ir_state->ctx->irgen_rule_map != 0 && e_ir_state->ctx->irgen_rule_map->slots_count != 0)
  {
    E_IRGenRuleMap *map = e_ir_state->ctx->irgen_rule_map;
    U64 hash = e_hash_from_string(5381, string);
    U64 slot_idx = hash%map->slots_count;
    for(E_IRGenRuleNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
    {
      if(str8_match(string, n->v.name, 0))
      {
        rule = &n->v;
        break;
      }
    }
  }
  return rule;
}

////////////////////////////////
//~ rjf: Auto Hooks

internal E_AutoHookMap
e_auto_hook_map_make(Arena *arena, U64 slots_count)
{
  E_AutoHookMap map = {0};
  map.slots_count = slots_count;
  map.slots = push_array(arena, E_AutoHookSlot, map.slots_count);
  return map;
}

internal void
e_auto_hook_map_insert_new_(Arena *arena, E_AutoHookMap *map, E_AutoHookParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_TypeKey type_key = params->type_key;
  if(params->type_pattern.size != 0)
  {
    E_TokenArray tokens = e_token_array_from_text(scratch.arena, params->type_pattern);
    E_Parse parse = e_parse_type_from_text_tokens(scratch.arena, params->type_pattern, &tokens);
    E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, parse.last_expr);
    type_key = irtree.type_key;
  }
  E_AutoHookNode *node = push_array(arena, E_AutoHookNode, 1);
  node->type_key = type_key;
  U8 pattern_split = '?';
  node->type_pattern_parts = str8_split(arena, params->type_pattern, &pattern_split, 1, 0);
  node->tag_expr = e_parse_expr_from_text(arena, push_str8_copy(arena, params->tag_expr_string));
  if(!e_type_key_match(e_type_key_zero(), type_key))
  {
    U64 hash = e_hash_from_string(5381, str8_struct(&type_key));
    U64 slot_idx = hash%map->slots_count;
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
  }
  else
  {
    SLLQueuePush_N(map->first_pattern, map->last_pattern, node, pattern_order_next);
  }
  scratch_end(scratch);
}

internal E_ExprList
e_auto_hook_tag_exprs_from_type_key(Arena *arena, E_TypeKey type_key)
{
  ProfBeginFunction();
  E_ExprList exprs = {0};
  if(e_ir_state != 0 && e_ir_state->ctx != 0)
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_AutoHookMap *map = e_ir_state->ctx->auto_hook_map;
    
    //- rjf: gather exact-type-key-matches from the map
    if(map != 0 && map->slots_count != 0)
    {
      U64 hash = e_hash_from_string(5381, str8_struct(&type_key));
      U64 slot_idx = hash%map->slots_count;
      for(E_AutoHookNode *n = map->slots[slot_idx].first; n != 0; n = n->hash_next)
      {
        if(e_type_key_match(n->type_key, type_key))
        {
          e_expr_list_push(arena, &exprs, n->tag_expr);
        }
      }
    }
    
    //- rjf: gather fuzzy matches from all patterns in the map
    if(map != 0 && map->first_pattern != 0)
    {
      String8 type_string = str8_skip_chop_whitespace(e_type_string_from_key(scratch.arena, type_key));
      for(E_AutoHookNode *auto_hook_node = map->first_pattern; auto_hook_node != 0; auto_hook_node = auto_hook_node->pattern_order_next)
      {
        B32 fits_this_type_string = 1;
        U64 scan_pos = 0;
        for(String8Node *n = auto_hook_node->type_pattern_parts.first; n != 0; n = n->next)
        {
          U64 pattern_part_pos = str8_find_needle(type_string, scan_pos, n->string, 0);
          if(pattern_part_pos >= type_string.size)
          {
            fits_this_type_string = 0;
            break;
          }
          scan_pos = pattern_part_pos + n->string.size;
        }
        if(scan_pos < type_string.size)
        {
          fits_this_type_string = 0;
        }
        if(fits_this_type_string)
        {
          e_expr_list_push(arena, &exprs, auto_hook_node->tag_expr);
        }
      }
    }
    
    scratch_end(scratch);
  }
  ProfEnd();
  return exprs;
}

internal E_ExprList
e_auto_hook_tag_exprs_from_type_key__cached(E_TypeKey type_key)
{
  E_ExprList exprs = {0};
  if(e_ir_state != 0 && e_ir_state->ctx != 0 && e_ir_state->type_auto_hook_cache_map != 0 && e_ir_state->type_auto_hook_cache_map->slots_count != 0)
  {
    U64 hash = e_hash_from_string(5381, str8_struct(&type_key));
    U64 slot_idx = hash%e_ir_state->type_auto_hook_cache_map->slots_count;
    E_TypeAutoHookCacheNode *node = 0;
    for(E_TypeAutoHookCacheNode *n = e_ir_state->type_auto_hook_cache_map->slots[slot_idx].first;
        n != 0;
        n = n->next)
    {
      if(e_type_key_match(n->key, type_key))
      {
        node = n;
      }
    }
    if(node == 0)
    {
      node = push_array(e_ir_state->arena, E_TypeAutoHookCacheNode, 1);
      SLLQueuePush(e_ir_state->type_auto_hook_cache_map->slots[slot_idx].first, e_ir_state->type_auto_hook_cache_map->slots[slot_idx].last, node);
      node->key = type_key;
      node->exprs = e_auto_hook_tag_exprs_from_type_key(e_type_state->arena, type_key);
    }
    exprs = node->exprs;
  }
  return exprs;
}

////////////////////////////////
//~ rjf: IR-ization Functions

//- rjf: op list functions

internal void
e_oplist_push_op(Arena *arena, E_OpList *list, RDI_EvalOp opcode, E_Value value)
{
  U16 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
  U32 p_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
  E_Op *node = push_array_no_zero(arena, E_Op, 1);
  node->opcode = opcode;
  node->value = value;
  SLLQueuePush(list->first, list->last, node);
  list->op_count += 1;
  list->encoded_size += 1 + p_size;
}

internal void
e_oplist_push_uconst(Arena *arena, E_OpList *list, U64 x)
{
  if(0){}
  else if(x <= 0xFF)       { e_oplist_push_op(arena, list, RDI_EvalOp_ConstU8,  e_value_u64(x)); }
  else if(x <= 0xFFFF)     { e_oplist_push_op(arena, list, RDI_EvalOp_ConstU16, e_value_u64(x)); }
  else if(x <= 0xFFFFFFFF) { e_oplist_push_op(arena, list, RDI_EvalOp_ConstU32, e_value_u64(x)); }
  else                     { e_oplist_push_op(arena, list, RDI_EvalOp_ConstU64, e_value_u64(x)); }
}

internal void
e_oplist_push_sconst(Arena *arena, E_OpList *list, S64 x)
{
  if(-0x80 <= x && x <= 0x7F)
  {
    e_oplist_push_op(arena, list, RDI_EvalOp_ConstU8, e_value_u64((U64)x));
    e_oplist_push_op(arena, list, RDI_EvalOp_TruncSigned, e_value_u64(8));
  }
  else if(-0x8000 <= x && x <= 0x7FFF)
  {
    e_oplist_push_op(arena, list, RDI_EvalOp_ConstU16, e_value_u64((U64)x));
    e_oplist_push_op(arena, list, RDI_EvalOp_TruncSigned, e_value_u64(16));
  }
  else if(-0x80000000ll <= x && x <= 0x7FFFFFFFll)
  {
    e_oplist_push_op(arena, list, RDI_EvalOp_ConstU32, e_value_u64((U64)x));
    e_oplist_push_op(arena, list, RDI_EvalOp_TruncSigned, e_value_u64(32));
  }
  else
  {
    e_oplist_push_op(arena, list, RDI_EvalOp_ConstU64, e_value_u64((U64)x));
  }
}

internal void
e_oplist_push_bytecode(Arena *arena, E_OpList *list, String8 bytecode)
{
  E_Op *node = push_array_no_zero(arena, E_Op, 1);
  node->opcode = E_IRExtKind_Bytecode;
  node->string = bytecode;
  SLLQueuePush(list->first, list->last, node);
  list->op_count += 1;
  list->encoded_size += bytecode.size;
}

internal void
e_oplist_push_set_space(Arena *arena, E_OpList *list, E_Space space)
{
  E_Op *node = push_array_no_zero(arena, E_Op, 1);
  node->opcode = E_IRExtKind_SetSpace;
  StaticAssert(sizeof(E_Space) <= sizeof(E_Value), space_size_check);
  MemoryCopy(&node->value, &space, sizeof(space));
  SLLQueuePush(list->first, list->last, node);
  list->op_count += 1;
  list->encoded_size += 1 + sizeof(space);
}

internal void
e_oplist_push_string_literal(Arena *arena, E_OpList *list, String8 string)
{
  RDI_EvalOp opcode = RDI_EvalOp_ConstString;
  U16 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
  U32 p_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
  E_Op *node = push_array_no_zero(arena, E_Op, 1);
  node->opcode = opcode;
  node->string = string;
  node->value.u64 = Min(string.size, 64);
  SLLQueuePush(list->first, list->last, node);
  list->op_count += 1;
  list->encoded_size += 1 + p_size + node->value.u64;
}

internal void
e_oplist_concat_in_place(E_OpList *dst, E_OpList *to_push)
{
  if(to_push->first && dst->first)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->op_count += to_push->op_count;
    dst->encoded_size += to_push->encoded_size;
  }
  else if(!dst->first)
  {
    MemoryCopyStruct(dst, to_push);
  }
  MemoryZeroStruct(to_push);
}

//- rjf: ir tree core building helpers

internal E_IRNode *
e_push_irnode(Arena *arena, RDI_EvalOp op)
{
  E_IRNode *n = push_array(arena, E_IRNode, 1);
  n->first = n->last = n->next = &e_irnode_nil;
  n->op = op;
  return n;
}

internal void
e_irnode_push_child(E_IRNode *parent, E_IRNode *child)
{
  SLLQueuePush_NZ(&e_irnode_nil, parent->first, parent->last, child, next);
}

//- rjf: ir subtree building helpers

internal E_IRNode *
e_irtree_const_u(Arena *arena, U64 v)
{
  // rjf: choose op
  RDI_EvalOp op = RDI_EvalOp_ConstU64;
  if     (v < 0x100)       { op = RDI_EvalOp_ConstU8; }
  else if(v < 0x10000)     { op = RDI_EvalOp_ConstU16; }
  else if(v < 0x100000000) { op = RDI_EvalOp_ConstU32; }
  
  // rjf: build
  E_IRNode *n = e_push_irnode(arena, op);
  n->value.u64 = v;
  return n;
}

internal E_IRNode *
e_irtree_leaf_u128(Arena *arena, U128 u128)
{
  E_IRNode *n = e_push_irnode(arena, RDI_EvalOp_ConstU128);
  n->value.u128 = u128;
  return n;
}

internal E_IRNode *
e_irtree_unary_op(Arena *arena, RDI_EvalOp op, RDI_EvalTypeGroup group, E_IRNode *c)
{
  E_IRNode *n = e_push_irnode(arena, op);
  n->value.u64 = group;
  e_irnode_push_child(n, c);
  return n;
}

internal E_IRNode *
e_irtree_binary_op(Arena *arena, RDI_EvalOp op, RDI_EvalTypeGroup group, E_IRNode *l, E_IRNode *r)
{
  E_IRNode *n = e_push_irnode(arena, op);
  n->value.u64 = group;
  e_irnode_push_child(n, l);
  e_irnode_push_child(n, r);
  return n;
}

internal E_IRNode *
e_irtree_binary_op_u(Arena *arena, RDI_EvalOp op, E_IRNode *l, E_IRNode *r)
{
  E_IRNode *n = e_irtree_binary_op(arena, op, RDI_EvalTypeGroup_U, l, r);
  return n;
}

internal E_IRNode *
e_irtree_conditional(Arena *arena, E_IRNode *c, E_IRNode *l, E_IRNode *r)
{
  E_IRNode *n = e_push_irnode(arena, RDI_EvalOp_Cond);
  e_irnode_push_child(n, c);
  e_irnode_push_child(n, l);
  e_irnode_push_child(n, r);
  return n;
}

internal E_IRNode *
e_irtree_bytecode_no_copy(Arena *arena, String8 bytecode)
{
  E_IRNode *n = e_push_irnode(arena, E_IRExtKind_Bytecode);
  n->string = bytecode;
  return n;
}

internal E_IRNode *
e_irtree_string_literal(Arena *arena, String8 string)
{
  E_IRNode *root = e_push_irnode(arena, RDI_EvalOp_ConstString);
  root->string = string;
  return root;
}

internal E_IRNode *
e_irtree_set_space(Arena *arena, E_Space space, E_IRNode *c)
{
  E_IRNode *root = e_push_irnode(arena, E_IRExtKind_SetSpace);
  StaticAssert(sizeof(E_Space) <= sizeof(E_Value), space_size_check);
  MemoryCopy(&root->value, &space, sizeof(space));
  e_irnode_push_child(root, c);
  return root;
}

internal E_IRNode *
e_irtree_mem_read_type(Arena *arena, E_IRNode *c, E_TypeKey type_key)
{
  E_IRNode *result = &e_irnode_nil;
  U64 byte_size = e_type_byte_size_from_key(type_key);
  byte_size = Min(64, byte_size);
  
  // rjf: build the read node
  E_IRNode *read_node = e_push_irnode(arena, RDI_EvalOp_MemRead);
  read_node->value.u64 = byte_size;
  e_irnode_push_child(read_node, c);
  
  // rjf: build a signed trunc node if needed
  U64 bit_size = byte_size << 3;
  E_IRNode *with_trunc = read_node;
  E_TypeKind kind = e_type_kind_from_key(type_key);
  if(bit_size < 64 && e_type_kind_is_signed(kind))
  {
    with_trunc = e_push_irnode(arena, RDI_EvalOp_TruncSigned);
    with_trunc->value.u64 = bit_size;
    e_irnode_push_child(with_trunc, read_node);
  }
  
  // rjf: fill
  result = with_trunc;
  
  return result;
}

internal E_IRNode *
e_irtree_convert_lo(Arena *arena, E_IRNode *c, RDI_EvalTypeGroup out, RDI_EvalTypeGroup in)
{
  E_IRNode *n = e_push_irnode(arena, RDI_EvalOp_Convert);
  n->value.u64 = in | (out << 8);
  e_irnode_push_child(n, c);
  return n;
}

internal E_IRNode *
e_irtree_trunc(Arena *arena, E_IRNode *c, E_TypeKey type_key)
{
  E_IRNode *result = c;
  U64 byte_size = e_type_byte_size_from_key(type_key);
  if(byte_size < 64)
  {
    RDI_EvalOp op = RDI_EvalOp_Trunc;
    E_TypeKind kind = e_type_kind_from_key(type_key);
    if(e_type_kind_is_signed(kind))
    {
      op = RDI_EvalOp_TruncSigned;
    }
    U64 bit_size = byte_size << 3;
    result = e_push_irnode(arena, op);
    result->value.u64 = bit_size;
    e_irnode_push_child(result, c);
  }
  return result;
}

internal E_IRNode *
e_irtree_convert_hi(Arena *arena, E_IRNode *c, E_TypeKey out, E_TypeKey in)
{
  E_IRNode *result = c;
  E_TypeKind in_kind = e_type_kind_from_key(in);
  E_TypeKind out_kind = e_type_kind_from_key(out);
  U8 in_group  = e_type_group_from_kind(in_kind);
  U8 out_group = e_type_group_from_kind(out_kind);
  U32 conversion_rule = rdi_eval_conversion_kind_from_typegroups(in_group, out_group);
  if(conversion_rule == RDI_EvalConversionKind_Legal)
  {
    result = e_irtree_convert_lo(arena, result, out_group, in_group);
  }
  U64 in_byte_size = e_type_byte_size_from_key(in);
  U64 out_byte_size = e_type_byte_size_from_key(out);
  if(out_byte_size < in_byte_size && e_type_kind_is_integer(out_kind))
  {
    result = e_irtree_trunc(arena, result, out);
  }
  return result;
}

internal E_IRNode *
e_irtree_resolve_to_value(Arena *arena, E_Mode from_mode, E_IRNode *tree, E_TypeKey type_key)
{
  E_IRNode *result = tree;
  if(from_mode == E_Mode_Offset)
  {
    result = e_irtree_mem_read_type(arena, tree, type_key);
  }
  return result;
}

//- rjf: top-level irtree/type extraction

E_IRGEN_FUNCTION_DEF(default)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  E_ExprKind kind = expr->kind;
  switch(kind)
  {
    default:{}break;
    
    //- rjf: accesses
    case E_ExprKind_MemberAccess:
    case E_ExprKind_ArrayIndex:
    {
      Temp scratch = scratch_begin(&arena, 1);
      E_Expr *lhs = expr->first;
      E_Expr *rhs = lhs->next;
      E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(scratch.arena, lhs);
      
      // rjf: determine lookup rule - first check explicitly-specified tags
      E_LookupRule *lookup_rule = &e_lookup_rule__default;
      for(E_Expr *tag = lhs->first_tag; tag != &e_expr_nil; tag = tag->next)
      {
        E_LookupRule *candidate = e_lookup_rule_from_string(tag->string);
        if(candidate != &e_lookup_rule__nil)
        {
          lookup_rule = candidate;
          break;
        }
      }
      
      // rjf: if the lookup rule is the default, try to (a) apply set-name hooks, or (b) apply auto-hooks
      if(lookup_rule == &e_lookup_rule__default)
      {
        // rjf: try set name
        E_Type *lhs_type = e_type_from_key__cached(lhs_irtree.type_key);
        if(lhs_type->kind == E_TypeKind_Set)
        {
          E_LookupRule *candidate = e_lookup_rule_from_string(lhs_type->name);
          if(candidate != &e_lookup_rule__nil)
          {
            lookup_rule = candidate;
          }
        }
        
        // rjf: try auto tags
        if(lookup_rule == &e_lookup_rule__default)
        {
          E_ExprList auto_tags = e_auto_hook_tag_exprs_from_type_key__cached(lhs_irtree.type_key);
          for(E_ExprNode *n = auto_tags.first; n != 0; n = n->next)
          {
            E_LookupRule *candidate = e_lookup_rule_from_string(n->v->string);
            if(candidate != &e_lookup_rule__nil)
            {
              lookup_rule = candidate;
              break;
            }
          }
        }
      }
      
      // rjf: use lookup rule to actually do the access
      ProfScope("lookup via rule '%.*s'", str8_varg(lookup_rule->name))
      {
        E_LookupInfo lookup_info = lookup_rule->info(arena, &lhs_irtree, str8_zero());
        E_LookupAccess lookup_access = lookup_rule->access(arena, expr->kind, lhs, rhs, lookup_info.user_data);
        result = lookup_access.irtree_and_type;
      }
      scratch_end(scratch);
    }break;
    
    //- rjf: dereference
    case E_ExprKind_Deref:
    {
      // rjf: unpack operand
      E_Expr *r_expr = expr->first;
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      E_TypeKey r_type = e_type_unwrap(r_tree.type_key);
      E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
      E_TypeKey r_type_direct = e_type_direct_from_key(r_type);
      U64 r_type_direct_size = e_type_byte_size_from_key(r_type_direct);
      e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(r_tree.root->op == 0)
      {
        break;
      }
      else if(r_type_direct_size == 0 &&
              (r_type_kind == E_TypeKind_Ptr ||
               r_type_kind == E_TypeKind_LRef ||
               r_type_kind == E_TypeKind_RRef))
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, r_expr->location, "Cannot dereference pointers of zero-sized types.");
        break;
      }
      else if(r_type_direct_size == 0 && r_type_kind == E_TypeKind_Array)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, r_expr->location, "Cannot dereference arrays of zero-sized types.");
        break;
      }
      else if(r_type_kind != E_TypeKind_Array &&
              r_type_kind != E_TypeKind_Ptr &&
              r_type_kind != E_TypeKind_LRef &&
              r_type_kind != E_TypeKind_RRef)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, r_expr->location, "Cannot dereference this type.");
        break;
      }
      
      // rjf: generate
      {
        E_IRNode *new_tree = r_tree.root;
        if(r_tree.mode != E_Mode_Value &&
           (r_type_kind == E_TypeKind_Ptr ||
            r_type_kind == E_TypeKind_LRef ||
            r_type_kind == E_TypeKind_RRef))
        {
          new_tree = e_irtree_resolve_to_value(arena, r_tree.mode, r_tree.root, r_type);
        }
        result.root     = new_tree;
        result.type_key = r_type_direct;
        result.mode     = E_Mode_Offset;
      }
    }break;
    
    //- rjf: address-of
    case E_ExprKind_Address:
    {
      // rjf: unpack operand
      E_Expr *r_expr = expr->first;
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      E_TypeKey r_type = r_tree.type_key;
      E_TypeKey r_type_unwrapped = e_type_unwrap(r_type);
      E_TypeKind r_type_unwrapped_kind = e_type_kind_from_key(r_type_unwrapped);
      e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(r_tree.root->op == 0)
      {
        break;
      }
      else if(e_type_key_match(e_type_key_zero(), r_type_unwrapped))
      {
        break;
      }
      
      // rjf: generate
      result.root     = r_tree.root;
      result.type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, r_type_unwrapped, 1, 0);
      result.mode     = E_Mode_Value;
    }break;
    
    //- rjf: cast
    case E_ExprKind_Cast:
    {
      // rjf: unpack operands
      E_Expr *cast_type_expr = expr->first;
      E_Expr *casted_expr = cast_type_expr->next;
      E_TypeKey cast_type = e_type_from_expr(cast_type_expr);
      E_TypeKind cast_type_kind = e_type_kind_from_key(cast_type);
      U64 cast_type_byte_size = e_type_byte_size_from_key(cast_type);
      E_IRTreeAndType casted_tree = e_irtree_and_type_from_expr(arena, casted_expr);
      e_msg_list_concat_in_place(&result.msgs, &casted_tree.msgs);
      E_TypeKey casted_type = e_type_unwrap(casted_tree.type_key);
      E_TypeKind casted_type_kind = e_type_kind_from_key(casted_type);
      U64 casted_type_byte_size = e_type_byte_size_from_key(casted_type);
      U8 in_group  = e_type_group_from_kind(casted_type_kind);
      U8 out_group = e_type_group_from_kind(cast_type_kind);
      RDI_EvalConversionKind conversion_rule = rdi_eval_conversion_kind_from_typegroups(in_group, out_group);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(casted_tree.root->op == 0)
      {
        break;
      }
      else if(cast_type_kind == E_TypeKind_Null)
      {
        break;
      }
      else if(conversion_rule != RDI_EvalConversionKind_Noop &&
              conversion_rule != RDI_EvalConversionKind_Legal)
      {
        String8 text = str8_lit("Unknown cast conversion rule.");
        if(conversion_rule < RDI_EvalConversionKind_COUNT)
        {
          text.str = rdi_explanation_string_from_eval_conversion_kind(conversion_rule, &text.size);
        }
        e_msg(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, text);
        break;
      }
      
      // rjf: generate
      {
        E_IRNode *in_tree = e_irtree_resolve_to_value(arena, casted_tree.mode, casted_tree.root, casted_type);
        E_IRNode *new_tree = in_tree;
        if(conversion_rule == RDI_EvalConversionKind_Legal)
        {
          new_tree = e_irtree_convert_lo(arena, in_tree, out_group, in_group);
        }
        if(cast_type_byte_size < casted_type_byte_size && e_type_kind_is_integer(cast_type_kind))
        {
          new_tree = e_irtree_trunc(arena, in_tree, cast_type);
        }
        result.root     = new_tree;
        result.type_key = cast_type;
        result.mode     = E_Mode_Value;
      }
    }break;
    
    //- rjf: sizeof
    case E_ExprKind_Sizeof:
    {
      // rjf: unpack operand
      E_Expr *r_expr = expr->first;
      E_TypeKey r_type = zero_struct;
      E_Space space = r_expr->space;
      switch(r_expr->kind)
      {
        case E_ExprKind_TypeIdent:
        case E_ExprKind_Ptr:
        case E_ExprKind_Array:
        case E_ExprKind_Func:
        {
          r_type = e_type_from_expr(r_expr);
        }break;
        default:
        {
          E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
          e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
          r_type = r_tree.type_key;
        }break;
      }
      
      // rjf: bad conditions? -> error if applicable, exit
      if(e_type_key_match(r_type, e_type_key_zero()))
      {
        break;
      }
      else if(e_type_kind_from_key(r_type) == E_TypeKind_Null)
      {
        break;
      }
      
      // rjf: generate
      {
        U64 r_type_byte_size = e_type_byte_size_from_key(r_type);
        result.root     = e_irtree_const_u(arena, r_type_byte_size);
        result.type_key = e_type_key_basic(E_TypeKind_U64);
        result.mode     = E_Mode_Value;
      }
    }break;
    
    //- rjf: typeof
    case E_ExprKind_Typeof:
    {
      // rjf: evaluate operand tree
      E_Expr *r_expr = expr->first;
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
      
      // rjf: fill output
      result.root     = e_irtree_const_u(arena, 0);
      result.type_key = r_tree.type_key;
      result.mode     = E_Mode_Null;
    }break;
    
    //- rjf: byteswap
    case E_ExprKind_ByteSwap:
    {
      // rjf: unpack operand
      E_Expr *r_expr = expr->first;
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
      E_TypeKey r_type = e_type_unwrap(r_tree.type_key);
      E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
      U64 r_type_size = e_type_byte_size_from_key(r_type);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(!e_type_kind_is_integer(r_type_kind) || (r_type_size != 8 && r_type_size != 4 && r_type_size != 2))
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Byteswapping this type is not supported.");
        break;
      }
      
      // rjf: generate
      {
        E_IRNode *node = e_push_irnode(arena, RDI_EvalOp_ByteSwap);
        E_IRNode *rhs = e_irtree_resolve_to_value(arena, r_tree.mode, r_tree.root, r_type);
        e_irnode_push_child(node, rhs);
        node->value.u64 = r_type_size;
        result.root = node;
        result.mode = E_Mode_Value;
        result.type_key = r_type;
      }
    }break;
    
    //- rjf: unary operations
    case E_ExprKind_Pos:
    {
      result = e_irtree_and_type_from_expr(arena, expr->first);
    }break;
    case E_ExprKind_Neg:
    case E_ExprKind_LogNot:
    case E_ExprKind_BitNot:
    {
      // rjf: unpack operand
      E_Expr *r_expr = expr->first;
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      E_TypeKey r_type = e_type_unwrap(r_tree.type_key);
      E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
      RDI_EvalTypeGroup r_type_group = e_type_group_from_kind(r_type_kind);
      E_TypeKey r_type_promoted = e_type_promote(r_type);
      RDI_EvalOp op = e_opcode_from_expr_kind(kind);
      e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(r_tree.root->op == 0)
      {
        break;
      }
      else if(!rdi_eval_op_typegroup_are_compatible(op, r_type_group))
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Cannot use this operator on this type.");
        break;
      }
      
      // rjf: generate
      {
        E_IRNode *in_tree = e_irtree_resolve_to_value(arena, r_tree.mode, r_tree.root, r_type);
        in_tree = e_irtree_convert_hi(arena, in_tree, r_type_promoted, r_type);
        E_IRNode *new_tree = e_irtree_unary_op(arena, op, r_type_group, in_tree);
        result.root     = new_tree;
        result.type_key = r_type_promoted;
        result.mode     = E_Mode_Value;
      }
    }break;
    
    //- rjf: binary operations
    case E_ExprKind_Mul:
    case E_ExprKind_Div:
    case E_ExprKind_Mod:
    case E_ExprKind_Add:
    case E_ExprKind_Sub:
    case E_ExprKind_LShift:
    case E_ExprKind_RShift:
    case E_ExprKind_Less:
    case E_ExprKind_LsEq:
    case E_ExprKind_Grtr:
    case E_ExprKind_GrEq:
    case E_ExprKind_EqEq:
    case E_ExprKind_NtEq:
    case E_ExprKind_BitAnd:
    case E_ExprKind_BitXor:
    case E_ExprKind_BitOr:
    case E_ExprKind_LogAnd:
    case E_ExprKind_LogOr:
    {
      // rjf: unpack operands
      RDI_EvalOp op = e_opcode_from_expr_kind(kind);
      B32 is_comparison = e_expr_kind_is_comparison(kind);
      E_Expr *l_expr = expr->first;
      E_Expr *r_expr = l_expr->next;
      E_IRTreeAndType l_tree = e_irtree_and_type_from_expr(arena, l_expr);
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      e_msg_list_concat_in_place(&result.msgs, &l_tree.msgs);
      e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
      E_TypeKey l_type = e_type_unwrap_enum(e_type_unwrap(l_tree.type_key));
      E_TypeKey r_type = e_type_unwrap_enum(e_type_unwrap(r_tree.type_key));
      E_TypeKind l_type_kind = e_type_kind_from_key(l_type);
      E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
      if(l_type.kind == E_TypeKeyKind_Reg)
      {
        l_type_kind = E_TypeKind_U64;
        l_type = e_type_key_basic(l_type_kind);
      }
      if(r_type.kind == E_TypeKeyKind_Reg)
      {
        r_type_kind = E_TypeKind_U64;
        r_type = e_type_key_basic(r_type_kind);
      }
      B32 l_is_pointer      = (l_type_kind == E_TypeKind_Ptr);
      B32 l_is_decay        = (l_type_kind == E_TypeKind_Array && l_tree.mode == E_Mode_Offset);
      B32 l_is_pointer_like = (l_is_pointer || l_is_decay);
      B32 r_is_pointer      = (r_type_kind == E_TypeKind_Ptr);
      B32 r_is_decay        = (r_type_kind == E_TypeKind_Array && r_tree.mode == E_Mode_Offset);
      B32 r_is_pointer_like = (r_is_pointer || r_is_decay);
      RDI_EvalTypeGroup l_type_group = e_type_group_from_kind(l_type_kind);
      RDI_EvalTypeGroup r_type_group = e_type_group_from_kind(r_type_kind);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(l_tree.root->op == 0 || r_tree.root->op == 0)
      {
        break;
      }
      
      // rjf: determine arithmetic path
#define E_ArithPath_Normal          0
#define E_ArithPath_PtrAdd          1
#define E_ArithPath_PtrSub          2
#define E_ArithPath_PtrArrayCompare 3
      B32 ptr_arithmetic_mul_rptr = 0;
      U32 arith_path = E_ArithPath_Normal;
      if(kind == E_ExprKind_Add)
      {
        if(l_is_pointer_like && e_type_kind_is_integer(r_type_kind))
        {
          arith_path = E_ArithPath_PtrAdd;
        }
        if(l_is_pointer_like && e_type_kind_is_integer(l_type_kind))
        {
          arith_path = E_ArithPath_PtrAdd;
          ptr_arithmetic_mul_rptr = 1;
        }
      }
      else if(kind == E_ExprKind_Sub)
      {
        if(l_is_pointer_like && e_type_kind_is_integer(r_type_kind))
        {
          arith_path = E_ArithPath_PtrAdd;
        }
        if(l_is_pointer_like && r_is_pointer_like)
        {
          E_TypeKey l_type_direct = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(l_type)));
          E_TypeKey r_type_direct = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(r_type)));
          U64 l_type_direct_byte_size = e_type_byte_size_from_key(l_type_direct);
          U64 r_type_direct_byte_size = e_type_byte_size_from_key(r_type_direct);
          if(l_type_direct_byte_size == r_type_direct_byte_size)
          {
            arith_path = E_ArithPath_PtrSub;
          }
        }
      }
      else if(kind == E_ExprKind_EqEq)
      {
        if(l_type_kind == E_TypeKind_Array && (r_type_kind == E_TypeKind_Ptr || r_is_decay))
        {
          arith_path = E_ArithPath_PtrArrayCompare;
        }
        if(r_type_kind == E_TypeKind_Array && (l_type_kind == E_TypeKind_Ptr || l_is_decay))
        {
          arith_path = E_ArithPath_PtrArrayCompare;
        }
      }
      
      // rjf: generate according to arithmetic path
      switch(arith_path)
      {
        //- rjf: normal arithmetic
        case E_ArithPath_Normal:
        {
          // rjf: bad conditions? -> error if applicable, exit
          if(!rdi_eval_op_typegroup_are_compatible(op, l_type_group) ||
             !rdi_eval_op_typegroup_are_compatible(op, r_type_group))
          {
            e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Cannot use this operator on this type.");
            break;
          }
          
          // rjf: generate
          {
            E_TypeKey final_type_key = is_comparison ? e_type_key_basic(E_TypeKind_Bool) : l_type;
            E_IRNode *l_value_tree = e_irtree_resolve_to_value(arena, l_tree.mode, l_tree.root, l_type);
            E_IRNode *r_value_tree = e_irtree_resolve_to_value(arena, r_tree.mode, r_tree.root, r_type);
            l_value_tree = e_irtree_convert_hi(arena, l_value_tree, l_type, l_type);
            r_value_tree = e_irtree_convert_hi(arena, r_value_tree, l_type, r_type);
            E_IRNode *new_tree = e_irtree_binary_op(arena, op, l_type_group, l_value_tree, r_value_tree);
            result.root     = new_tree;
            result.type_key = final_type_key;
            result.mode     = E_Mode_Value;
          }
        }break;
        
        //- rjf: pointer addition
        case E_ArithPath_PtrAdd:
        {
          // rjf: map l/r to ptr/int
          E_IRTreeAndType *ptr_tree = &l_tree;
          E_IRTreeAndType *int_tree = &r_tree;
          B32 ptr_is_decay = l_is_decay;
          if(ptr_arithmetic_mul_rptr)
          {
            ptr_tree = &r_tree;
            int_tree = &l_tree;
            ptr_is_decay = r_is_decay;
          }
          
          // rjf: unpack type
          E_TypeKey direct_type = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(ptr_tree->type_key)));
          U64 direct_type_size = e_type_byte_size_from_key(direct_type);
          
          // rjf: generate
          {
            E_IRNode *ptr_root = ptr_tree->root;
            if(!ptr_is_decay)
            {
              ptr_root = e_irtree_resolve_to_value(arena, ptr_tree->mode, ptr_root, ptr_tree->type_key);
            }
            E_IRNode *int_root = int_tree->root;
            int_root = e_irtree_resolve_to_value(arena, int_tree->mode, int_root, int_tree->type_key);
            if(direct_type_size > 1)
            {
              E_IRNode *const_root = e_irtree_const_u(arena, direct_type_size);
              int_root = e_irtree_binary_op_u(arena, RDI_EvalOp_Mul, int_root, const_root);
            }
            E_TypeKey ptr_type = ptr_tree->type_key;
            if(ptr_is_decay)
            {
              ptr_type = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, direct_type, 1, 0);
            }
            E_IRNode *new_root = e_irtree_binary_op_u(arena, op, ptr_root, int_root);
            result.root     = new_root;
            result.type_key = ptr_type;
            result.mode     = E_Mode_Value;
          }
        }break;
        
        //- rjf: pointer subtraction
        case E_ArithPath_PtrSub:
        {
          // rjf: unpack type
          E_TypeKey direct_type = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(l_type)));
          U64 direct_type_size = e_type_byte_size_from_key(direct_type);
          
          // rjf: generate
          E_IRNode *l_root = l_tree.root;
          E_IRNode *r_root = r_tree.root;
          if(!l_is_decay)
          {
            l_root = e_irtree_resolve_to_value(arena, l_tree.mode, l_root, l_type);
          }
          if(!r_is_decay)
          {
            r_root = e_irtree_resolve_to_value(arena, r_tree.mode, r_root, r_type);
          }
          E_IRNode *op_tree = e_irtree_binary_op_u(arena, op, l_root, r_root);
          E_IRNode *new_tree = op_tree;
          if(direct_type_size > 1)
          {
            E_IRNode *const_tree = e_irtree_const_u(arena, direct_type_size);
            new_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Div, new_tree, const_tree);
          }
          result.root     = new_tree;
          result.type_key = e_type_key_basic(E_TypeKind_U64);
          result.mode     = E_Mode_Value;
        }break;
        
        //- rjf: pointer array comparison
        case E_ArithPath_PtrArrayCompare:
        {
          // rjf: map l/r to pointer/array
          B32 ptr_is_decay = l_is_decay;
          E_IRTreeAndType *ptr_tree = &l_tree;
          E_IRTreeAndType *arr_tree = &r_tree;
          if(l_type_kind == E_TypeKind_Array && l_tree.mode == E_Mode_Value)
          {
            ptr_is_decay = r_is_decay;
            ptr_tree = &r_tree;
            arr_tree = &l_tree;
          }
          
          // rjf: resolve pointer to value, sized same as array
          E_IRNode *ptr_root = ptr_tree->root;
          E_IRNode *arr_root = arr_tree->root;
          if(!ptr_is_decay)
          {
            ptr_root = e_irtree_resolve_to_value(arena, ptr_tree->mode, ptr_tree->root, ptr_tree->type_key);
          }
          
          // rjf: read from pointer into value, to compare with array
          E_IRNode *mem_root = e_irtree_mem_read_type(arena, ptr_root, arr_tree->type_key);
          
          // rjf: generate
          result.root     = e_irtree_binary_op(arena, op, RDI_EvalTypeGroup_Other, mem_root, arr_root);
          result.type_key = e_type_key_basic(E_TypeKind_Bool);
          result.mode     = E_Mode_Value;
        }break;
      }
    }break;
    
    //- rjf: ternary operators
    case E_ExprKind_Ternary:
    {
      // rjf: unpack operands
      E_Expr *c_expr = expr->first;
      E_Expr *l_expr = c_expr->next;
      E_Expr *r_expr = l_expr->next;
      E_IRTreeAndType c_tree = e_irtree_and_type_from_expr(arena, c_expr);
      E_IRTreeAndType l_tree = e_irtree_and_type_from_expr(arena, l_expr);
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      e_msg_list_concat_in_place(&result.msgs, &c_tree.msgs);
      e_msg_list_concat_in_place(&result.msgs, &l_tree.msgs);
      e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
      E_TypeKey c_type = e_type_unwrap(c_tree.type_key);
      E_TypeKey l_type = e_type_unwrap(l_tree.type_key);
      E_TypeKey r_type = e_type_unwrap(r_tree.type_key);
      E_TypeKind c_type_kind = e_type_kind_from_key(c_type);
      E_TypeKind l_type_kind = e_type_kind_from_key(l_type);
      E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
      E_TypeKey result_type = l_type;
      
      // rjf: bad conditions? -> error if applicable, exit
      if(c_tree.root->op == 0 || l_tree.root->op == 0 || r_tree.root->op == 0)
      {
        break;
      }
      else if(!e_type_kind_is_integer(c_type_kind) && c_type_kind != E_TypeKind_Bool)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Conditional term must be an integer or boolean type.");
      }
      else if(!e_type_match(l_type, r_type))
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Left and right terms must have matching types.");
      }
      
      // rjf: generate
      {
        E_IRNode *c_value_tree = e_irtree_resolve_to_value(arena, c_tree.mode, c_tree.root, c_type);
        E_IRNode *l_value_tree = e_irtree_resolve_to_value(arena, l_tree.mode, l_tree.root, l_type);
        E_IRNode *r_value_tree = e_irtree_resolve_to_value(arena, r_tree.mode, r_tree.root, r_type);
        l_value_tree = e_irtree_convert_hi(arena, l_value_tree, result_type, l_type);
        r_value_tree = e_irtree_convert_hi(arena, r_value_tree, result_type, r_type);
        E_IRNode *new_tree = e_irtree_conditional(arena, c_value_tree, l_value_tree, r_value_tree);
        result.root     = new_tree;
        result.type_key = result_type;
        result.mode     = E_Mode_Value;
      }
    }break;
    
    //- rjf: leaf bytecode
    case E_ExprKind_LeafBytecode:
    {
      E_IRNode *new_tree = e_irtree_bytecode_no_copy(arena, expr->bytecode);
      new_tree->space = expr->space;
      E_TypeKey final_type_key = expr->type_key;
      result.root     = new_tree;
      result.type_key = final_type_key;
      result.mode     = expr->mode;
    }break;
    
    //- rjf: (unexpected) leaf member
    case E_ExprKind_LeafMember:
    {
      e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "(internal) Leaf member not expected here.");
    }break;
    
    //- rjf: leaf string literal
    case E_ExprKind_LeafStringLiteral:
    {
      String8 string = expr->string;
      E_TypeKey type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_UChar8), string.size);
      E_IRNode *new_tree = e_irtree_string_literal(arena, string);
      result.root     = new_tree;
      result.type_key = type_key;
      result.mode     = E_Mode_Value;
    }break;
    
    //- rjf: leaf bools
    case E_ExprKind_LeafBool:
    {
      result.root     = e_irtree_const_u(arena, expr->value.u64);
      result.type_key = expr->type_key;
      result.mode     = E_Mode_Value;
    }break;
    
    //- rjf: leaf U64s
    case E_ExprKind_LeafU64:
    {
      U64 val = expr->value.u64;
      E_IRNode *new_tree = e_irtree_const_u(arena, val);
      E_TypeKey type_key = zero_struct;
      if(0){}
      else if(val <= max_S32){type_key = e_type_key_basic(E_TypeKind_S32);}
      else if(val <= max_S64){type_key = e_type_key_basic(E_TypeKind_S64);}
      else                   {type_key = e_type_key_basic(E_TypeKind_U64);}
      result.root     = new_tree;
      result.type_key = type_key;
      result.mode     = E_Mode_Value;
    }break;
    
    //- rjf: leaf F64s
    case E_ExprKind_LeafF64:
    {
      U64 val = expr->value.u64;
      E_IRNode *new_tree = e_irtree_const_u(arena, val);
      result.root     = new_tree;
      result.type_key = e_type_key_basic(E_TypeKind_F64);
      result.mode     = E_Mode_Value;
    }break;
    
    //- rjf: leaf F32s
    case E_ExprKind_LeafF32:
    {
      U32 val = expr->value.u32;
      E_IRNode *new_tree = e_irtree_const_u(arena, val);
      result.root     = new_tree;
      result.type_key = e_type_key_basic(E_TypeKind_F32);
      result.mode     = E_Mode_Value;
    }break;
    
    //- rjf: leaf identifiers
    case E_ExprKind_LeafIdent:
    {
      E_Expr *macro_expr = e_string2expr_lookup(e_ir_state->ctx->macro_map, expr->string);
      if(macro_expr == &e_expr_nil)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_ResolutionFailure, expr->location, "`%S` could not be found.", expr->string);
      }
      else
      {
        e_string2expr_map_inc_poison(e_ir_state->ctx->macro_map, expr->string);
        result = e_irtree_and_type_from_expr(arena, macro_expr);
        e_string2expr_map_dec_poison(e_ir_state->ctx->macro_map, expr->string);
      }
    }break;
    
    //- rjf: leaf offsets
    case E_ExprKind_LeafOffset:
    {
      E_IRNode *new_tree = e_push_irnode(arena, RDI_EvalOp_ConstU64);
      new_tree->value.u64 = expr->value.u64;
      new_tree->space = expr->space;
      result.root     = new_tree;
      result.type_key = expr->type_key;
      result.mode     = E_Mode_Offset;
    }break;
    
    //- rjf: leaf file paths
    case E_ExprKind_LeafFilePath:
    {
      U128 key = fs_key_from_path_range(expr->string, r1u64(0, max_U64));
      E_Space space = {E_SpaceKind_HashStoreKey, .u128 = key};
      U64 size = fs_size_from_path(expr->string);
      E_IRNode *base_offset = e_irtree_const_u(arena, 0);
      base_offset->space = space;
      result.root     = base_offset;
      result.type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), size);
      result.mode     = E_Mode_Offset;
    }break;
    
    //- rjf: types
    case E_ExprKind_TypeIdent:
    {
      result.root = e_irtree_const_u(arena, 0);
      result.root->space = expr->space;
      result.type_key = expr->type_key;
      result.mode = E_Mode_Null;
    }break;
    
    //- rjf: (unexpected) type expressions
    case E_ExprKind_Ptr:
    case E_ExprKind_Array:
    case E_ExprKind_Func:
    {
      e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Type expression not expected.");
    }break;
    
    //- rjf: textual line slicing
    case E_ExprKind_Line:
    {
      E_Expr *lhs = expr->first;
      E_Expr *rhs = expr->last;
      E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(arena, lhs);
      U64 line_num = rhs->value.u64;
      B32 space_is_good = 1;
      E_Space space = {0};
      if(lhs_irtree.root->op != E_IRExtKind_SetSpace)
      {
        space_is_good = 0;
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, lhs->location, "Cannot take a line from a non-file.");
      }
      else
      {
        MemoryCopy(&space, &lhs_irtree.root->value, sizeof(space));
      }
      B32 line_num_is_good = 1;
      if(rhs->kind != E_ExprKind_LeafU64)
      {
        line_num_is_good = 0;
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, rhs->location, "Line number must be specified as a constant number.");
      }
      if(space_is_good && line_num_is_good)
      {
        TXT_Scope *txt_scope = txt_scope_open();
        U128 key = space.u128;
        U128 hash = {0};
        TXT_TextInfo text_info = txt_text_info_from_key_lang(txt_scope, key, TXT_LangKind_Null, &hash);
        if(1 <= line_num && line_num <= text_info.lines_count)
        {
          Rng1U64 line_range = text_info.lines_ranges[line_num-1];
          U64 line_size = dim_1u64(line_range);
          E_IRNode *line_offset = e_irtree_const_u(arena, line_range.min);
          result.root        = line_offset;
          result.root->space = space;
          result.type_key    = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), line_size);
          result.mode        = E_Mode_Offset;
        }
        else
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, rhs->location, "Line %I64u is out of bounds.", line_num);
        }
        txt_scope_close(txt_scope);
      }
    }break;
    
    //- rjf: definitions
    case E_ExprKind_Define:
    {
      E_Expr *lhs = expr->first;
      E_Expr *rhs = lhs->next;
      result = e_irtree_and_type_from_expr(arena, rhs);
      if(lhs->kind != E_ExprKind_LeafIdent)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Left side of assignment must be an unused identifier.");
      }
    }break;
  }
  return result;
}

internal E_IRTreeAndType
e_irtree_and_type_from_expr(Arena *arena, E_Expr *expr)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType result = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_Ref)
  {
    expr = expr->ref;
  }
  
  //- rjf: pick the ir-generation rule from explicitly-stored expressions
  E_IRGenRule *explicit_irgen_rule = &e_irgen_rule__default;
  E_Expr *explicit_irgen_rule_tag = &e_expr_nil;
  for(E_Expr *tag = expr->first_tag; tag != &e_expr_nil; tag = tag->next)
  {
    String8 name = tag->string;
    E_IRGenRule *irgen_rule_candidate = e_irgen_rule_from_string(name);
    if(irgen_rule_candidate != &e_irgen_rule__default)
    {
      B32 tag_is_poisoned = 0;
      U64 hash = e_hash_from_string(5381, str8_struct(&tag));
      U64 slot_idx = hash%e_ir_state->used_tag_map->slots_count;
      for(E_UsedTagNode *n = e_ir_state->used_tag_map->slots[slot_idx].first; n != 0; n = n->next)
      {
        if(n->tag == tag)
        {
          tag_is_poisoned = 1;
          break;
        }
      }
      if(!tag_is_poisoned)
      {
        explicit_irgen_rule = irgen_rule_candidate;
        explicit_irgen_rule_tag = tag;
        break;
      }
    }
  }
  
  //- rjf: apply all ir-generation steps
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    E_IRGenRule *rule;
    E_Expr *tag;
  };
  Task start_task = {0, explicit_irgen_rule, explicit_irgen_rule_tag};
  Task *first_task = &start_task;
  Task *last_task = first_task;
  for(Task *t = first_task; t != 0; t = t->next)
  {
    // rjf: poison the tag we are about to use, so we don't recursively use it
    if(t->tag != &e_expr_nil)
    {
      U64 hash = e_hash_from_string(5381, str8_struct(&t->tag));
      U64 slot_idx = hash%e_ir_state->used_tag_map->slots_count;
      E_UsedTagNode *n = push_array(arena, E_UsedTagNode, 1);
      n->tag = t->tag;
      DLLPushBack(e_ir_state->used_tag_map->slots[slot_idx].first, e_ir_state->used_tag_map->slots[slot_idx].last, n);
    }
    
    // rjf: do this rule's generation
    ProfScope("irgen rule '%.*s'", str8_varg(t->rule->name))
    {
      result = t->rule->irgen(arena, expr, t->tag);
      if(result.root == &e_irnode_nil && t->rule != &e_irgen_rule__default)
      {
        result = e_irgen_rule__default.irgen(arena, expr, &e_expr_nil);
      }
    }
    
    // rjf: find any auto hooks according to this generation's type
    {
      E_ExprList exprs = e_auto_hook_tag_exprs_from_type_key__cached(result.type_key);
      for(E_ExprNode *n = exprs.first; n != 0; n = n->next)
      {
        for(E_Expr *tag = n->v; tag != &e_expr_nil; tag = tag->next)
        {
          B32 tag_is_poisoned = 0;
          U64 hash = e_hash_from_string(5381, str8_struct(&tag));
          U64 slot_idx = hash%e_ir_state->used_tag_map->slots_count;
          for(E_UsedTagNode *n = e_ir_state->used_tag_map->slots[slot_idx].first; n != 0; n = n->next)
          {
            if(n->tag == tag)
            {
              tag_is_poisoned = 1;
              break;
            }
          }
          if(!tag_is_poisoned)
          {
            E_IRGenRule *rule = e_irgen_rule_from_string(tag->string);
            if(rule != &e_irgen_rule__default)
            {
              Task *task = push_array(scratch.arena, Task, 1);
              SLLQueuePush(first_task, last_task, task);
              task->rule = rule;
              task->tag = tag;
              break;
            }
          }
        }
      }
    }
  }
  
  //- rjf: unpoison the tags we used
  for(Task *t = first_task; t != 0; t = t->next)
  {
    if(t->tag != &e_expr_nil)
    {
      U64 hash = e_hash_from_string(5381, str8_struct(&t->tag));
      U64 slot_idx = hash%e_ir_state->used_tag_map->slots_count;
      for(E_UsedTagNode *n = e_ir_state->used_tag_map->slots[slot_idx].first; n != 0; n = n->next)
      {
        if(n->tag == t->tag)
        {
          DLLRemove(e_ir_state->used_tag_map->slots[slot_idx].first, e_ir_state->used_tag_map->slots[slot_idx].last, n);
          break;
        }
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

//- rjf: irtree -> linear ops/bytecode

internal void
e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_Space *current_space, E_OpList *out)
{
  U32 op = root->op;
  {
    E_Space zero_space = zero_struct;
    if(!MemoryMatchStruct(&root->space, &zero_space) &&
       !MemoryMatchStruct(&root->space, current_space))
    {
      *current_space = root->space;
      e_oplist_push_set_space(arena, out, root->space);
    }
  }
  switch(op)
  {
    case RDI_EvalOp_Stop:
    case RDI_EvalOp_Skip:
    {
      // TODO: error - invalid ir-tree op
    }break;
    
    case E_IRExtKind_Bytecode:
    {
      e_oplist_push_bytecode(arena, out, root->string);
    }break;
    
    case E_IRExtKind_SetSpace:
    {
      E_Space space = {0};
      MemoryCopy(&space, &root->value, sizeof(space));
      e_oplist_push_set_space(arena, out, space);
      for(E_IRNode *child = root->first;
          child != &e_irnode_nil;
          child = child->next)
      {
        e_append_oplist_from_irtree(arena, child, current_space, out);
      }
    }break;
    
    case RDI_EvalOp_Cond:
    {
      // rjf: generate oplists for each child
      E_OpList prt_cond = e_oplist_from_irtree(arena, root->first);
      E_OpList prt_left = e_oplist_from_irtree(arena, root->first->next);
      E_OpList prt_right = e_oplist_from_irtree(arena, root->first->next->next);
      
      // rjf: put together like so:
      //  1. <prt_cond> , Op_Cond (sizeof(2))
      //  2. <prt_right>, Op_Skip (sizeof(3))
      //  3. <ptr_left>
      
      // rjf: modify prt_right in place to create step 2
      e_oplist_push_op(arena, &prt_right, RDI_EvalOp_Skip, e_value_u64(prt_left.encoded_size));
      
      // rjf: merge 1 into out
      e_oplist_concat_in_place(out, &prt_cond);
      e_oplist_push_op(arena, out, RDI_EvalOp_Cond, e_value_u64(prt_right.encoded_size));
      
      // rjf: merge 2 into out
      e_oplist_concat_in_place(out, &prt_right);
      
      // rjf: merge 3 into out
      e_oplist_concat_in_place(out, &prt_left);
    }break;
    
    case RDI_EvalOp_ConstString:
    {
      e_oplist_push_string_literal(arena, out, root->string);
    }break;
    
    default:
    {
      if(op >= RDI_EvalOp_COUNT)
      {
        // TODO: error - invalid ir-tree op
      }
      else
      {
        // rjf: append ops for all children
        U16 ctrlbits = rdi_eval_op_ctrlbits_table[op];
        U64 child_count = RDI_POPN_FROM_CTRLBITS(ctrlbits);
        U64 idx = 0;
        for(E_IRNode *child = root->first;
            child != &e_irnode_nil && idx < child_count;
            child = child->next, idx += 1)
        {
          e_append_oplist_from_irtree(arena, child, current_space, out);
        }
        
        // rjf: emit op to compute this node
        e_oplist_push_op(arena, out, (RDI_EvalOp)root->op, root->value);
      }
    }break;
  }
}

internal E_OpList
e_oplist_from_irtree(Arena *arena, E_IRNode *root)
{
  E_OpList ops = {0};
  E_Space space = e_interpret_ctx->primary_space;
  e_append_oplist_from_irtree(arena, root, &space, &ops);
  return ops;
}

internal String8
e_bytecode_from_oplist(Arena *arena, E_OpList *oplist)
{
  // rjf: allocate buffer
  U64 size = oplist->encoded_size;
  U8 *str = push_array_no_zero(arena, U8, size);
  
  // rjf: iterate loose op nodes; fill buffer
  U8 *ptr = str;
  U8 *opl = str + size;
  for(E_Op *op = oplist->first;
      op != 0;
      op = op->next)
  {
    U32 opcode = op->opcode;
    switch(opcode)
    {
      default:
      {
        // rjf: compute bytecode advance
        U16 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
        U64 extra_byte_count = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
        U8 *next_ptr = ptr + 1 + extra_byte_count;
        Assert(next_ptr <= opl);
        
        // rjf: fill bytecode
        ptr[0] = opcode;
        MemoryCopy(ptr + 1, &op->value.u64, extra_byte_count);
        
        // rjf: advance
        ptr = next_ptr;
      }break;
      
      case RDI_EvalOp_ConstString:
      {
        // rjf: compute bytecode advance
        U8 *next_ptr = ptr + 2 + op->value.u64;
        Assert(next_ptr <= opl);
        
        // rjf: fill
        ptr[0] = opcode;
        ptr[1] = (U8)op->value.u64;
        MemoryCopy(ptr+2, op->string.str, op->value.u64);
        
        // rjf: advance
        ptr = next_ptr;
      }break;
      
      case E_IRExtKind_Bytecode:
      {
        // rjf: compute bytecode advance
        U64 size = op->string.size;
        U8 *next_ptr = ptr + size;
        Assert(next_ptr <= opl);
        
        // rjf: fill bytecode
        MemoryCopy(ptr, op->string.str, size);
        
        // rjf: advance
        ptr = next_ptr;
      }break;
      
      case E_IRExtKind_SetSpace:
      {
        // rjf: compute bytecode advance
        U64 extra_byte_count = sizeof(E_Space);
        U8 *next_ptr = ptr + 1 + extra_byte_count;
        Assert(next_ptr <= opl);
        
        // rjf: fill bytecode
        ptr[0] = opcode;
        MemoryCopy(ptr + 1, &op->value.u128, extra_byte_count);
        
        // rjf: advance
        ptr = next_ptr;
      }break;
    }
  }
  
  // rjf: fill result
  String8 result = {0};
  result.size = size;
  result.str = str;
  return result;
}

//- rjf: leaf-bytecode expression extensions

internal E_Expr *
e_expr_irext_member_access(Arena *arena, E_Expr *lhs, E_IRTreeAndType *lhs_irtree, String8 member_name)
{
  E_Expr *root = e_push_expr(arena, E_ExprKind_MemberAccess, 0);
  E_Expr *lhs_bytecode = e_push_expr(arena, E_ExprKind_LeafBytecode, lhs->location);
  E_OpList lhs_oplist = e_oplist_from_irtree(arena, lhs_irtree->root);
  lhs_bytecode->string = lhs->string;
  lhs_bytecode->space = lhs->space;
  lhs_bytecode->mode = lhs_irtree->mode;
  lhs_bytecode->type_key = lhs_irtree->type_key;
  lhs_bytecode->bytecode = e_bytecode_from_oplist(arena, &lhs_oplist);
  E_Expr *rhs = e_push_expr(arena, E_ExprKind_LeafMember, 0);
  rhs->string = push_str8_copy(arena, member_name);
  e_expr_push_child(root, lhs_bytecode);
  e_expr_push_child(root, rhs);
  return root;
}

////////////////////////////////
//~ rjf: IRified Expression Cache

internal E_IRTreeAndType
e_irtree_and_type_from_expr__cached(E_Expr *expr)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  {
    U64 hash = e_hash_from_string(5381, str8_struct(&expr));
    U64 slot_idx = hash%e_ir_state->irtree_and_type_cache_slots_count;
    E_IRTreeAndTypeCacheNode *node = 0;
    for(E_IRTreeAndTypeCacheNode *n = e_ir_state->irtree_and_type_cache_slots[slot_idx].first; n != 0; n = n->next)
    {
      if(n->expr == expr)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = push_array(e_ir_state->arena, E_IRTreeAndTypeCacheNode, 1);
      SLLQueuePush(e_ir_state->irtree_and_type_cache_slots[slot_idx].first, e_ir_state->irtree_and_type_cache_slots[slot_idx].last, node);
      node->irtree_and_type = e_irtree_and_type_from_expr(e_ir_state->arena, expr);
    }
    if(node != 0)
    {
      result = node->irtree_and_type;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Expression & IR-Tree => Lookup Rule

internal E_LookupRuleTagPair
e_lookup_rule_tag_pair_from_expr_irtree(E_Expr *expr, E_IRTreeAndType *irtree)
{
  E_LookupRuleTagPair result = {&e_lookup_rule__default, &e_expr_nil};
  {
    // rjf: first try explicitly-stored tags
    if(result.rule == &e_lookup_rule__default)
    {
      for(E_Expr *tag = expr->first_tag; tag != &e_expr_nil; tag = tag->next)
      {
        E_LookupRule *candidate = e_lookup_rule_from_string(tag->string);
        if(candidate != &e_lookup_rule__nil)
        {
          result.rule = candidate;
          result.tag = tag;
          break;
        }
      }
    }
    
    // rjf: next try implicit set name -> rule mapping
    if(result.rule == &e_lookup_rule__default)
    {
      E_TypeKind type_kind = e_type_kind_from_key(irtree->type_key);
      if(type_kind == E_TypeKind_Set)
      {
        E_Type *type = e_type_from_key__cached(irtree->type_key);
        String8 name = type->name;
        E_LookupRule *candidate = e_lookup_rule_from_string(name);
        if(candidate != &e_lookup_rule__nil)
        {
          result.rule = candidate;
        }
      }
    }
    
    // rjf: next try auto hook map
    if(result.rule == &e_lookup_rule__default)
    {
      E_ExprList tags = e_auto_hook_tag_exprs_from_type_key__cached(irtree->type_key);
      for(E_ExprNode *n = tags.first; n != 0; n = n->next)
      {
        E_LookupRule *candidate = e_lookup_rule_from_string(n->v->string);
        if(candidate != &e_lookup_rule__nil)
        {
          result.rule = candidate;
          result.tag = n->v;
          break;
        }
      }
    }
  }
  return result;
}
