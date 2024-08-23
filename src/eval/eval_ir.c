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
  return e_ir_ctx;
}

internal void
e_select_ir_ctx(E_IRCtx *ctx)
{
  e_ir_ctx = ctx;
}

////////////////////////////////
//~ rjf: IR-ization Functions

//- rjf: op list functions

internal void
e_oplist_push_op(Arena *arena, E_OpList *list, RDI_EvalOp opcode, E_Value value)
{
  U8 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
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
  node->value.u128 = space;
  SLLQueuePush(list->first, list->last, node);
  list->op_count += 1;
  list->encoded_size += 1 + sizeof(space);
}

internal void
e_oplist_push_string_literal(Arena *arena, E_OpList *list, String8 string)
{
  RDI_EvalOp opcode = RDI_EvalOp_ConstString;
  U8 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
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
  root->value.u128 = space;
  e_irnode_push_child(root, c);
  return root;
}

internal E_IRNode *
e_irtree_mem_read_type(Arena *arena, E_Space space, E_IRNode *c, E_TypeKey type_key)
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
  
  // rjf: set space for this mem read
  E_IRNode *set_space_node = e_irtree_set_space(arena, space, with_trunc);
  
  // rjf: fill
  result = set_space_node;
  
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
e_irtree_resolve_to_value(Arena *arena, E_Space from_space, E_Mode from_mode, E_IRNode *tree, E_TypeKey type_key)
{
  // TODO(rjf): @spaces double check that this path is working for register spaces
  E_IRNode *result = tree;
  if(from_mode == E_Mode_Offset)
  {
    result = e_irtree_mem_read_type(arena, from_space, tree, type_key);
  }
  return result;
}

//- rjf: top-level irtree/type extraction

internal E_IRTreeAndType
e_irtree_and_type_from_expr(Arena *arena, E_Expr *expr)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  E_ExprKind kind = expr->kind;
  switch(kind)
  {
    default:{}break;
    
    //- rjf: references -> just descend to sub-expr
    case E_ExprKind_Ref:
    {
      result = e_irtree_and_type_from_expr(arena, expr->ref);
    }break;
    
    //- rjf: array indices
    case E_ExprKind_ArrayIndex:
    {
      // rjf: unpack left/right expressions
      E_Expr *exprl = expr->first;
      E_Expr *exprr = exprl->next;
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
      e_msg_list_concat_in_place(&result.msgs, &l.msgs);
      e_msg_list_concat_in_place(&result.msgs, &r.msgs);
      
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
      {
        switch(l.mode)
        {
          // rjf: null (types) -> just resolve to new type
          case E_Mode_Null:
          {
          }break;
          
          // rjf: offsets -> read from base offset
          default:
          case E_Mode_Offset:
          {
            // rjf: ops to compute the offset
            E_IRNode *offset_tree = e_irtree_resolve_to_value(arena, r.space, r.mode, r.root, r_restype);
            if(direct_type_size > 1)
            {
              E_IRNode *const_tree = e_irtree_const_u(arena, direct_type_size);
              offset_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Mul, offset_tree, const_tree);
            }
            
            // rjf: ops to compute the base offset (resolve to value if addr-of-pointer)
            E_IRNode *base_tree = l.root;
            if(l_restype_kind == E_TypeKind_Ptr && l.mode != E_Mode_Value)
            {
              base_tree = e_irtree_resolve_to_value(arena, l.space, l.mode, base_tree, l_restype);
            }
            
            // rjf: ops to compute the final address
            new_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Add, offset_tree, base_tree);
          }break;
          
          // rjf: values -> read from stack value
          case E_Mode_Value:
          {
            // rjf: ops to compute the offset
            E_IRNode *offset_tree = e_irtree_resolve_to_value(arena, r.space, r.mode, r.root, r_restype);
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
          }break;
        }
      }
      
      // rjf: fill
      result.root     = new_tree;
      result.type_key = direct_type;
      result.mode     = l.mode;
      result.space    = l.space;
    }break;
    
    //- rjf: member accesses
    case E_ExprKind_MemberAccess:
    {
      // rjf: unpack left/right expressions
      E_Expr *exprl = expr->first;
      E_Expr *exprr = exprl->next;
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
      e_msg_list_concat_in_place(&result.msgs, &l.msgs);
      
      // rjf: look up member
      B32 r_found = 0;
      E_TypeKey r_type = zero_struct;
      U32 r_off = 0;
      {
        Temp scratch = scratch_begin(&arena, 1);
        E_MemberArray check_type_members = e_type_data_members_from_key(scratch.arena, check_type_key);
        E_Member *match = 0;
        for(U64 member_idx = 0; member_idx < check_type_members.count; member_idx += 1)
        {
          E_Member *member = &check_type_members.v[member_idx];
          if(str8_match(member->name, exprr->string, 0))
          {
            match = member;
            break;
          }
          else if(str8_match(member->name, exprr->string, StringMatchFlag_CaseInsensitive))
          {
            match = member;
          }
        }
        if(match != 0)
        {
          r_found = 1;
          r_type = match->type_key;
          r_off = match->off;
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
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprl->location, "Expected member name.");
        break;
      }
      else if(!r_found)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprr->location, "Could not find a member named `%S`.", exprr->string);
        break;
      }
      else if(check_type_kind != E_TypeKind_Struct &&
              check_type_kind != E_TypeKind_Class &&
              check_type_kind != E_TypeKind_Union)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, exprl->location, "Cannot perform member access on this type.");
        break;
      }
      
      // rjf: generate
      {
        // rjf: build tree
        E_IRNode *new_tree = l.root;
        E_Mode mode = l.mode;
        if(l.root != &e_irnode_nil)
        {
          if(l_restype_kind == E_TypeKind_Ptr ||
             l_restype_kind == E_TypeKind_LRef ||
             l_restype_kind == E_TypeKind_RRef)
          {
            new_tree = e_irtree_resolve_to_value(arena, l.space, l.mode, new_tree, l_restype);
            mode = E_Mode_Offset;
          }
          if(r_off != 0)
          {
            E_IRNode *const_tree = e_irtree_const_u(arena, r_off);
            new_tree = e_irtree_binary_op_u(arena, RDI_EvalOp_Add, new_tree, const_tree);
          }
        }
        
        // rjf: fill
        result.root     = new_tree;
        result.type_key = r_type;
        result.mode     = mode;
        result.space    = l.space;
      }
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
          new_tree = e_irtree_resolve_to_value(arena, r_tree.space, r_tree.mode, r_tree.root, r_type);
        }
        result.root     = new_tree;
        result.type_key = r_type_direct;
        result.mode     = E_Mode_Offset;
        result.space    = r_tree.space;
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
      result.type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, r_type_unwrapped);
      result.mode     = E_Mode_Value;
      result.space    = r_tree.space;
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
        E_IRNode *in_tree = e_irtree_resolve_to_value(arena, casted_tree.space, casted_tree.mode, casted_tree.root, casted_type);
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
        result.space    = casted_tree.space;
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
          space = r_tree.space;
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
        result.space    = space;
      }
    }break;
    
    //- rjf: unary operations
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
        E_IRNode *in_tree = e_irtree_resolve_to_value(arena, r_tree.space, r_tree.mode, r_tree.root, r_type);
        in_tree = e_irtree_convert_hi(arena, in_tree, r_type_promoted, r_type);
        E_IRNode *new_tree = e_irtree_unary_op(arena, op, r_type_group, in_tree);
        result.root     = new_tree;
        result.type_key = r_type_promoted;
        result.mode     = E_Mode_Value;
        result.space    = r_tree.space;
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
      E_TypeKey l_type = e_type_unwrap(l_tree.type_key);
      E_TypeKey r_type = e_type_unwrap(r_tree.type_key);
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
        if(l_type_kind == E_TypeKind_Array && r_type_kind == E_TypeKind_Ptr)
        {
          arith_path = E_ArithPath_PtrArrayCompare;
        }
        if(r_type_kind == E_TypeKind_Array && l_type_kind == E_TypeKind_Ptr)
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
            E_IRNode *l_value_tree = e_irtree_resolve_to_value(arena, l_tree.space, l_tree.mode, l_tree.root, l_type);
            E_IRNode *r_value_tree = e_irtree_resolve_to_value(arena, r_tree.space, r_tree.mode, r_tree.root, r_type);
            l_value_tree = e_irtree_convert_hi(arena, l_value_tree, l_type, l_type);
            r_value_tree = e_irtree_convert_hi(arena, r_value_tree, l_type, r_type);
            E_IRNode *new_tree = e_irtree_binary_op(arena, op, l_type_group, l_value_tree, r_value_tree);
            result.root     = new_tree;
            result.type_key = final_type_key;
            result.mode     = E_Mode_Value;
            result.space    = l_tree.space;
            E_Space zero_space = {0};
            if(MemoryMatchStruct(&result.space, &zero_space))
            {
              result.space = r_tree.space;
            }
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
              ptr_root = e_irtree_resolve_to_value(arena, ptr_tree->space, ptr_tree->mode, ptr_root, ptr_tree->type_key);
            }
            E_IRNode *int_root = int_tree->root;
            if(direct_type_size > 1)
            {
              E_IRNode *const_root = e_irtree_const_u(arena, direct_type_size);
              int_root = e_irtree_binary_op_u(arena, RDI_EvalOp_Mul, int_root, const_root);
            }
            E_TypeKey ptr_type = ptr_tree->type_key;
            if(ptr_is_decay)
            {
              ptr_type = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, direct_type);
            }
            E_IRNode *new_root = e_irtree_binary_op_u(arena, op, ptr_root, int_root);
            result.root     = new_root;
            result.type_key = ptr_type;
            result.mode     = E_Mode_Value;
            result.space    = l_tree.space;
            E_Space zero_space = {0};
            if(MemoryMatchStruct(&result.space, &zero_space))
            {
              result.space = r_tree.space;
            }
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
            l_root = e_irtree_resolve_to_value(arena, l_tree.space, l_tree.mode, l_root, l_type);
          }
          if(!r_is_decay)
          {
            r_root = e_irtree_resolve_to_value(arena, r_tree.space, r_tree.mode, r_root, r_type);
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
          result.space    = l_tree.space;
          E_Space zero_space = {0};
          if(MemoryMatchStruct(&result.space, &zero_space))
          {
            result.space = r_tree.space;
          }
        }break;
        
        //- rjf: pointer array comparison
        case E_ArithPath_PtrArrayCompare:
        {
          // rjf: map l/r to pointer/array
          E_IRTreeAndType *ptr_tree = &l_tree;
          E_IRTreeAndType *arr_tree = &r_tree;
          if(l_type_kind == E_TypeKind_Array)
          {
            ptr_tree = &r_tree;
            arr_tree = &l_tree;
          }
          
          // rjf: resolve pointer to value, sized same as array
          E_IRNode *ptr_root = ptr_tree->root;
          E_IRNode *arr_root = arr_tree->root;
          {
            ptr_root = e_irtree_resolve_to_value(arena, ptr_tree->space, ptr_tree->mode, ptr_tree->root, ptr_tree->type_key);
          }
          
          // rjf: read from pointer into value, to compare with array
          E_IRNode *mem_root = e_irtree_mem_read_type(arena, ptr_tree->space, ptr_root, arr_tree->type_key);
          
          // rjf: generate
          result.root     = e_irtree_binary_op(arena, op, RDI_EvalTypeGroup_Other, mem_root, arr_root);
          result.type_key = e_type_key_basic(E_TypeKind_Bool);
          result.mode     = E_Mode_Value;
          result.space    = ptr_tree->space;
          E_Space zero_space = {0};
          if(MemoryMatchStruct(&result.space, &zero_space))
          {
            result.space = arr_tree->space;
          }
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
        E_IRNode *c_value_tree = e_irtree_resolve_to_value(arena, c_tree.space, c_tree.mode, c_tree.root, c_type);
        E_IRNode *l_value_tree = e_irtree_resolve_to_value(arena, l_tree.space, l_tree.mode, l_tree.root, l_type);
        E_IRNode *r_value_tree = e_irtree_resolve_to_value(arena, r_tree.space, r_tree.mode, r_tree.root, r_type);
        l_value_tree = e_irtree_convert_hi(arena, l_value_tree, result_type, l_type);
        r_value_tree = e_irtree_convert_hi(arena, r_value_tree, result_type, r_type);
        E_IRNode *new_tree = e_irtree_conditional(arena, c_value_tree, l_value_tree, r_value_tree);
        result.root     = new_tree;
        result.type_key = result_type;
        result.mode     = E_Mode_Value;
        result.space = l_expr->space;
        E_Space zero_space = {0};
        if(MemoryMatchStruct(&result.space, &zero_space))
        {
          result.space = r_expr->space;
        }
      }
    }break;
    
    //- rjf: leaf bytecode
    case E_ExprKind_LeafBytecode:
    {
      E_IRNode *new_tree = e_irtree_bytecode_no_copy(arena, expr->bytecode);
      E_TypeKey final_type_key = expr->type_key;
      result.root     = new_tree;
      result.type_key = final_type_key;
      result.mode     = expr->mode;
      result.space    = expr->space;
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
      E_Expr *macro_expr = e_expr_from_string(e_ir_ctx->macro_map, expr->string);
      if(macro_expr == &e_expr_nil)
      {
        e_msgf(arena, &result.msgs, E_MsgKind_ResolutionFailure, expr->location, "`%S` could not be found.", expr->string);
      }
      else
      {
        e_string2expr_map_inc_poison(e_ir_ctx->macro_map, expr->string);
        result = e_irtree_and_type_from_expr(arena, macro_expr);
        e_string2expr_map_dec_poison(e_ir_ctx->macro_map, expr->string);
      }
    }break;
    
    //- rjf: leaf offsets
    case E_ExprKind_LeafOffset:
    {
      E_IRNode *new_tree = e_push_irnode(arena, RDI_EvalOp_ConstU64);
      new_tree->value.u64 = expr->value.u64;
      result.root     = new_tree;
      result.type_key = expr->type_key;
      result.mode     = E_Mode_Offset;
      result.space    = expr->space;
    }break;
    
    //- rjf: leaf file paths
    case E_ExprKind_LeafFilePath:
    {
      U128 key = fs_key_from_path_range(expr->string, r1u64(0, max_U64));
      U64 size = fs_size_from_path(expr->string);
      E_IRNode *base_offset = e_irtree_const_u(arena, 0);
      E_IRNode *set_space = e_irtree_set_space(arena, key, base_offset);
      result.root     = set_space;
      result.type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), size);
      result.mode     = E_Mode_Offset;
      result.space    = key;
    }break;
    
    //- rjf: types
    case E_ExprKind_TypeIdent:
    {
      result.root = &e_irnode_nil;
      result.type_key = expr->type_key;
      result.mode = E_Mode_Null;
      result.space = expr->space;
    }break;
    
    //- rjf: (unexpected) type expressions
    case E_ExprKind_Ptr:
    case E_ExprKind_Array:
    case E_ExprKind_Func:
    {
      e_msgf(arena, &result.msgs, E_MsgKind_MalformedInput, expr->location, "Type expression not expected.");
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

//- rjf: irtree -> linear ops/bytecode

internal void
e_append_oplist_from_irtree(Arena *arena, E_IRNode *root, E_OpList *out)
{
  U32 op = root->op;
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
      e_oplist_push_set_space(arena, out, root->value.u128);
      for(E_IRNode *child = root->first;
          child != &e_irnode_nil;
          child = child->next)
      {
        e_append_oplist_from_irtree(arena, child, out);
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
        U8 ctrlbits = rdi_eval_op_ctrlbits_table[op];
        U64 child_count = RDI_POPN_FROM_CTRLBITS(ctrlbits);
        U64 idx = 0;
        for(E_IRNode *child = root->first;
            child != &e_irnode_nil && idx < child_count;
            child = child->next, idx += 1)
        {
          e_append_oplist_from_irtree(arena, child, out);
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
  e_append_oplist_from_irtree(arena, root, &ops);
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
        U8 ctrlbits = rdi_eval_op_ctrlbits_table[opcode];
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
