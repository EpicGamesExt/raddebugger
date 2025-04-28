// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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

//- rjf: rule tag poison checking

internal B32
e_expr_is_poisoned(E_Expr *expr)
{
  B32 tag_is_poisoned = 0;
  U64 hash = e_hash_from_string(5381, str8_struct(&expr));
  U64 slot_idx = hash%e_cache->used_expr_map->slots_count;
  for(E_UsedExprNode *n = e_cache->used_expr_map->slots[slot_idx].first; n != 0; n = n->next)
  {
    if(n->expr == expr)
    {
      tag_is_poisoned = 1;
      break;
    }
  }
  return tag_is_poisoned;
}

internal void
e_expr_poison(E_Expr *expr)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&expr));
  U64 slot_idx = hash%e_cache->used_expr_map->slots_count;
  E_UsedExprNode *n = push_array(e_cache->arena, E_UsedExprNode, 1);
  n->expr = expr;
  DLLPushBack(e_cache->used_expr_map->slots[slot_idx].first, e_cache->used_expr_map->slots[slot_idx].last, n);
}

internal void
e_expr_unpoison(E_Expr *expr)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&expr));
  U64 slot_idx = hash%e_cache->used_expr_map->slots_count;
  for(E_UsedExprNode *n = e_cache->used_expr_map->slots[slot_idx].first; n != 0; n = n->next)
  {
    if(n->expr == expr)
    {
      DLLRemove(e_cache->used_expr_map->slots[slot_idx].first, e_cache->used_expr_map->slots[slot_idx].last, n);
      break;
    }
  }
}

//- rjf: top-level irtree/type extraction

E_TYPE_ACCESS_FUNCTION_DEF(default)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_IRTreeAndType result = {&e_irnode_nil};
  switch(expr->kind)
  {
    default:{}break;
    
    //- rjf: member accessing (. operator)
    case E_ExprKind_MemberAccess:
    {
      // rjf: unpack left/right expressions
      E_Expr *exprl = expr->first;
      E_Expr *exprr = exprl->next;
      E_IRTreeAndType l = *lhs_irtree;
      E_TypeKey l_restype = e_type_key_unwrap(l.type_key, E_TypeUnwrapFlag_AllDecorative & ~E_TypeUnwrapFlag_Enums);
      E_TypeKind l_restype_kind = e_type_kind_from_key(l_restype);
      E_TypeKey check_type_key = l_restype;
      E_TypeKind check_type_kind = l_restype_kind;
      if(l_restype_kind == E_TypeKind_Ptr ||
         l_restype_kind == E_TypeKind_LRef ||
         l_restype_kind == E_TypeKind_RRef)
      {
        check_type_key = e_type_key_unwrap(l.type_key, E_TypeUnwrapFlag_All);
        check_type_kind = e_type_kind_from_key(check_type_key);
      }
      e_msg_list_concat_in_place(&result.msgs, &l.msgs);
      
      // rjf: look up member
      E_Member member = zero_struct;
      B32 r_found = 0;
      E_TypeKey r_type = zero_struct;
      U64 r_value = 0;
      String8 r_query_name = {0};
      B32 r_is_constant_value = 0;
      {
        Temp scratch = scratch_begin(&arena, 1);
        E_Member match = e_type_member_from_key_name__cached(check_type_key, exprr->string);
        member = match;
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
      if(exprr->kind != E_ExprKind_LeafIdentifier)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprl->location, "Expected member name.");
        break;
      }
      else if(!r_found)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprr->location, "Could not find a member named `%S`.", exprr->string);
        break;
      }
      else if(l.root == &e_irnode_nil ||
              e_type_key_match(e_type_key_zero(), check_type_key))
      {
        break;
      }
      else if(check_type_kind != E_TypeKind_Struct &&
              check_type_kind != E_TypeKind_Class &&
              check_type_kind != E_TypeKind_Union &&
              check_type_kind != E_TypeKind_Enum)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprl->location, "Cannot perform member access on this type.");
        break;
      }
      
      // rjf: generate
      {
        // rjf: build tree
        E_IRNode *new_tree = l.root;
        E_TypeKey new_tree_type = r_type;
        E_Mode mode = l.mode;
        if(l_restype_kind == E_TypeKind_Ptr ||
           l_restype_kind == E_TypeKind_LRef ||
           l_restype_kind == E_TypeKind_RRef)
        {
          new_tree = e_irtree_resolve_to_value(arena, l.mode, new_tree, l_restype);
          if(l.mode != E_Mode_Null)
          {
            mode = E_Mode_Offset;
          }
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
        result.root     = new_tree;
        result.type_key = r_type;
        result.mode     = mode;
      }
    }break;
    
    //- rjf: indexing ([] operator)
    case E_ExprKind_ArrayIndex:
    {
      // rjf: unpack left/right expressions
      E_Expr *exprl = expr->first;
      E_Expr *exprr = exprl->next;
      E_IRTreeAndType l = *lhs_irtree;
      E_IRTreeAndType r = e_push_irtree_and_type_from_expr(arena, overridden, 0, 0, 0, exprr);
      e_msg_list_concat_in_place(&result.msgs, &r.msgs);
      E_TypeKey l_restype = e_type_key_unwrap(l.type_key, E_TypeUnwrapFlag_AllDecorative);
      E_TypeKey r_restype = e_type_key_unwrap(r.type_key, E_TypeUnwrapFlag_AllDecorative);
      E_TypeKind l_restype_kind = e_type_kind_from_key(l_restype);
      E_TypeKind r_restype_kind = e_type_kind_from_key(r_restype);
      E_TypeKey direct_type = e_type_key_unwrap(l_restype, E_TypeUnwrapFlag_All);
      U64 direct_type_size = e_type_byte_size_from_key(direct_type);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(r.root->op == 0)
      {
        break;
      }
      else if(l_restype_kind != E_TypeKind_Ptr && l_restype_kind != E_TypeKind_Array)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprl->location, "Cannot index into this type.");
        break;
      }
      else if(!e_type_kind_is_integer(r_restype_kind))
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprr->location, "Cannot index with this type.");
        break;
      }
      else if(l_restype_kind == E_TypeKind_Ptr && direct_type_size == 0)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprr->location, "Cannot index into pointers of zero-sized types.");
        break;
      }
      else if(l_restype_kind == E_TypeKind_Array && direct_type_size == 0)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprr->location, "Cannot index into arrays of zero-sized types.");
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
        }
      }
      
      // rjf: fill
      result.root     = new_tree;
      result.type_key = direct_type;
      result.mode     = mode;
    }break;
  }
  scratch_end(scratch);
  return result;
}

internal E_IRTreeAndType
e_push_irtree_and_type_from_expr(Arena *arena, E_IRTreeAndType *root_parent, B32 disallow_autohooks, B32 disallow_chained_fastpaths, B32 disallow_chained_casts, E_Expr *root_expr)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  E_TypeKeyList inherited_lenses = {0};
  E_IRTreeAndType result = {&e_irnode_nil};
  
  //////////////////////////////
  //- rjf: apply all ir-generation steps
  //
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    E_Expr *expr;
    E_IRTreeAndType *overridden;
  };
  Task start_task = {0, root_expr, 0};
  Task *first_task = &start_task;
  Task *last_task = first_task;
  for(Task *t = first_task; t != 0; t = t->next)
  {
    E_Expr *expr = t->expr;
    E_IRTreeAndType *parent = t->overridden ? t->overridden : root_parent;
    
    //- rjf: poison the expression we are about to use, so we don't recursively use it
    e_expr_poison(expr);
    
    //- rjf: do expr -> irtree generation for this expression
    if(expr->kind == E_ExprKind_Ref)
    {
      expr = expr->ref;
    }
    E_ExprKind kind = expr->kind;
    switch(kind)
    {
      default:{}break;
      
      //- rjf: member accesses & array indexing expressions
      case E_ExprKind_MemberAccess:
      case E_ExprKind_ArrayIndex:
      {
        // rjf: unpack left-hand-side
        E_Expr *lhs = expr->first;
        E_IRTreeAndType lhs_irtree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, lhs);
        e_msg_list_concat_in_place(&result.msgs, &lhs_irtree.msgs);
        
        // rjf: try all IR trees in chain
        for(E_IRTreeAndType *lhs_irtree_try = &lhs_irtree; lhs_irtree_try != 0; lhs_irtree_try = lhs_irtree_try->prev)
        {
          // rjf: gather inherited lenses from the left-hand-side
          {
            E_TypeKey k = lhs_irtree_try->type_key;
            E_TypeKind kind = e_type_kind_from_key(k);
            for(;kind == E_TypeKind_Lens;)
            {
              E_Type *lens_type = e_type_from_key__cached(k);
              if((lens_type->flags & E_TypeFlag_InheritedByMembers && expr->kind == E_ExprKind_MemberAccess) ||
                 (lens_type->flags & E_TypeFlag_InheritedByElements && expr->kind == E_ExprKind_ArrayIndex))
              {
                e_type_key_list_push_front(scratch.arena, &inherited_lenses, k);
              }
              k = e_type_key_direct(k);
              kind = e_type_kind_from_key(k);
            }
          }
          
          // rjf: pick access hook based on type
          E_Type *lhs_type = e_type_from_key__cached(lhs_irtree_try->type_key);
          E_TypeAccessFunctionType *lhs_access = lhs_type->access;
          for(E_Type *lens_type = lhs_type;
              lens_type->kind == E_TypeKind_Lens || lens_type->kind == E_TypeKind_Set;
              lens_type = e_type_from_key__cached(lens_type->direct_type_key))
          {
            if(lens_type->access != 0)
            {
              lhs_access = lens_type->access;
              break;
            }
          }
          if(lhs_access == 0)
          {
            lhs_access = E_TYPE_ACCESS_FUNCTION_NAME(default);
          }
          
          // rjf: call into hook to do access
          result = lhs_access(arena, parent, expr, lhs_irtree_try);
          
          // rjf: end chain if we found a result
          if(result.root != &e_irnode_nil)
          {
            break;
          }
        }
      }break;
      
      //- rjf: dereference
      case E_ExprKind_Deref:
      {
        // rjf: unpack operand
        E_Expr *r_expr = expr->first;
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        E_TypeKey r_type = e_type_key_unwrap(r_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
        E_TypeKey r_type_direct = e_type_key_unwrap(r_type, E_TypeUnwrapFlag_All);
        U64 r_type_direct_size = e_type_byte_size_from_key(r_type_direct);
        
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
          result.mode     = E_Mode_Null;
          if(r_tree.mode != E_Mode_Null)
          {
            result.mode = E_Mode_Offset;
          }
        }
      }break;
      
      //- rjf: address-of
      case E_ExprKind_Address:
      {
        // rjf: unpack operand
        E_Expr *r_expr = expr->first;
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        E_TypeKey r_type = r_tree.type_key;
        E_TypeKey r_type_unwrapped = e_type_key_unwrap(r_type, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKind r_type_unwrapped_kind = e_type_kind_from_key(r_type_unwrapped);
        
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
        result.type_key = e_type_key_cons_ptr(e_base_ctx->primary_module->arch, r_type_unwrapped, 1, 0);
        result.mode     = E_Mode_Value;
      }break;
      
      //- rjf: cast
      case E_ExprKind_Cast:
      {
        // rjf: unpack operands
        E_Expr *cast_type_expr = expr->first;
        E_Expr *casted_expr = cast_type_expr->next;
        E_IRTreeAndType cast_irtree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, cast_type_expr);
        e_msg_list_concat_in_place(&result.msgs, &cast_irtree.msgs);
        E_TypeKey cast_type = cast_irtree.type_key;
        E_TypeKind cast_type_kind = e_type_kind_from_key(cast_type);
        U64 cast_type_byte_size = e_type_byte_size_from_key(cast_type);
        E_IRTreeAndType casted_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, casted_expr);
        e_msg_list_concat_in_place(&result.msgs, &casted_tree.msgs);
        E_TypeKey casted_type = e_type_key_unwrap(casted_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
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
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        r_type = r_tree.type_key;
        
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
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
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
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        E_TypeKey r_type = e_type_key_unwrap(r_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
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
        result = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, expr->first);
      }break;
      case E_ExprKind_Neg:
      case E_ExprKind_BitNot:
      {
        // rjf: unpack operand
        E_Expr *r_expr = expr->first;
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        E_TypeKey r_type = e_type_key_unwrap(r_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
        RDI_EvalTypeGroup r_type_group = e_type_group_from_kind(r_type_kind);
        E_TypeKey r_type_promoted = e_type_key_promote(r_type);
        RDI_EvalOp op = e_opcode_from_expr_kind(kind);
        
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
      case E_ExprKind_LogNot:
      {
        // rjf: unpack operand
        E_Expr *r_expr = expr->first;
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        E_TypeKey r_type = e_type_key_unwrap(r_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
        RDI_EvalTypeGroup r_type_group = e_type_group_from_kind(r_type_kind);
        E_TypeKey r_type_promoted = e_type_key_basic(E_TypeKind_Bool);
        RDI_EvalOp op = e_opcode_from_expr_kind(kind);
        
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
        E_IRTreeAndType l_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, l_expr);
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &l_tree.msgs);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        E_TypeKey l_type = e_type_key_unwrap(l_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKey r_type = e_type_key_unwrap(r_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
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
            E_TypeKey l_type_direct = e_type_key_unwrap(l_type, E_TypeUnwrapFlag_All);
            E_TypeKey r_type_direct = e_type_key_unwrap(r_type, E_TypeUnwrapFlag_All);
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
            E_TypeKey direct_type = e_type_key_unwrap(ptr_tree->type_key, E_TypeUnwrapFlag_All);
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
                ptr_type = e_type_key_cons_ptr(e_base_ctx->primary_module->arch, direct_type, 1, 0);
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
            E_TypeKey direct_type = e_type_key_unwrap(l_type, E_TypeUnwrapFlag_All);
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
        E_IRTreeAndType c_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, c_expr);
        E_IRTreeAndType l_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, l_expr);
        E_IRTreeAndType r_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, r_expr);
        e_msg_list_concat_in_place(&result.msgs, &c_tree.msgs);
        e_msg_list_concat_in_place(&result.msgs, &l_tree.msgs);
        e_msg_list_concat_in_place(&result.msgs, &r_tree.msgs);
        E_TypeKey c_type = e_type_key_unwrap(c_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKey l_type = e_type_key_unwrap(l_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
        E_TypeKey r_type = e_type_key_unwrap(r_tree.type_key, E_TypeUnwrapFlag_AllDecorative);
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
      
      //- rjf: call
      case E_ExprKind_Call:
      {
        B32 strip_lenses = 0;
        E_Expr *lhs = expr->first;
        E_IRTreeAndType lhs_irtree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, 1, 1, lhs);
        e_msg_list_concat_in_place(&result.msgs, &lhs_irtree.msgs);
        E_TypeKey lhs_type_key = lhs_irtree.type_key;
        E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
        
        // rjf: calling a type? -> treat as a cast of that type
        if(lhs_irtree.mode == E_Mode_Null && lhs_type != &e_type_nil)
        {
          E_IRTreeAndType casted_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, 1, disallow_chained_casts, expr->first->next);
          e_msg_list_concat_in_place(&result.msgs, &casted_tree.msgs);
          E_TypeKey cast_type = lhs_irtree.type_key;
          E_TypeKind cast_type_kind = e_type_kind_from_key(cast_type);
          U64 cast_type_byte_size = e_type_byte_size_from_key(cast_type);
          E_TypeKey casted_type = casted_tree.type_key;
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
          
          // rjf: generate casted result
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
        }
        
        // rjf: calling an unresolved leaf-identifier member access, and we can determine
        // that that identifer maps to a type? -> generate a call expression with the
        // left-hand-side of the dot operator as the first argument. this is a fast path
        // which prevents paren nesting in simple cases, to easily chain multiple
        // calls - for example, bin(2).digits(4)
        else if(lhs_type == &e_type_nil &&
                lhs->kind == E_ExprKind_MemberAccess)
        {
          E_Expr *callee = lhs->first->next;
          E_Expr *first_arg = e_expr_ref(arena, lhs->first);
          E_Expr *call = e_push_expr(arena, E_ExprKind_Call, 0);
          e_expr_push_child(call, callee);
          e_expr_push_child(call, first_arg);
          for(E_Expr *arg = lhs->next; arg != &e_expr_nil; arg = arg->next)
          {
            e_expr_push_child(call, e_expr_copy(arena, arg));
          }
          if(str8_match(callee->string, str8_lit("raw"), 0))
          {
            strip_lenses = 1;
            disallow_autohooks = 1;
          }
          result = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, call);
          E_MsgList new_msgs = {0};
          e_msg_list_concat_in_place(&new_msgs, &lhs_irtree.msgs);
          e_msg_list_concat_in_place(&new_msgs, &result.msgs);
          result.msgs = new_msgs;
        }
        
        // rjf: calling a lens? -> generate IR for the first argument; if enabled, wrap
        // the type in a lens type, which preserves the name & arguments of the lens call
        // expression
        else if(lhs_type->kind == E_TypeKind_LensSpec)
        {
          // rjf: is "raw"? -> disable hooks
          if(str8_match(lhs_type->name, str8_lit("raw"), 0))
          {
            strip_lenses = 1;
            disallow_autohooks = 1;
          }
          
          // rjf: generate result via first argument to lens
          result = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, 1, disallow_chained_casts, lhs->next);
          
          // rjf: if not raw, wrap resultant type with lens type
          if(!strip_lenses)
          {
            Temp scratch = scratch_begin(&arena, 1);
            
            // rjf: count extra arguments
            U64 arg_count = 0;
            for(E_Expr *arg = lhs->next->next; arg != &e_expr_nil; arg = arg->next)
            {
              arg_count += 1;
            }
            
            // rjf: flatten extra arguments
            E_Expr **args = push_array(scratch.arena, E_Expr *, arg_count);
            {
              U64 idx = 0;
              for(E_Expr *arg = lhs->next->next; arg != &e_expr_nil; arg = arg->next, idx += 1)
              {
                args[idx] = arg;
              }
            }
            
            // rjf: patch resultant type with a lens w/ args, pointing to the original type
            {
              result.type_key = e_type_key_cons(.kind       = E_TypeKind_Lens,
                                                .flags      = lhs_type->flags,
                                                .count      = arg_count,
                                                .args       = args,
                                                .direct_key = result.type_key,
                                                .name       = lhs_type->name,
                                                .irext      = lhs_type->irext,
                                                .access     = lhs_type->access,
                                                .expand     = lhs_type->expand);
            }
            scratch_end(scratch);
          }
        }
        
        // rjf: calling any other type? -> not valid
        else
        {
          e_msgf(arena, &result.msgs, E_MsgKind_InterpretationError, expr->location, "Calling this type is not supported.");
        }
        
        // rjf: strip overrides and lenses if needed
        if(strip_lenses)
        {
          if(t->overridden)
          {
            E_MsgList existing_msgs = result.msgs;
            for(E_IRTreeAndType *prev = t->overridden; prev != 0; prev = prev->prev)
            {
              result = *prev;
            }
            E_MsgList overridden_msgs = e_msg_list_copy(arena, &result.msgs);
            result.msgs = existing_msgs;
            e_msg_list_concat_in_place(&result.msgs, &overridden_msgs);
          }
          result.type_key = e_type_key_unwrap(result.type_key, E_TypeUnwrapFlag_Lenses|E_TypeUnwrapFlag_Meta);
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
      
      //- rjf: leaf string literal
      case E_ExprKind_LeafStringLiteral:
      {
        String8 string = expr->string;
        E_TypeKey type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_UChar8), string.size, 0);
        E_IRNode *new_tree = e_irtree_string_literal(arena, string);
        result.root     = new_tree;
        result.type_key = type_key;
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
      case E_ExprKind_LeafIdentifier:
      {
        Temp scratch = scratch_begin(&arena, 1);
        String8 qualifier = expr->qualifier;
        String8 string = expr->string;
        String8 string__redirected = string;
        B32 string_mapped = 0;
        E_TypeKey mapped_type_key = zero_struct;
        E_Module *mapped_location_block_module = &e_module_nil;
        RDI_LocationBlock *mapped_location_block = 0;
        E_Mode mapped_bytecode_mode = E_Mode_Offset;
        E_Space mapped_bytecode_space = zero_struct;
        String8 mapped_bytecode = {0};
        
        //- rjf: form namespaceified fallback versions of this lookup string
        String8List namespaceified_strings = {0};
        {
          E_Module *module = e_base_ctx->primary_module;
          RDI_Parsed *rdi = module->rdi;
          RDI_Procedure *procedure = e_cache->thread_ip_procedure;
          U64 name_size = 0;
          U8 *name_ptr = rdi_string_from_idx(rdi, procedure->name_string_idx, &name_size);
          String8 containing_procedure_name = str8(name_ptr, name_size);
          U64 last_past_scope_resolution_pos = 0;
          for(;;)
          {
            U64 past_next_dbl_colon_pos = str8_find_needle(containing_procedure_name, last_past_scope_resolution_pos, str8_lit("::"), 0)+2;
            U64 past_next_dot_pos = str8_find_needle(containing_procedure_name, last_past_scope_resolution_pos, str8_lit("."), 0)+1;
            U64 past_next_scope_resolution_pos = Min(past_next_dbl_colon_pos, past_next_dot_pos);
            if(past_next_scope_resolution_pos >= containing_procedure_name.size)
            {
              break;
            }
            String8 new_namespace_prefix_possibility = str8_prefix(containing_procedure_name, past_next_scope_resolution_pos);
            String8 namespaceified_string = push_str8f(scratch.arena, "%S%S", new_namespace_prefix_possibility, string);
            str8_list_push_front(scratch.arena, &namespaceified_strings, namespaceified_string);
            last_past_scope_resolution_pos = past_next_scope_resolution_pos;
          }
        }
        
        //- rjf: try to map name as parent expression signifier ('$')
        if(!string_mapped && str8_match(string, str8_lit("$"), 0) && parent != 0 && (parent->root != &e_irnode_nil || parent->msgs.first != 0))
        {
          E_IRTreeAndType *parent_irtree = parent;
          if(disallow_autohooks)
          {
            for(E_IRTreeAndType *prev = parent_irtree->prev; prev != 0; prev = prev->prev)
            {
              parent_irtree = prev;
            }
          }
          E_OpList oplist = e_oplist_from_irtree(arena, parent_irtree->root);
          string_mapped = 1;
          mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
          mapped_bytecode_mode = parent_irtree->mode;
          mapped_type_key = parent_irtree->type_key;
          disallow_autohooks = 1;
          E_MsgList msgs = e_msg_list_copy(arena, &parent_irtree->msgs);
          e_msg_list_concat_in_place(&result.msgs, &msgs);
        }
        
        //- rjf: try to map name as implicit access of overridden expression ('$.member_name', where the $. prefix is omitted)
        if(!string_mapped && parent != 0 && parent->root != &e_irnode_nil)
        {
          for(E_IRTreeAndType *prev = parent; prev != 0; prev = prev->prev)
          {
            E_Expr *access = e_expr_irext_member_access(scratch.arena, &e_expr_nil, prev, string);
            E_IRTreeAndType access_irtree = e_push_irtree_and_type_from_expr(scratch.arena, prev, disallow_autohooks, 1, 1, access);
            if(access_irtree.root != &e_irnode_nil)
            {
              string_mapped = 1;
              E_OpList oplist = e_oplist_from_irtree(scratch.arena, access_irtree.root);
              mapped_type_key = access_irtree.type_key;
              mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
              mapped_bytecode_mode = access_irtree.mode;
              e_msg_list_concat_in_place(&result.msgs, &access_irtree.msgs);
              break;
            }
          }
        }
        
        //- rjf: try to map name as member - if found, string__redirected := "this", and turn
        // on later implicit-member-lookup generation
        B32 string_is_implicit_member_name = 0;
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("member"), 0)))
        {
          E_Module *module = e_base_ctx->primary_module;
          U32 module_idx = (U32)(module - e_base_ctx->modules);
          RDI_Parsed *rdi = module->rdi;
          RDI_Procedure *procedure = e_cache->thread_ip_procedure;
          RDI_UDT *udt = rdi_container_udt_from_procedure(rdi, procedure);
          RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, udt->self_type_idx);
          E_TypeKey container_type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), udt->self_type_idx, module_idx);
          E_Member member = e_type_member_from_key_name__cached(container_type_key, string);
          if(member.kind != E_MemberKind_Null)
          {
            string_is_implicit_member_name = 1;
            string__redirected = str8_lit("this");
          }
        }
        
        //- rjf: try locals
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("local"), 0)))
        {
          E_Module *module = e_base_ctx->primary_module;
          U32 module_idx = (U32)(module - e_base_ctx->modules);
          RDI_Parsed *rdi = module->rdi;
          U64 local_num = e_num_from_string(e_ir_ctx->locals_map, string__redirected);
          if(local_num != 0)
          {
            RDI_Local *local = rdi_element_from_name_idx(rdi, Locals, local_num-1);
            
            // rjf: extract local's type key
            RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, local->type_idx);
            mapped_type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), local->type_idx, module_idx);
            
            // rjf: extract local's location block
            U64 ip_voff = e_base_ctx->thread_ip_voff;
            for(U32 loc_block_idx = local->location_first;
                loc_block_idx < local->location_opl;
                loc_block_idx += 1)
            {
              RDI_LocationBlock *block = rdi_element_from_name_idx(rdi, LocationBlocks, loc_block_idx);
              if(block->scope_off_first <= ip_voff && ip_voff < block->scope_off_opl)
              {
                mapped_location_block_module = module;
                mapped_location_block = block;
              }
            }
          }
        }
        
        //- rjf: mapped to location block -> extract or produce bytecode for this mapping
        if(mapped_location_block != 0)
        {
          E_Module *module = mapped_location_block_module;
          E_Space space = module->space;
          Arch arch = module->arch;
          RDI_Parsed *rdi = module->rdi;
          RDI_LocationBlock *block = mapped_location_block;
          U64 all_location_data_size = 0;
          U8 *all_location_data = rdi_table_from_name(rdi, LocationData, &all_location_data_size);
          if(block->location_data_off + sizeof(RDI_LocationKind) <= all_location_data_size)
          {
            RDI_LocationKind loc_kind = *((RDI_LocationKind *)(all_location_data + block->location_data_off));
            switch(loc_kind)
            {
              default:{}break;
              case RDI_LocationKind_ValBytecodeStream: {mapped_bytecode_mode = E_Mode_Value;}goto bytecode_stream;
              case RDI_LocationKind_AddrBytecodeStream:{mapped_bytecode_mode = E_Mode_Offset;}goto bytecode_stream;
              bytecode_stream:;
              {
                string_mapped = 1;
                U64 bytecode_size = 0;
                U64 off_first = block->location_data_off + sizeof(RDI_LocationKind);
                U64 off_opl = all_location_data_size;
                for(U64 off = off_first, next_off = off_opl;
                    off < all_location_data_size;
                    off = next_off)
                {
                  next_off = off_opl;
                  U8 op = all_location_data[off];
                  if(op == 0)
                  {
                    break;
                  }
                  U16 ctrlbits = rdi_eval_op_ctrlbits_table[op];
                  U32 p_size = RDI_DECODEN_FROM_CTRLBITS(ctrlbits);
                  bytecode_size += (1 + p_size);
                  next_off = (off + 1 + p_size);
                }
                mapped_bytecode = str8(all_location_data + off_first, bytecode_size);
              }break;
              case RDI_LocationKind_AddrRegPlusU16:
              if(block->location_data_off + sizeof(RDI_LocationRegPlusU16) <= all_location_data_size)
              {
                string_mapped = 1;
                RDI_LocationRegPlusU16 loc = *(RDI_LocationRegPlusU16 *)(all_location_data + block->location_data_off);
                E_OpList oplist = {0};
                U64 byte_size = bit_size_from_arch(arch)/8;
                U64 regread_param = RDI_EncodeRegReadParam(loc.reg_code, byte_size, 0);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_RegRead, e_value_u64(regread_param));
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU16, e_value_u64(loc.offset));
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_Add, e_value_u64(0));
                mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
                mapped_bytecode_mode = E_Mode_Offset;
                mapped_bytecode_space = space;
              }break;
              case RDI_LocationKind_AddrAddrRegPlusU16:
              {
                string_mapped = 1;
                RDI_LocationRegPlusU16 loc = *(RDI_LocationRegPlusU16 *)(all_location_data + block->location_data_off);
                E_OpList oplist = {0};
                U64 byte_size = bit_size_from_arch(arch)/8;
                U64 regread_param = RDI_EncodeRegReadParam(loc.reg_code, byte_size, 0);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_RegRead, e_value_u64(regread_param));
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU16, e_value_u64(loc.offset));
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_Add, e_value_u64(0));
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_MemRead, e_value_u64(bit_size_from_arch(arch)/8));
                mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
                mapped_bytecode_mode = E_Mode_Offset;
                mapped_bytecode_space = space;
              }break;
              case RDI_LocationKind_ValReg:
              if(block->location_data_off + sizeof(RDI_LocationReg) <= all_location_data_size)
              {
                string_mapped = 1;
                RDI_LocationReg loc = *(RDI_LocationReg *)(all_location_data + block->location_data_off);
                
                REGS_RegCode regs_reg_code = regs_reg_code_from_arch_rdi_code(arch, loc.reg_code);
                REGS_Rng reg_rng = regs_reg_code_rng_table_from_arch(arch)[regs_reg_code];
                E_OpList oplist = {0};
                U64 byte_size = (U64)reg_rng.byte_size;
                U64 byte_pos = 0;
                U64 regread_param = RDI_EncodeRegReadParam(loc.reg_code, byte_size, byte_pos);
                e_oplist_push_op(arena, &oplist, RDI_EvalOp_RegRead, e_value_u64(regread_param));
                mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
                mapped_bytecode_mode = E_Mode_Value;
                mapped_bytecode_space = space;
              }break;
            }
          }
        }
        
        //- rjf: try globals
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("global"), 0)))
        {
          for(U64 module_idx = 0; module_idx < e_base_ctx->modules_count; module_idx += 1)
          {
            E_Module *module = &e_base_ctx->modules[module_idx];
            RDI_Parsed *rdi = module->rdi;
            RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_GlobalVariables);
            RDI_ParsedNameMap parsed_name_map = {0};
            rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
            RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, string.str, string.size);
            U32 matches_count = 0;
            U32 *matches = rdi_matches_from_map_node(rdi, node, &matches_count);
            for(String8Node *n = namespaceified_strings.first;
                n != 0 && matches_count == 0;
                n = n->next)
            {
              node = rdi_name_map_lookup(rdi, &parsed_name_map, n->string.str, n->string.size);
              matches_count = 0;
              matches = rdi_matches_from_map_node(rdi, node, &matches_count);
            }
            if(matches_count != 0)
            {
              U32 match_idx = matches[matches_count-1];
              RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, match_idx);
              U32 type_idx = global_var->type_idx;
              RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, type_idx);
              E_OpList oplist = {0};
              e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU64, e_value_u64(module->vaddr_range.min + global_var->voff));
              string_mapped = 1;
              mapped_type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)module_idx);
              mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
              mapped_bytecode_mode = E_Mode_Offset;
              mapped_bytecode_space = module->space;
              break;
            }
          }
        }
        
        //- rjf: try thread-locals
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("thread_local"), 0)))
        {
          for(U64 module_idx = 0; module_idx < e_base_ctx->modules_count; module_idx += 1)
          {
            E_Module *module = &e_base_ctx->modules[module_idx];
            RDI_Parsed *rdi = module->rdi;
            RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_ThreadVariables);
            RDI_ParsedNameMap parsed_name_map = {0};
            rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
            RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, string.str, string.size);
            U32 matches_count = 0;
            U32 *matches = rdi_matches_from_map_node(rdi, node, &matches_count);
            for(String8Node *n = namespaceified_strings.first;
                n != 0 && matches_count == 0;
                n = n->next)
            {
              node = rdi_name_map_lookup(rdi, &parsed_name_map, n->string.str, n->string.size);
              matches_count = 0;
              matches = rdi_matches_from_map_node(rdi, node, &matches_count);
            }
            if(matches_count != 0)
            {
              U32 match_idx = matches[matches_count-1];
              RDI_ThreadVariable *thread_var = rdi_element_from_name_idx(rdi, ThreadVariables, match_idx);
              U32 type_idx = thread_var->type_idx;
              RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, type_idx);
              E_OpList oplist = {0};
              e_oplist_push_op(arena, &oplist, RDI_EvalOp_TLSOff, e_value_u64(thread_var->tls_off));
              string_mapped = 1;
              mapped_type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)module_idx);
              mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
              mapped_bytecode_mode = E_Mode_Offset;
              mapped_bytecode_space = module->space;
              break;
            }
          }
        }
        
        //- rjf: try procedures
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("procedure"), 0)))
        {
          for(U64 module_idx = 0; module_idx < e_base_ctx->modules_count; module_idx += 1)
          {
            E_Module *module = &e_base_ctx->modules[module_idx];
            RDI_Parsed *rdi = module->rdi;
            RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Procedures);
            RDI_ParsedNameMap parsed_name_map = {0};
            rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
            RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, string.str, string.size);
            U32 matches_count = 0;
            U32 *matches = rdi_matches_from_map_node(rdi, node, &matches_count);
            for(String8Node *n = namespaceified_strings.first;
                n != 0 && matches_count == 0;
                n = n->next)
            {
              node = rdi_name_map_lookup(rdi, &parsed_name_map, n->string.str, n->string.size);
              matches_count = 0;
              matches = rdi_matches_from_map_node(rdi, node, &matches_count);
            }
            if(matches_count != 0)
            {
              U32 match_idx = matches[matches_count-1];
              RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, match_idx);
              RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, procedure->root_scope_idx);
              U64 voff = *rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_first);
              U32 type_idx = procedure->type_idx;
              RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, type_idx);
              E_OpList oplist = {0};
              e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU64, e_value_u64(module->vaddr_range.min + voff));
              string_mapped = 1;
              mapped_type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)module_idx);
              mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
              mapped_bytecode_mode = E_Mode_Value;
              mapped_bytecode_space = module->space;
              break;
            }
          }
        }
        
        //- rjf: try types
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("type"), 0)))
        {
          mapped_type_key = e_leaf_type_from_name(string);
          if(!e_type_key_match(e_type_key_zero(), mapped_type_key))
          {
            string_mapped = 1;
          }
        }
        
        //- rjf: try registers
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("reg"), 0)))
        {
          U64 reg_num = e_num_from_string(e_ir_ctx->regs_map, string);
          if(reg_num != 0)
          {
            string_mapped = 1;
            REGS_Rng reg_rng = regs_reg_code_rng_table_from_arch(e_base_ctx->primary_module->arch)[reg_num];
            E_OpList oplist = {0};
            e_oplist_push_uconst(arena, &oplist, reg_rng.byte_off);
            mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
            mapped_bytecode_mode = E_Mode_Offset;
            mapped_bytecode_space = e_base_ctx->thread_reg_space;
            mapped_type_key = e_type_key_reg(e_base_ctx->primary_module->arch, reg_num);
          }
        }
        
        //- rjf: try register aliases
        if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("reg"), 0)))
        {
          U64 alias_num = e_num_from_string(e_ir_ctx->reg_alias_map, string);
          if(alias_num != 0)
          {
            string_mapped = 1;
            REGS_Slice alias_slice = regs_alias_code_slice_table_from_arch(e_base_ctx->primary_module->arch)[alias_num];
            REGS_Rng alias_reg_rng = regs_reg_code_rng_table_from_arch(e_base_ctx->primary_module->arch)[alias_slice.code];
            E_OpList oplist = {0};
            e_oplist_push_uconst(arena, &oplist, alias_reg_rng.byte_off + alias_slice.byte_off);
            mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
            mapped_bytecode_mode = E_Mode_Offset;
            mapped_bytecode_space = e_base_ctx->thread_reg_space;
            mapped_type_key = e_type_key_reg_alias(e_base_ctx->primary_module->arch, alias_num);
          }
        }
        
        //- rjf: try basic constants
        if(!string_mapped && str8_match(string, str8_lit("true"), 0))
        {
          string_mapped = 1;
          E_OpList oplist = {0};
          e_oplist_push_uconst(arena, &oplist, 1);
          mapped_type_key = e_type_key_basic(E_TypeKind_Bool);
          mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
          mapped_bytecode_mode = E_Mode_Value;
        }
        if(!string_mapped && str8_match(string, str8_lit("false"), 0))
        {
          string_mapped = 1;
          E_OpList oplist = {0};
          e_oplist_push_uconst(arena, &oplist, 0);
          mapped_type_key = e_type_key_basic(E_TypeKind_Bool);
          mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
          mapped_bytecode_mode = E_Mode_Value;
        }
        
        //- rjf: generate IR trees for bytecode
        B32 generated = 0;
        if(!generated && mapped_bytecode.size != 0)
        {
          generated = 1;
          E_IRNode *root = e_irtree_bytecode_no_copy(arena, mapped_bytecode);
          root->space = mapped_bytecode_space;
          result.root = root;
          result.type_key = mapped_type_key;
          result.mode = mapped_bytecode_mode;
        }
        
        //- rjf: generate nil-IR trees w/ type for types
        if(!generated && !e_type_key_match(e_type_key_zero(), mapped_type_key))
        {
          generated = 1;
          result.root = e_irtree_const_u(arena, 0);
          result.type_key = mapped_type_key;
          result.mode = E_Mode_Null;
        }
        
        //- rjf: generate IR trees for macros
        if(!generated)
        {
          E_Expr *macro_expr = e_string2expr_map_lookup(e_ir_ctx->macro_map, string);
          if(macro_expr != &e_expr_nil)
          {
            generated = 1;
            e_string2expr_map_inc_poison(e_ir_ctx->macro_map, string);
            result = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, macro_expr);
            e_string2expr_map_dec_poison(e_ir_ctx->macro_map, string);
          }
        }
        
        //- rjf: extend generation with member access, if original string was an
        // implicit member access
        if(generated && string_is_implicit_member_name)
        {
          // TODO(rjf): @cfg
#if 0
          E_LookupRule *lookup_rule = &e_lookup_rule__default;
          E_LookupInfo lookup_info = lookup_rule->info(arena, &result, &e_expr_nil, str8_zero());
          E_LookupAccess lookup_access = lookup_rule->access(arena, E_ExprKind_MemberAccess, lhs, rhs, &e_expr_nil, lookup_info.user_data);
          result = lookup_access.irtree_and_type;
#endif
        }
        
        //- rjf: error on failure-to-generate
        if(!generated && !str8_match(string, str8_lit("$"), 0))
        {
          e_msgf(arena, &result.msgs, E_MsgKind_ResolutionFailure, expr->location, "`%S` could not be found.", string);
        }
        
        scratch_end(scratch);
      }break;
      
      //- rjf: leaf offsets
      case E_ExprKind_LeafOffset:
      {
        E_IRNode *new_tree = e_push_irnode(arena, RDI_EvalOp_ConstU64);
        new_tree->value = expr->value;
        new_tree->space = expr->space;
        result.root     = new_tree;
        result.type_key = expr->type_key;
        result.mode     = E_Mode_Offset;
      }break;
      
      //- rjf: leaf values
      case E_ExprKind_LeafValue:
      {
        E_IRNode *new_tree = e_push_irnode(arena, RDI_EvalOp_ConstU128);
        new_tree->value = expr->value;
        new_tree->space = expr->space;
        result.root     = new_tree;
        result.type_key = expr->type_key;
        result.mode     = E_Mode_Value;
      }break;
      
      //- rjf: leaf file paths
      case E_ExprKind_LeafFilePath:
      {
        Temp scratch = scratch_begin(&arena, 1);
        String8 file_path = expr->string;
        FileProperties props = os_properties_from_file_path(file_path);
        if(!str8_match(expr->qualifier, str8_lit("folder"), 0) && !(props.flags & FilePropertyFlag_IsFolder) && file_path.size != 0)
        {
          E_Space space = e_space_make(E_SpaceKind_FileSystem);
          result.root     = e_irtree_set_space(arena, space, e_irtree_const_u(arena, e_id_from_string(file_path)));
          result.type_key = e_cache->file_type_key;
          result.mode     = E_Mode_Value;
        }
        else
        {
          String8 folder_path = str8_chop_last_slash(file_path);
          props = os_properties_from_file_path(folder_path);
          if(props.flags & FilePropertyFlag_IsFolder || folder_path.size == 0 || str8_match(folder_path, str8_lit("/"), StringMatchFlag_SlashInsensitive))
          {
            E_Space space = e_space_make(E_SpaceKind_FileSystem);
            result.root     = e_irtree_set_space(arena, space, e_irtree_const_u(arena, e_id_from_string(folder_path)));
            result.type_key = e_cache->folder_type_key;
            result.mode     = E_Mode_Value;
          }
        }
        scratch_end(scratch);
      }break;
      
      //- rjf: types
      case E_ExprKind_TypeIdent:
      {
        result.root = e_irtree_const_u(arena, 0);
        result.root->space = expr->space;
        result.type_key = expr->type_key;
        result.mode = E_Mode_Null;
      }break;
      case E_ExprKind_Unsigned:
      {
        E_IRTreeAndType direct_irtree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, expr->first);
        result = direct_irtree;
        E_TypeKey direct_type_key = result.type_key;
        E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
        if(e_type_kind_is_signed(direct_type_kind))
        {
          E_TypeKind new_kind = direct_type_kind;
          switch(direct_type_kind)
          {
            default:{}break;
            case E_TypeKind_Char8: {new_kind = E_TypeKind_UChar8;}break;
            case E_TypeKind_Char16:{new_kind = E_TypeKind_UChar16;}break;
            case E_TypeKind_Char32:{new_kind = E_TypeKind_UChar32;}break;
            case E_TypeKind_S8:{new_kind = E_TypeKind_U8;}break;
            case E_TypeKind_S16:{new_kind = E_TypeKind_U16;}break;
            case E_TypeKind_S32:{new_kind = E_TypeKind_U32;}break;
            case E_TypeKind_S64:{new_kind = E_TypeKind_U64;}break;
            case E_TypeKind_S128:{new_kind = E_TypeKind_U128;}break;
            case E_TypeKind_S256:{new_kind = E_TypeKind_U256;}break;
            case E_TypeKind_S512:{new_kind = E_TypeKind_U512;}break;
          }
          result.type_key = e_type_key_basic(new_kind);
        }
        else
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->first->location, "Cannot apply an `unsigned` modifier to this type.");
        }
      }break;
      case E_ExprKind_Ptr:
      {
        E_IRTreeAndType ptee_irtree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, expr->first);
        result = ptee_irtree;
        result.type_key = e_type_key_cons_ptr(e_base_ctx->primary_module->arch, result.type_key, 1, 0);
      }break;
      case E_ExprKind_Array:
      {
        E_IRTreeAndType element_irtree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, expr->first);
        result = element_irtree;
        result.type_key = e_type_key_cons_array(result.type_key, expr->value.u64, 0);
      }break;
      case E_ExprKind_Func:
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Function type expressions are currently not supported.");
      }break;
      
      //- rjf: definitions
      case E_ExprKind_Define:
      {
        E_Expr *lhs = expr->first;
        E_Expr *rhs = lhs->next;
        result = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, disallow_chained_fastpaths, disallow_chained_casts, rhs);
        if(lhs->kind != E_ExprKind_LeafIdentifier)
        {
          e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Left side of assignment must be an unused identifier.");
        }
      }break;
    }
    
    //- rjf: if our currently-computed result is just a type, and we have
    // a chained expression, then treat the first expression as a cast on
    // the second.
    if(!disallow_chained_casts && result.mode == E_Mode_Null && expr->next != &e_expr_nil)
    {
      // rjf: unpack
      E_IRTreeAndType cast_type_tree = result;
      E_IRTreeAndType casted_tree = e_push_irtree_and_type_from_expr(arena, parent, disallow_autohooks, 1, disallow_chained_casts, expr->next);
      e_msg_list_concat_in_place(&result.msgs, &casted_tree.msgs);
      E_TypeKey cast_type = cast_type_tree.type_key;
      E_TypeKind cast_type_kind = e_type_kind_from_key(cast_type);
      U64 cast_type_byte_size = e_type_byte_size_from_key(cast_type);
      E_TypeKey casted_type = casted_tree.type_key;
      E_TypeKind casted_type_kind = e_type_kind_from_key(casted_type);
      U64 casted_type_byte_size = e_type_byte_size_from_key(casted_type);
      U8 in_group  = e_type_group_from_kind(casted_type_kind);
      U8 out_group = e_type_group_from_kind(cast_type_kind);
      RDI_EvalConversionKind conversion_rule = rdi_eval_conversion_kind_from_typegroups(in_group, out_group);
      
      // rjf: bad conditions? -> error if applicable, exit
      if(casted_tree.root->op == 0)
      {
        goto end_cast_gen;
      }
      else if(cast_type_kind == E_TypeKind_Null)
      {
        goto end_cast_gen;
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
        goto end_cast_gen;
      }
      
      // rjf: generate casted result
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
      
      end_cast_gen:;
    }
    
    //- rjf: check chained expressions for simple wrappers
    if(!disallow_chained_fastpaths)
    {
      struct
      {
        String8 shorthand;
        String8 full_name;
      }
      shorthand_lens_pair_table[] =
      {
        {str8_lit("x"), str8_lit("hex")},
        {str8_lit("b"), str8_lit("bin")},
        {str8_lit("o"), str8_lit("oct")},
        {str8_lit("d"), str8_lit("dec")},
      };
      for(E_Expr *chained_expr = expr->next;
          chained_expr != &e_expr_nil;
          chained_expr = chained_expr->next)
      {
        B32 matches_shorthand = 0;
        if(chained_expr->kind == E_ExprKind_LeafIdentifier)
        {
          for EachElement(shorthand_idx, shorthand_lens_pair_table)
          {
            if(str8_match(chained_expr->string, shorthand_lens_pair_table[shorthand_idx].shorthand, 0))
            {
              String8 full_name = shorthand_lens_pair_table[shorthand_idx].full_name;
              result.type_key = e_type_key_cons(.kind       = E_TypeKind_Lens,
                                                .direct_key = result.type_key,
                                                .name       = full_name);
              matches_shorthand = 1;
              break;
            }
          }
        }
        if(!matches_shorthand)
        {
          E_TypeKind type_kind = e_type_kind_from_key(e_type_key_unwrap(result.type_key, E_TypeUnwrapFlag_AllDecorative));
          if(e_type_kind_is_pointer_or_ref(type_kind) ||
             type_kind == E_TypeKind_Array)
          {
            E_Expr *lens_spec_expr = e_string2expr_map_lookup(e_ir_ctx->macro_map, str8_lit("array"));
            E_TypeKey lens_spec_type_key = lens_spec_expr->type_key;
            E_Type *lens_spec_type = e_type_from_key__cached(lens_spec_type_key);
            result.type_key = e_type_key_cons(.kind       = E_TypeKind_Lens,
                                              .flags      = lens_spec_type->flags,
                                              .count      = 1,
                                              .args       = &chained_expr,
                                              .direct_key = result.type_key,
                                              .name       = lens_spec_type->name,
                                              .irext      = lens_spec_type->irext,
                                              .access     = lens_spec_type->access,
                                              .expand     = lens_spec_type->expand);
          }
        }
      }
    }
    
    //- rjf: if the evaluated type has a hook for an extra layer of ir extension,
    // call into it
    E_Type *type = e_type_from_key__cached(result.type_key);
    {
      E_TypeIRExtFunctionType *irext = type->irext;
      for(E_Type *t = type; t->kind == E_TypeKind_Lens || t->kind == E_TypeKind_Set; t = e_type_from_key__cached(t->direct_type_key))
      {
        if(t->irext != 0)
        {
          irext = t->irext;
          break;
        }
      }
      if(irext != 0)
      {
        E_IRTreeAndType irtree_stripped = result;
        if(type->kind == E_TypeKind_Lens)
        {
          irtree_stripped.type_key = e_type_key_direct(irtree_stripped.type_key);
        }
        E_IRExt ext = irext(arena, expr, &irtree_stripped);
        result.user_data = ext.user_data;
      }
    }
    
    //- rjf: equip previous task's irtree
    if(t->overridden != 0)
    {
      result.prev = push_array(arena, E_IRTreeAndType, 1);
      result.prev[0] = *t->overridden;
    }
    
    //- rjf: find any auto hooks according to this generation's type
    if(!disallow_autohooks && result.mode != E_Mode_Null)
    {
      E_ExprList exprs = e_auto_hook_exprs_from_type_key__cached(result.type_key);
      for(E_ExprNode *n = exprs.first; n != 0; n = n->next)
      {
        for(E_Expr *e = n->v; e != &e_expr_nil; e = e->next)
        {
          B32 e_is_poisoned = e_expr_is_poisoned(e);
          if(!e_is_poisoned)
          {
            Task *task = push_array(scratch.arena, Task, 1);
            SLLQueuePush(first_task, last_task, task);
            task->expr = e;
            task->overridden = push_array(scratch.arena, E_IRTreeAndType, 1);
            task->overridden[0] = result;
            goto end_autohook_find;
          }
        }
      }
      end_autohook_find:;
    }
  }
  
  //////////////////////////////
  //- rjf: unpoison the tags we used
  //
  for(Task *t = first_task; t != 0; t = t->next)
  {
    e_expr_unpoison(t->expr);
  }
  
  //////////////////////////////
  //- rjf: apply inherited lenses to the resultant type
  //
  if(inherited_lenses.count != 0)
  {
    E_Type *result_type = e_type_from_key__cached(result.type_key);
    for(E_TypeKeyNode *n = inherited_lenses.first; n != 0; n = n->next)
    {
      E_Type *src_type = e_type_from_key__cached(n->v);
      E_TypeKey dst_type_key = e_type_key_cons(.kind   = src_type->kind,
                                               .flags  = src_type->flags,
                                               .name   = src_type->name,
                                               .count  = src_type->count,
                                               .args   = src_type->args,
                                               .irext  = src_type->irext,
                                               .access = src_type->access,
                                               .expand = src_type->expand,
                                               .direct_key = result.type_key);
      result.type_key = dst_type_key;
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
  lhs_bytecode->string = e_string_from_expr(arena, lhs, str8_zero());
  lhs_bytecode->qualifier = lhs->qualifier;
  lhs_bytecode->space = lhs->space;
  lhs_bytecode->mode = lhs_irtree->mode;
  lhs_bytecode->type_key = lhs_irtree->type_key;
  lhs_bytecode->bytecode = e_bytecode_from_oplist(arena, &lhs_oplist);
  E_Expr *rhs = e_push_expr(arena, E_ExprKind_LeafIdentifier, 0);
  rhs->string = push_str8_copy(arena, member_name);
  e_expr_push_child(root, lhs_bytecode);
  e_expr_push_child(root, rhs);
  return root;
}

internal E_Expr *
e_expr_irext_array_index(Arena *arena, E_Expr *lhs, E_IRTreeAndType *lhs_irtree, U64 index)
{
  E_Expr *root = e_push_expr(arena, E_ExprKind_ArrayIndex, 0);
  E_Expr *lhs_bytecode = e_push_expr(arena, E_ExprKind_LeafBytecode, lhs->location);
  E_OpList lhs_oplist = e_oplist_from_irtree(arena, lhs_irtree->root);
  lhs_bytecode->string = e_string_from_expr(arena, lhs, str8_zero());
  lhs_bytecode->qualifier = lhs->qualifier;
  lhs_bytecode->space = lhs->space;
  lhs_bytecode->mode = lhs_irtree->mode;
  lhs_bytecode->type_key = lhs_irtree->type_key;
  lhs_bytecode->bytecode = e_bytecode_from_oplist(arena, &lhs_oplist);
  E_Expr *rhs = e_push_expr(arena, E_ExprKind_LeafU64, 0);
  rhs->value.u64 = index;
  e_expr_push_child(root, lhs_bytecode);
  e_expr_push_child(root, rhs);
  return root;
}

internal E_Expr *
e_expr_irext_deref(Arena *arena, E_Expr *rhs, E_IRTreeAndType *rhs_irtree)
{
  E_Expr *root = e_push_expr(arena, E_ExprKind_Deref, 0);
  E_Expr *rhs_bytecode = e_push_expr(arena, E_ExprKind_LeafBytecode, rhs->location);
  E_OpList rhs_oplist = e_oplist_from_irtree(arena, rhs_irtree->root);
  rhs_bytecode->string = e_string_from_expr(arena, rhs, str8_zero());
  rhs_bytecode->space = rhs->space;
  rhs_bytecode->mode = rhs_irtree->mode;
  rhs_bytecode->type_key = rhs_irtree->type_key;
  rhs_bytecode->bytecode = e_bytecode_from_oplist(arena, &rhs_oplist);
  e_expr_push_child(root, rhs_bytecode);
  return root;
}
