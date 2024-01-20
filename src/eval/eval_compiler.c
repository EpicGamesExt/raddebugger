// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ allen: Eval Errors

internal void
eval_error(Arena *arena, EVAL_ErrorList *list, EVAL_ErrorKind kind, void *location, String8 text){
  EVAL_Error *error = push_array_no_zero(arena, EVAL_Error, 1);
  SLLQueuePush(list->first, list->last, error);
  list->count += 1;
  list->max_kind = Max(kind, list->max_kind);
  error->kind = kind;
  error->location = location;
  error->text = text;
}

internal void
eval_errorf(Arena *arena, EVAL_ErrorList *list, EVAL_ErrorKind kind, void *location, char *fmt, ...){
  va_list args;
  va_start(args, fmt);
  String8 text = push_str8fv(arena, fmt, args);
  va_end(args);
  eval_error(arena, list, kind, location, text);
}

internal void
eval_error_list_concat_in_place(EVAL_ErrorList *dst, EVAL_ErrorList *to_push){
  if (dst->last != 0){
    if (to_push->last != 0){
      dst->last->next = to_push->first;
      dst->last = to_push->last;
      dst->count += to_push->count;
    }
  }
  else{
    *dst = *to_push;
  }
  MemoryZeroStruct(to_push);
}

////////////////////////////////
//~ allen: EVAL Bytecode Helpers

internal String8
eval_bytecode_from_oplist(Arena *arena, EVAL_OpList *list){
  // allocate output
  U64 size = list->encoded_size;
  U8 *str = push_array_no_zero(arena, U8, size);
  
  // iterate loose op nodes
  U8 *ptr = str;
  U8 *opl = str + size;
  for (EVAL_Op *op = list->first_op;
       op != 0;
       op = op->next){
    U32 opcode = op->opcode;
    
    switch (opcode){
      default:
      {
        // compute bytecode advance
        U8 ctrlbits = raddbg_eval_opcode_ctrlbits[opcode];
        U64 extra_byte_count = RADDBG_DECODEN_FROM_CTRLBITS(ctrlbits);
        
        U8 *next_ptr = ptr + 1 + extra_byte_count;
        Assert(next_ptr <= opl);
        
        // fill bytecode
        ptr[0] = opcode;
        MemoryCopy(ptr + 1, &op->p, extra_byte_count);
        
        // advance output pointer
        ptr = next_ptr;
      }break;
      
      case EVAL_IRExtKind_Bytecode:
      {
        // compute bytecode advance
        U64 size = op->bytecode.size;
        U8 *next_ptr = ptr + size;
        Assert(next_ptr <= opl);
        
        // fill bytecode
        MemoryCopy(ptr, op->bytecode.str, size);
        
        // advance output pointer
        ptr = next_ptr;
      }break;
    }
  }
  
  // fill result
  String8 result = {0};
  result.size = size;
  result.str = str;
  return(result);
}

internal void
eval_oplist_push_op(Arena *arena, EVAL_OpList *list, RADDBG_EvalOp opcode, U64 p){
  U8 ctrlbits = raddbg_eval_opcode_ctrlbits[opcode];
  U32 p_size = RADDBG_DECODEN_FROM_CTRLBITS(ctrlbits);
  
  EVAL_Op *node = push_array_no_zero(arena, EVAL_Op, 1);
  node->opcode = opcode;
  node->p = p;
  
  SLLQueuePush(list->first_op, list->last_op, node);
  list->op_count += 1;
  list->encoded_size += 1 + p_size;
}

internal void
eval_oplist_push_uconst(Arena *arena, EVAL_OpList *list, U64 x){
  if (x <= 0xFF){
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU8, x);
  }
  else if (x <= 0xFFFF){
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU16, x);
  }
  else if (x <= 0xFFFFFFFF){
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU32, x);
  }
  else{
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU64, x);
  }
}

internal void
eval_oplist_push_sconst(Arena *arena, EVAL_OpList *list, S64 x){
  if (-0x80 <= x && x <= 0x7F){
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU8, (U64)x);
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_TruncSigned, 8);
  }
  else if (-0x8000 <= x && x <= 0x7FFF){
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU16, (U64)x);
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_TruncSigned, 16);
  }
  else if (-0x80000000ll <= x && x <= 0x7FFFFFFFll){
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU32, (U64)x);
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_TruncSigned, 32);
  }
  else{
    eval_oplist_push_op(arena, list, RADDBG_EvalOp_ConstU64, (U64)x);
  }
}

internal void
eval_oplist_push_bytecode(Arena *arena, EVAL_OpList *list, String8 bytecode){
  EVAL_Op *node = push_array_no_zero(arena, EVAL_Op, 1);
  node->opcode = EVAL_IRExtKind_Bytecode;
  node->bytecode = bytecode;
  SLLQueuePush(list->first_op, list->last_op, node);
  list->op_count += 1;
  list->encoded_size += bytecode.size;
}

internal void
eval_oplist_concat_in_place(EVAL_OpList *left_dst, EVAL_OpList *right_destroyed){
  if (right_destroyed->first_op != 0){
    if (left_dst->first_op == 0){
      MemoryCopyStruct(left_dst, right_destroyed);
    }
    else{
      left_dst->last_op = right_destroyed->last_op;
      left_dst->op_count += right_destroyed->op_count;
      left_dst->encoded_size += right_destroyed->encoded_size;
    }
    MemoryZeroStruct(right_destroyed);
  }
}

////////////////////////////////
//~ allen: EVAL Expression Info Functions

internal RADDBG_EvalOp
eval_opcode_from_expr_kind(EVAL_ExprKind kind){
  RADDBG_EvalOp result = RADDBG_EvalOp_Stop;
  switch (kind){
    case EVAL_ExprKind_Neg:    result = RADDBG_EvalOp_Neg;    break;
    case EVAL_ExprKind_LogNot: result = RADDBG_EvalOp_LogNot; break;
    case EVAL_ExprKind_BitNot: result = RADDBG_EvalOp_BitNot; break;
    case EVAL_ExprKind_Mul:    result = RADDBG_EvalOp_Mul;    break;
    case EVAL_ExprKind_Div:    result = RADDBG_EvalOp_Div;    break;
    case EVAL_ExprKind_Mod:    result = RADDBG_EvalOp_Mod;    break;
    case EVAL_ExprKind_Add:    result = RADDBG_EvalOp_Add;    break;
    case EVAL_ExprKind_Sub:    result = RADDBG_EvalOp_Sub;    break;
    case EVAL_ExprKind_LShift: result = RADDBG_EvalOp_LShift; break;
    case EVAL_ExprKind_RShift: result = RADDBG_EvalOp_RShift; break;
    case EVAL_ExprKind_Less:   result = RADDBG_EvalOp_Less;   break;
    case EVAL_ExprKind_LsEq:   result = RADDBG_EvalOp_LsEq;   break;
    case EVAL_ExprKind_Grtr:   result = RADDBG_EvalOp_Grtr;   break;
    case EVAL_ExprKind_GrEq:   result = RADDBG_EvalOp_GrEq;   break;
    case EVAL_ExprKind_EqEq:   result = RADDBG_EvalOp_EqEq;   break;
    case EVAL_ExprKind_NtEq:   result = RADDBG_EvalOp_NtEq;   break;
    case EVAL_ExprKind_BitAnd: result = RADDBG_EvalOp_BitAnd; break;
    case EVAL_ExprKind_BitXor: result = RADDBG_EvalOp_BitXor; break;
    case EVAL_ExprKind_BitOr:  result = RADDBG_EvalOp_BitOr;  break;
    case EVAL_ExprKind_LogAnd: result = RADDBG_EvalOp_LogAnd; break;
    case EVAL_ExprKind_LogOr:  result = RADDBG_EvalOp_LogOr;  break;
  }
  return(result);
}

internal B32
eval_expr_kind_is_comparison(EVAL_ExprKind kind){
  B32 result = 0;
  switch (kind){
    case EVAL_ExprKind_EqEq:
    case EVAL_ExprKind_NtEq:
    case EVAL_ExprKind_Less:
    case EVAL_ExprKind_Grtr:
    case EVAL_ExprKind_LsEq:
    case EVAL_ExprKind_GrEq:
    {
      result = 1;
    }break;
  }
  return(result);
}

////////////////////////////////
//~ allen: EVAL Expression Constructors

internal EVAL_Expr*
eval_expr(Arena *arena, EVAL_ExprKind kind, void *location,
          EVAL_Expr *c0, EVAL_Expr *c1, EVAL_Expr *c2){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->kind = kind;
  result->location = location;
  result->children[0] = c0;
  result->children[1] = c1;
  result->children[2] = c2;
  return(result);
}

internal EVAL_Expr*
eval_expr_u64(Arena *arena, void *location, U64 u64){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->kind = EVAL_ExprKind_LeafU64;
  result->location = location;
  result->u64 = u64;
  return(result);
}

internal EVAL_Expr*
eval_expr_f64(Arena *arena, void *location, F64 f64){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->kind = EVAL_ExprKind_LeafF64;
  result->location = location;
  result->f64 = f64;
  return(result);
}

internal EVAL_Expr*
eval_expr_f32(Arena *arena, void *location, F32 f32){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->kind = EVAL_ExprKind_LeafF32;
  result->location = location;
  result->f32 = f32;
  return(result);
}

internal EVAL_Expr*
eval_expr_child_and_u64(Arena *arena, EVAL_ExprKind kind, void *location,
                        EVAL_Expr *child, U64 u64){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->kind = kind;
  result->location = location;
  result->child_and_constant.child = child;
  result->child_and_constant.u64 = u64;
  return(result);
}

internal EVAL_Expr*
eval_expr_leaf_member(Arena *arena, void *location, String8 name){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->location = location;
  result->kind = EVAL_ExprKind_LeafMember;
  result->name = name;
  return(result);
}

internal EVAL_Expr*
eval_expr_leaf_bytecode(Arena *arena, void *location, TG_Key type_key, String8 bytecode, EVAL_EvalMode mode){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->location = location;
  result->kind = EVAL_ExprKind_LeafBytecode;
  result->type_key = type_key;
  result->bytecode = bytecode;
  result->mode = mode;
  return(result);
}

internal EVAL_Expr*
eval_expr_leaf_op_list(Arena *arena, void *location, TG_Key type_key, EVAL_OpList *ops, EVAL_EvalMode mode){
  String8 bytecode = eval_bytecode_from_oplist(arena, ops);
  EVAL_Expr *result = eval_expr_leaf_bytecode(arena, location, type_key, bytecode, mode);
  return(result);
}

internal EVAL_Expr*
eval_expr_leaf_type(Arena *arena, void *location, TG_Key type_key){
  EVAL_Expr *result = push_array(arena, EVAL_Expr, 1);
  result->location = location;
  result->kind = EVAL_ExprKind_TypeIdent;
  result->type_key = type_key;
  return(result);
}

////////////////////////////////
//~ allen: EVAL Type Information Transformers

internal RADDBG_EvalTypeGroup
eval_type_group_from_kind(TG_Kind kind){
  RADDBG_EvalTypeGroup result = 0;
  switch (kind){
    default:{}break;
    
    case TG_Kind_Null: case TG_Kind_Void:
    case TG_Kind_F16:  case TG_Kind_F32PP: case TG_Kind_F48:
    case TG_Kind_F80:  case TG_Kind_F128:
    case TG_Kind_ComplexF32: case TG_Kind_ComplexF64:
    case TG_Kind_ComplexF80: case TG_Kind_ComplexF128:
    case TG_Kind_Modifier:   case TG_Kind_Array:
    case TG_Kind_Struct:     case TG_Kind_Class: case TG_Kind_Union:
    case TG_Kind_Enum:       case TG_Kind_Alias:
    case TG_Kind_IncompleteStruct: case TG_Kind_IncompleteClass:
    case TG_Kind_IncompleteUnion:  case TG_Kind_IncompleteEnum:
    case TG_Kind_Bitfield:   case TG_Kind_Variadic:
    result = RADDBG_EvalTypeGroup_Other; break;
    
    case TG_Kind_Handle:
    case TG_Kind_UChar8: case TG_Kind_UChar16: case TG_Kind_UChar32:
    case TG_Kind_U8:     case TG_Kind_U16:     case TG_Kind_U32:
    case TG_Kind_U64:    case TG_Kind_U128:    case TG_Kind_U256:
    case TG_Kind_U512:
    case TG_Kind_Ptr: case TG_Kind_LRef: case TG_Kind_RRef:
    case TG_Kind_Function: case TG_Kind_Method: case TG_Kind_MemberPtr:
    result = RADDBG_EvalTypeGroup_U; break;
    
    case TG_Kind_Char8: case TG_Kind_Char16: case TG_Kind_Char32:
    case TG_Kind_S8:    case TG_Kind_S16:    case TG_Kind_S32:
    case TG_Kind_S64:   case TG_Kind_S128:   case TG_Kind_S256:
    case TG_Kind_S512:
    case TG_Kind_Bool:
    result = RADDBG_EvalTypeGroup_S; break;
    
    case TG_Kind_F32:
    result = RADDBG_EvalTypeGroup_F32; break;
    
    case TG_Kind_F64:
    result = RADDBG_EvalTypeGroup_F64; break;
  }
  return(result);
}

internal TG_Key
eval_type_unwrap(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_Key result = key;
  for(B32 good = 1; good;)
  {
    TG_Kind kind = tg_kind_from_key(result);
    if((TG_Kind_FirstIncomplete <= kind && kind <= TG_Kind_LastIncomplete) ||
       kind == TG_Kind_Modifier ||
       kind == TG_Kind_Alias)
    {
      result = tg_direct_from_graph_raddbg_key(graph, rdbg, result);
    }
    else
    {
      good = 0;
    }
  }
  return result;
}

internal TG_Key
eval_type_unwrap_enum(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key)
{
  TG_Key result = key;
  for(B32 good = 1; good;)
  {
    TG_Kind kind = tg_kind_from_key(key);
    if(kind == TG_Kind_Enum)
    {
      result = tg_direct_from_graph_raddbg_key(graph, rdbg, result);
    }
    else
    {
      good = 0;
    }
  }
  return result;
}

internal TG_Key
eval_type_promote(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key key){
  TG_Key result = key;
  TG_Kind kind = tg_kind_from_key(key);
  if(kind == TG_Kind_Bool ||
     kind == TG_Kind_S8 ||
     kind == TG_Kind_S16 ||
     kind == TG_Kind_U8 ||
     kind == TG_Kind_U16)
  {
    result = tg_key_basic(TG_Kind_S32);
  }
  return result;
}

internal TG_Key
eval_type_coerce(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key l, TG_Key r){
  Assert(eval_kind_is_basic_or_enum(tg_kind_from_key(l)) &&
         eval_kind_is_basic_or_enum(tg_kind_from_key(r)));
  
  // replace enums with corresponding ints
  TG_Key lt = eval_type_unwrap_enum(graph, rdbg, l);
  TG_Key rt = eval_type_unwrap_enum(graph, rdbg, r);
  
  // first promote each
  TG_Key lp = eval_type_promote(graph, rdbg, lt);
  TG_Key rp = eval_type_promote(graph, rdbg, rt);
  TG_Kind lk = tg_kind_from_key(lp);
  TG_Kind rk = tg_kind_from_key(rp);
  
  TG_Key result = l;
  return(result);
}

internal B32
eval_type_match(TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key l, TG_Key r){
  B32 result = 0;
  
  // unwrap
  TG_Key lu = eval_type_unwrap(graph, rdbg, l);
  TG_Key ru = eval_type_unwrap(graph, rdbg, r);
  
  if (tg_key_match(lu, ru)){
    result = 1;
  }
  else{
    TG_Kind luk = tg_kind_from_key(lu);
    TG_Kind ruk = tg_kind_from_key(ru);
    if (luk == ruk){
      switch (luk){
        default:
        {
          result = 1;
        }break;
        
        case TG_Kind_Ptr:
        case TG_Kind_LRef:
        case TG_Kind_RRef:
        {
          TG_Key lud = tg_direct_from_graph_raddbg_key(graph, rdbg, lu);
          TG_Key rud = tg_direct_from_graph_raddbg_key(graph, rdbg, ru);
          if (eval_type_match(graph, rdbg, lud, rud)){
            result = 1;
          }
        }break;
        
        case TG_Kind_MemberPtr:
        {
          TG_Key lud = tg_direct_from_graph_raddbg_key(graph, rdbg, lu);
          TG_Key rud = tg_direct_from_graph_raddbg_key(graph, rdbg, ru);
          TG_Key luo = tg_owner_from_graph_raddbg_key(graph, rdbg, lu);
          TG_Key ruo = tg_owner_from_graph_raddbg_key(graph, rdbg, ru);
          if (eval_type_match(graph, rdbg, lud, rud) &&
              eval_type_match(graph, rdbg, luo, ruo)){
            result = 1;
          }
        }break;
        
        case TG_Kind_Array:
        {
          Temp scratch = scratch_begin(0, 0);
          TG_Type *lt = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, l);
          TG_Type *rt = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, r);
          if(lt->count == rt->count && eval_type_match(graph, rdbg, lt->direct_type_key, rt->direct_type_key))
          {
            result = 1;
          }
          scratch_end(scratch);
        }break;
        
        case TG_Kind_Function:
        {
          Temp scratch = scratch_begin(0, 0);
          TG_Type *lt = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, l);
          TG_Type *rt = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, r);
          if (lt->count == rt->count && eval_type_match(graph, rdbg, lt->direct_type_key, rt->direct_type_key))
          {
            B32 params_match = 1;
            TG_Key *lp = lt->param_type_keys;
            TG_Key *rp = rt->param_type_keys;
            U64 count = lt->count;
            for(U64 i = 0; i < count; i += 1, lp += 1, rp += 1)
            {
              if(!eval_type_match(graph, rdbg, *lp, *rp))
              {
                params_match = 0;
                break;
              }
            }
            result = params_match;
          }
          scratch_end(scratch);
        }break;
        
        case TG_Kind_Method:
        {
          Temp scratch = scratch_begin(0, 0);
          TG_Type *lt = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, l);
          TG_Type *rt = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, r);
          if (lt->count == rt->count &&
              eval_type_match(graph, rdbg, lt->direct_type_key, rt->direct_type_key) &&
              eval_type_match(graph, rdbg, lt->owner_type_key, rt->owner_type_key))
          {
            B32 params_match = 1;
            TG_Key *lp = lt->param_type_keys;
            TG_Key *rp = rt->param_type_keys;
            U64 count = lt->count;
            for(U64 i = 0; i < count; i += 1, lp += 1, rp += 1)
            {
              if(!eval_type_match(graph, rdbg, *lp, *rp))
              {
                params_match = 0;
                break;
              }
            }
            result = params_match;
          }
          scratch_end(scratch);
        }break;
      }
    }
  }
  
  return(result);
}

internal B32
eval_kind_is_integer(TG_Kind kind){
  B32 result = (TG_Kind_FirstInteger <= kind && kind <= TG_Kind_LastInteger);
  return(result);
}

internal B32
eval_kind_is_signed(TG_Kind kind){
  B32 result = ((TG_Kind_FirstSigned1 <= kind && kind <= TG_Kind_LastSigned1) ||
                (TG_Kind_FirstSigned2 <= kind && kind <= TG_Kind_LastSigned2));
  return(result);
}

internal B32
eval_kind_is_basic_or_enum(TG_Kind kind){
  B32 result = ((TG_Kind_FirstBasic <= kind && kind <= TG_Kind_LastBasic) ||
                kind == TG_Kind_Enum);
  return(result);
}


////////////////////////////////
//~ allen: EVAL IR-Tree Constructors

internal EVAL_IRTree*
eval_irtree_const_u(Arena *arena, U64 v){
  // choose encoding op
  RADDBG_EvalOp op = RADDBG_EvalOp_ConstU64;
  if (v < 0x100){
    op = RADDBG_EvalOp_ConstU8;
  }
  else if (v < 0x10000){
    op = RADDBG_EvalOp_ConstU16;
  }
  else if (v < 0x100000000){
    op = RADDBG_EvalOp_ConstU32;
  }
  
  // make the tree node
  EVAL_IRTree *result = push_array(arena, EVAL_IRTree, 1);
  result->op = op;
  result->p = v;
  return(result);
}

internal EVAL_IRTree*
eval_irtree_unary_op(Arena *arena, RADDBG_EvalOp op,
                     RADDBG_EvalTypeGroup group, EVAL_IRTree *c){
  EVAL_IRTree *result = push_array(arena, EVAL_IRTree, 1);
  result->op = op;
  result->p = group;
  result->children[0] = c;
  return(result);
}

internal EVAL_IRTree*
eval_irtree_binary_op(Arena *arena, RADDBG_EvalOp op, RADDBG_EvalTypeGroup group,
                      EVAL_IRTree *l, EVAL_IRTree *r){
  EVAL_IRTree *result = push_array(arena, EVAL_IRTree, 1);
  result->op = op;
  result->p = group;
  result->children[0] = l;
  result->children[1] = r;
  return(result);
}

internal EVAL_IRTree*
eval_irtree_binary_op_u(Arena *arena, RADDBG_EvalOp op, EVAL_IRTree *l, EVAL_IRTree *r){
  EVAL_IRTree *result = eval_irtree_binary_op(arena, op, RADDBG_EvalTypeGroup_U, l, r);
  return(result);
}

internal EVAL_IRTree*
eval_irtree_conditional(Arena *arena, EVAL_IRTree *c, EVAL_IRTree *l, EVAL_IRTree *r){
  EVAL_IRTree *result = push_array(arena, EVAL_IRTree, 1);
  result->op = RADDBG_EvalOp_Cond;
  result->children[0] = c;
  result->children[1] = l;
  result->children[2] = r;
  return(result);
}

internal EVAL_IRTree*
eval_irtree_bytecode_no_copy(Arena *arena, String8 bytecode){
  EVAL_IRTree *result = push_array(arena, EVAL_IRTree, 1);
  result->op = EVAL_IRExtKind_Bytecode;
  result->bytecode = bytecode;
  return(result);
}

////////////////////////////////
//~ allen: EVAL IR-Tree High Level Helpers

internal EVAL_IRTree*
eval_irtree_mem_read_type(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, EVAL_IRTree *c, TG_Key type_key){
  U64 byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, type_key);
  EVAL_IRTree *result = &eval_irtree_nil;
  if (0 < byte_size && byte_size <= 8){
    // build the read node
    EVAL_IRTree *read_node = push_array(arena, EVAL_IRTree, 1);
    read_node->op = RADDBG_EvalOp_MemRead;
    read_node->p = byte_size;
    read_node->children[0] = c;
    
    // build a signed trunc node if needed
    U64 bit_size = byte_size << 3;
    EVAL_IRTree *with_trunc = read_node;
    TG_Kind kind = tg_kind_from_key(type_key);
    if (bit_size < 64 && eval_kind_is_signed(kind)){
      with_trunc = push_array(arena, EVAL_IRTree, 1);
      with_trunc->op = RADDBG_EvalOp_TruncSigned;
      with_trunc->p = bit_size;
      with_trunc->children[0] = read_node;
    }
    
    // set result
    result = with_trunc;
  }
  else{
    // TODO: unexpected path
  }
  return(result);
}

internal EVAL_IRTree*
eval_irtree_convert_lo(Arena *arena, EVAL_IRTree *c, RADDBG_EvalTypeGroup out, RADDBG_EvalTypeGroup in){
  EVAL_IRTree *result = push_array(arena, EVAL_IRTree, 1);
  result->op = RADDBG_EvalOp_Convert;
  result->p = in | (out << 8);
  result->children[0] = c;
  return(result);
}

internal EVAL_IRTree*
eval_irtree_trunc(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, EVAL_IRTree *c, TG_Key type_key){
  EVAL_IRTree *result = c;
  U64 byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, type_key);
  if (byte_size < 64){
    RADDBG_EvalOp op = RADDBG_EvalOp_Trunc;
    TG_Kind kind = tg_kind_from_key(type_key);
    if (eval_kind_is_signed(kind)){
      op = RADDBG_EvalOp_TruncSigned;
    }
    U64 bit_size = byte_size << 3;
    result = push_array(arena, EVAL_IRTree, 1);
    result->op = op;
    result->p = bit_size;
    result->children[0] = c;
  }
  return(result);
}

internal EVAL_IRTree*
eval_irtree_convert_hi(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, EVAL_IRTree *c, TG_Key out, TG_Key in){
  EVAL_IRTree *result = c;
  TG_Kind in_kind = tg_kind_from_key(in);
  TG_Kind out_kind = tg_kind_from_key(out);
  U8 in_group  = eval_type_group_from_kind(in_kind);
  U8 out_group = eval_type_group_from_kind(out_kind);
  U32 conversion_rule = raddbg_eval_conversion_rule(in_group, out_group);
  if(conversion_rule == RADDBG_EvalConversionKind_Legal)
  {
    result = eval_irtree_convert_lo(arena, result, out_group, in_group);
  }
  U64 in_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, in);
  U64 out_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, out);
  if(out_byte_size < in_byte_size && eval_kind_is_integer(out_kind))
  {
    result = eval_irtree_trunc(arena, graph, rdbg, result, out);
  }
  return(result);
}

internal EVAL_IRTree*
eval_irtree_resolve_to_value(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, EVAL_EvalMode from_mode,
                             EVAL_IRTree *tree, TG_Key type_key){
  EVAL_IRTree *result = tree;
  switch (from_mode){
    default:{}break;
    case EVAL_EvalMode_Addr:
    {
      result = eval_irtree_mem_read_type(arena, graph, rdbg, tree, type_key);
    }break;
    case EVAL_EvalMode_Reg:
    {
      result = eval_irtree_unary_op(arena, RADDBG_EvalOp_RegReadDyn, RADDBG_EvalTypeGroup_U, tree);
    }break;
  }
  return(result);
}

////////////////////////////////
//~ allen: EVAL Compiler Phases

internal TG_Key
eval_type_from_type_expr(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, EVAL_Expr *expr, EVAL_ErrorList *eout){
  TG_Key result = zero_struct;
  
  EVAL_ExprKind kind = expr->kind;
  switch (kind){
    default:
    {
      eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "Expected type expression.");
    }break;
    
    case EVAL_ExprKind_TypeIdent:
    {
      result = expr->type_key;
    }break;
    
    case EVAL_ExprKind_Ptr:
    {
      TG_Key direct_type_key = eval_type_from_type_expr(arena, graph, rdbg, expr->children[0], eout);
      result = tg_cons_type_make(graph, TG_Kind_Ptr, direct_type_key, 0);
    }break;
    
    case EVAL_ExprKind_Array:
    {
      EVAL_Expr *child_expr = expr->child_and_constant.child;
      TG_Key direct_type_key = eval_type_from_type_expr(arena, graph, rdbg, child_expr, eout);
      result = tg_cons_type_make(graph, TG_Kind_Array, direct_type_key, expr->child_and_constant.u64);
    }break;
    
    case EVAL_ExprKind_Func:
    {
      // TODO(rjf): old type graph code is below:
#if 0
      TG_Type *ret_type = eval_type_from_type_expr(arena, graph, expr->children[0], eout);
      // TODO(allen): decision: do we do the extra work to preserve full function type info?
      result = tg_type_func(graph, ret_type, 0, 0);
#endif
    }break;
  }
  
  return(result);
}

internal EVAL_IRTreeAndType
eval_irtree_and_type_from_expr(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, EVAL_Expr *expr, EVAL_ErrorList *eout)
{
  EVAL_IRTreeAndType result = {0};
  result.tree = &eval_irtree_nil;
  
  EVAL_ExprKind kind = expr->kind;
  switch(kind)
  {
    default:
    {
      eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "(internal) Undefined expression kind (%u).", kind);
    }break;
    
    case EVAL_ExprKind_ArrayIndex:
    {
      EVAL_Expr *exprl = expr->children[0];
      EVAL_Expr *exprr = expr->children[1];
      
      EVAL_IRTreeAndType l = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprl, eout);
      EVAL_IRTreeAndType r = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprr, eout);
      
      if (l.tree->op != 0 && r.tree->op != 0){      
        TG_Key l_restype = eval_type_unwrap(graph, rdbg, l.type_key);
        TG_Key r_restype = eval_type_unwrap(graph, rdbg, r.type_key);
        TG_Kind l_restype_kind = tg_kind_from_key(l_restype);
        TG_Kind r_restype_kind = tg_kind_from_key(r_restype);
        
        // analyze situation
        B32 can_generate = 0;
        B32 l_resolve = 0;
        TG_Key direct_type = zero_struct;
        U64 direct_type_size = 0;
        if (!eval_kind_is_integer(r_restype_kind)){
          eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprr->location, "Cannot index with this type.");
        }
        else{
          direct_type = tg_direct_from_graph_raddbg_key(graph, rdbg, l_restype);
          direct_type_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, direct_type);
          if (l_restype_kind == TG_Kind_Ptr){
            if (direct_type_size == 0){
              eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprr->location, "Cannot index into pointers of zero-sized types.");
            }
            else{
              can_generate = 1;
              if (l.mode != EVAL_EvalMode_Value){
                l_resolve = 1;
              }
            }
          }
          else if (l_restype_kind == TG_Kind_Array){
            if (l.mode == EVAL_EvalMode_Addr){
              if (direct_type_size == 0){
                eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprl->location, "Cannot index into arrays of zero-sized types.");
              }
              else{
                can_generate = 1;
              }
            }
            else{
              eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprl->location, "(Not supported) Cannot index into array without base address.");
            }
          }
          else{
            eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprl->location, "Cannot index into this type.");
          }
        }
        
        // generate ir tree
        if (can_generate){
          // how to compute the index
          EVAL_IRTree *index_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, r.mode, r.tree, r_restype);
          if (direct_type_size > 1){
            EVAL_IRTree *const_tree = eval_irtree_const_u(arena, direct_type_size);
            index_tree = eval_irtree_binary_op_u(arena, RADDBG_EvalOp_Mul, index_tree, const_tree);
          }
          
          // how to compute the base address
          EVAL_IRTree *base_tree = l.tree;
          if (l_resolve){
            base_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, l.mode, base_tree, l_restype);
          }
          
          // how to compute the final address
          EVAL_IRTree *new_tree = eval_irtree_binary_op_u(arena, RADDBG_EvalOp_Add, index_tree, base_tree);
          
          // fill result
          result.tree     = new_tree;
          result.type_key = direct_type;
          result.mode     = EVAL_EvalMode_Addr;
        }
      }
    }break;
    
    case EVAL_ExprKind_MemberAccess:
    {
      EVAL_Expr *exprl = expr->children[0];
      EVAL_Expr *exprr = expr->children[1];
      
      EVAL_IRTreeAndType l = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprl, eout);
      
      if (l.tree->op != 0 && !tg_key_match(tg_key_zero(), l.type_key)){
        TG_Key l_restype = eval_type_unwrap(graph, rdbg, l.type_key);
        TG_Kind l_restype_kind = tg_kind_from_key(l_restype);
        
        // determine which type to use
        TG_Key check_type_key = l_restype;
        TG_Kind check_type_kind = l_restype_kind;
        if (l_restype_kind == TG_Kind_Ptr || l_restype_kind == TG_Kind_LRef || l_restype_kind == TG_Kind_RRef){
          check_type_key = tg_direct_from_graph_raddbg_key(graph, rdbg, l_restype);
          check_type_kind = tg_kind_from_key(check_type_key);
        }
        
        // switch to handle
        switch(check_type_kind)
        {
          default:
          {
            eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprl->location, "Cannot perform member access on this type.");
          }break;
          
          case TG_Kind_Struct:
          case TG_Kind_Class:
          case TG_Kind_Union:
          {
            // analyze situation
            B32 can_generate = 0;
            TG_Key r_type = zero_struct;
            U32 r_off = 0;
            B32 l_resolve = 0;
            
            // determine how to treat left
            B32 l_good = 0;
            if (l_restype_kind == TG_Kind_Ptr || l_restype_kind == TG_Kind_LRef || l_restype_kind == TG_Kind_RRef){
              l_good = 1;
              l_resolve = 1;
            }
            else{
              if (l.mode == EVAL_EvalMode_Addr){
                l_good = 1;
              }
              else if(l.mode == EVAL_EvalMode_Reg)
              {
                l_good = 1;
              }
              else{
                eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprl->location,
                            "(Not supported) Cannot access member without a base address or register location.");
              }
            }
            
            // right must be identifier
            B32 r_good = 0;
            if (exprr->kind == EVAL_ExprKind_LeafMember){
              r_good = 1;
            }
            else{
              eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprr->location,
                          "(internal) Expected a leaf member in member access.");
            }
            
            if (l_good && r_good){
              Temp scratch = scratch_begin(&arena, 1);
              TG_MemberArray check_type_members = tg_data_members_from_graph_raddbg_key(scratch.arena, graph, rdbg, check_type_key);
              
              // lookup member
              String8 member_name = exprr->name;
              TG_Member *match = 0;
              for(U64 member_idx = 0; member_idx < check_type_members.count; member_idx += 1)
              {
                TG_Member *member = &check_type_members.v[member_idx];
                if(str8_match(member->name, member_name, 0))
                {
                  match = member;
                  break;
                }
              }
              
              // extract member info
              if (match != 0){
                can_generate = 1;
                r_type = match->type_key;
                r_off = match->off;
              }
              else{
                eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprr->location, "Could not find a member named '%S' in type.", member_name);
              }
              
              scratch_end(scratch);
            }
            
            // generate ir tree
            if (can_generate){
              EVAL_IRTree *new_tree = l.tree;
              if (l_resolve){
                new_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, l.mode, new_tree, l_restype);
              }
              if (r_off != 0){
                EVAL_IRTree *const_tree = eval_irtree_const_u(arena, r_off);
                new_tree = eval_irtree_binary_op_u(arena, RADDBG_EvalOp_Add, new_tree, const_tree);
              }
              
              // fill result
              result.tree     = new_tree;
              result.type_key = r_type;
              result.mode     = l.mode;
            }
          }break;
        }
        
      }
    }break;
    
    case EVAL_ExprKind_Deref:
    {
      EVAL_Expr *exprc = expr->children[0];
      
      EVAL_IRTreeAndType c = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprc, eout);
      
      if (c.tree->op != 0){
        TG_Key c_restype = eval_type_unwrap(graph, rdbg, c.type_key);
        TG_Kind c_restype_kind = tg_kind_from_key(c_restype);
        TG_Key c_restype_direct = tg_direct_from_graph_raddbg_key(graph, rdbg, c_restype);
        U64 c_restype_direct_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, c_restype_direct);
        
        // analyze situation
        B32 can_generate = 0;
        B32 c_resolve = 0;
        if (c_restype_kind == TG_Kind_Ptr){
          if (c_restype_direct_size == 0){
            eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprc->location, "Cannot dereference pointers of zero-sized types.");
          }
          else{
            can_generate = 1;
            c_resolve = 1;
          }
        }
        else if (c_restype_kind == TG_Kind_Array){
          if (c.mode == EVAL_EvalMode_Addr){
            if (c_restype_direct_size == 0){
              eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprc->location, "Cannot dereference arrays of zero-sized types.");
            }
            else{
              can_generate = 1;
            }
          }
          else{
            eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprc->location, "(Not supported) Cannot dereference array without base address.");
          }
        }
        else{
          eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, exprc->location, "Cannot dereference this type.");
        }
        
        // generate ir tree
        if (can_generate){
          EVAL_IRTree *new_tree = c.tree;
          if (c_resolve){
            new_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, c.mode, c.tree, c_restype);
          }
          
          // fill result
          result.tree     = new_tree;
          result.type_key = c_restype_direct;
          result.mode     = EVAL_EvalMode_Addr;
        }
      }
    }break;
    
    case EVAL_ExprKind_Address:
    {
      EVAL_Expr *exprc = expr->children[0];
      EVAL_IRTreeAndType c = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprc, eout);
      if(c.tree->op != 0 && !tg_key_match(c.type_key, tg_key_zero()))
      {
        TG_Key c_restype = eval_type_unwrap(graph, rdbg, c.type_key);
        TG_Kind c_restype_kind = tg_kind_from_key(c_restype);
        
        // analyze situation
        B32 can_generate = 0;
        if(c.mode == EVAL_EvalMode_Addr)
        {
          can_generate = 1;
        }
        
        // generate ir tree
        if(can_generate)
        {
          EVAL_IRTree *new_tree = c.tree;
          TG_Key ptr_type = tg_cons_type_make(graph, TG_Kind_Ptr, c_restype, 0);
          
          // fill result
          result.tree     = new_tree;
          result.type_key = ptr_type;
          result.mode     = EVAL_EvalMode_Value;
        }
      }
    }break;
    
    case EVAL_ExprKind_Cast:
    {
      EVAL_Expr *exprl = expr->children[0];
      EVAL_Expr *exprr = expr->children[1];
      
      TG_Key cast_type_key = eval_type_from_type_expr(arena, graph, rdbg, exprl, eout);
      TG_Kind cast_type_kind = tg_kind_from_key(cast_type_key);
      EVAL_IRTreeAndType c = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprr, eout);
      
      if(cast_type_kind != TG_Kind_Null && c.tree->op != 0)
      {
        TG_Key c_restype = eval_type_unwrap(graph, rdbg, c.type_key);
        TG_Kind c_restype_kind = tg_kind_from_key(c_restype);
        U64 c_restype_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, c_restype);
        U64 cast_type_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, cast_type_key);
        
        // analyze situation
        U8 in_group  = eval_type_group_from_kind(c_restype_kind);
        U8 out_group = eval_type_group_from_kind(cast_type_kind);
        RADDBG_EvalConversionKind conversion_rule = raddbg_eval_conversion_rule(in_group, out_group);
        
        // generate tree
        switch(conversion_rule)
        {
          case RADDBG_EvalConversionKind_Noop:
          case RADDBG_EvalConversionKind_Legal:
          {
            EVAL_IRTree *in_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, c.mode, c.tree, c_restype);
            
            EVAL_IRTree *new_tree = in_tree;
            if (conversion_rule == RADDBG_EvalConversionKind_Legal){
              new_tree = eval_irtree_convert_lo(arena, in_tree, out_group, in_group);
            }
            if (cast_type_byte_size < c_restype_byte_size && eval_kind_is_integer(cast_type_kind)){
              new_tree = eval_irtree_trunc(arena, graph, rdbg, in_tree, cast_type_key);
            }
            
            result.tree     = new_tree;
            result.type_key = cast_type_key;
            result.mode     = EVAL_EvalMode_Value;
          }break;
          
          default:
          {
            String8 text = str8_lit("(internal) unknown conversion rule");
            if (conversion_rule < RADDBG_EvalConversionKind_COUNT){
              text.str = raddbg_eval_conversion_message(conversion_rule, &text.size);
            }
            eval_error(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, text);
          }break;
        }
      }
    }break;
    
    case EVAL_ExprKind_Sizeof:
    {
      EVAL_Expr *exprc = expr->children[0];
      
      // analyze situation
      TG_Key type_key = zero_struct;
      switch (exprc->kind){
        // size of type expression
        case EVAL_ExprKind_TypeIdent:
        case EVAL_ExprKind_Ptr:
        case EVAL_ExprKind_Array:
        case EVAL_ExprKind_Func:
        {
          type_key = eval_type_from_type_expr(arena, graph, rdbg, exprc, eout);
        }break;
        
        // size of value expression
        default:
        {
          EVAL_IRTreeAndType c = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprc, eout);
          type_key = c.type_key;
        }break;
      }
      B32 can_generate = 0;
      U64 size = 0;
      TG_Kind type_kind = tg_kind_from_key(type_key);
      if (type_kind != TG_Kind_Null){
        can_generate = 1;
        size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, type_key);
      }
      
      // generate ir tree
      if(can_generate)
      {
        EVAL_IRTree *new_tree = eval_irtree_const_u(arena, size);
        result.tree     = new_tree;
        result.type_key = tg_key_basic(TG_Kind_U64);
        result.mode     = EVAL_EvalMode_Value;
      }
    }break;
    
    case EVAL_ExprKind_Neg:
    case EVAL_ExprKind_LogNot:
    case EVAL_ExprKind_BitNot:
    {
      EVAL_Expr *exprc = expr->children[0];
      
      EVAL_IRTreeAndType c = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprc, eout);
      if (c.tree->op != 0){
        TG_Key c_restype = eval_type_unwrap(graph, rdbg, c.type_key);
        TG_Key p_type = eval_type_promote(graph, rdbg, c_restype);
        TG_Kind c_restype_kind = tg_kind_from_key(c_restype);
        
        // analyze situation
        B32 can_generate = 0;
        RADDBG_EvalOp op = eval_opcode_from_expr_kind(kind);
        U8 c_group = eval_type_group_from_kind(c_restype_kind);
        if (!raddbg_eval_opcode_type_compatible(op, c_group)){
          eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "Cannot use this operator on this type.");
        }
        else{
          can_generate = 1;
        }
        
        // generate ir tree
        if (can_generate){
          EVAL_IRTree *in_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, c.mode, c.tree, c_restype);
          in_tree = eval_irtree_convert_hi(arena, graph, rdbg, in_tree, p_type, c_restype);
          
          EVAL_IRTree *new_tree = eval_irtree_unary_op(arena, op, c_group, in_tree);
          
          result.tree     = new_tree;
          result.type_key = p_type;
          result.mode     = EVAL_EvalMode_Value;
        }
      }
    }break;
    
    case EVAL_ExprKind_Mul:
    case EVAL_ExprKind_Div:
    case EVAL_ExprKind_Mod:
    case EVAL_ExprKind_Add:
    case EVAL_ExprKind_Sub:
    case EVAL_ExprKind_LShift:
    case EVAL_ExprKind_RShift:
    case EVAL_ExprKind_Less:
    case EVAL_ExprKind_LsEq:
    case EVAL_ExprKind_Grtr:
    case EVAL_ExprKind_GrEq:
    case EVAL_ExprKind_EqEq:
    case EVAL_ExprKind_NtEq:
    case EVAL_ExprKind_BitAnd:
    case EVAL_ExprKind_BitXor:
    case EVAL_ExprKind_BitOr:
    case EVAL_ExprKind_LogAnd:
    case EVAL_ExprKind_LogOr:
    {
      //- setup & dispatch
      EVAL_Expr *exprl = expr->children[0];
      EVAL_Expr *exprr = expr->children[1];
      
      EVAL_IRTreeAndType l = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprl, eout);
      EVAL_IRTreeAndType r = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprr, eout);
      
      if (l.tree->op != 0 && r.tree->op != 0){
        TG_Key l_restype = eval_type_unwrap(graph, rdbg, l.type_key);
        TG_Key r_restype = eval_type_unwrap(graph, rdbg, r.type_key);
        TG_Kind l_restype_kind = tg_kind_from_key(l_restype);
        TG_Kind r_restype_kind = tg_kind_from_key(r_restype);
        
        //- rjf: decay register types to basics
        if(l_restype.kind == TG_KeyKind_Reg)
        {
          l_restype = tg_key_basic(TG_Kind_U64);
          l_restype_kind = tg_kind_from_key(l_restype);
        }
        if(r_restype.kind == TG_KeyKind_Reg)
        {
          r_restype = tg_key_basic(TG_Kind_U64);
          r_restype_kind = tg_kind_from_key(r_restype);
        }
        
        RADDBG_EvalOp op = eval_opcode_from_expr_kind(kind);
        
        //- pointer decay
        B32 l_is_pointer = (l_restype_kind == TG_Kind_Ptr);
        B32 l_is_decay = (l_restype_kind == TG_Kind_Array && l.mode == EVAL_EvalMode_Addr);
        B32 l_is_pointer_like = (l_is_pointer || l_is_decay);
        
        B32 r_is_pointer = (r_restype_kind == TG_Kind_Ptr);
        B32 r_is_decay = (r_restype_kind == TG_Kind_Array && r.mode == EVAL_EvalMode_Addr);
        B32 r_is_pointer_like = (r_is_pointer || r_is_decay);
        
        //- determine arithmetic path
#define EVAL_ArithPath_Normal 0
#define EVAL_ArithPath_PtrAdd 1
#define EVAL_ArithPath_PtrSub 2
        
        B32 ptr_arithmetic_mul_rptr = 0;
        U32 arith_path = EVAL_ArithPath_Normal;
        if (kind == EVAL_ExprKind_Add){
          if (l_is_pointer_like && eval_kind_is_integer(r_restype_kind)){
            arith_path = EVAL_ArithPath_PtrAdd;
          }
          if (l_is_pointer_like && eval_kind_is_integer(l_restype_kind)){
            arith_path = EVAL_ArithPath_PtrAdd;
            ptr_arithmetic_mul_rptr = 1;
          }
        }
        else if (kind == EVAL_ExprKind_Sub){
          if (l_is_pointer_like && eval_kind_is_integer(r_restype_kind)){
            arith_path = EVAL_ArithPath_PtrAdd;
          }
          if (l_is_pointer_like && r_is_pointer_like){
            TG_Key l_restype_direct = tg_direct_from_graph_raddbg_key(graph, rdbg, l_restype);
            TG_Key r_restype_direct = tg_direct_from_graph_raddbg_key(graph, rdbg, r_restype);
            U64 l_restype_direct_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, l_restype_direct);
            U64 r_restype_direct_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, r_restype_direct);
            if (l_restype_direct_byte_size == r_restype_direct_byte_size){
              arith_path = EVAL_ArithPath_PtrSub;
            }
          }
        }
        
        //- specific arithmetic handlers
        
        switch (arith_path){
          case EVAL_ArithPath_Normal:
          {
            // analyze situation
            B32 is_comparison = eval_expr_kind_is_comparison(kind);
            B32 both_basic = (eval_kind_is_basic_or_enum(l_restype_kind) &&
                              eval_kind_is_basic_or_enum(r_restype_kind));
            
            TG_Key cv_type_key = zero_struct;
            if (both_basic){
              cv_type_key = eval_type_coerce(graph, rdbg, l_restype, r_restype);
            }
            else if (is_comparison && eval_type_match(graph, rdbg, l_restype, r_restype)){
              cv_type_key = l_restype;
            }
            
            TG_Kind cv_type_kind = tg_kind_from_key(cv_type_key);
            U8 cv_group = eval_type_group_from_kind(cv_type_kind);
            
            B32 can_generate = 0;
            if (raddbg_eval_opcode_type_compatible(op, cv_group)){
              can_generate = 1;
            }
            else{
              eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "Cannot use this operator on this type.");
            }
            
            // generate ir tree
            if (can_generate){
              TG_Key final_type_key = cv_type_key;
              if (is_comparison){
                final_type_key = tg_key_basic(TG_Kind_Bool);
              }
              
              EVAL_IRTree *l_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, l.mode, l.tree, l_restype);
              l_tree = eval_irtree_convert_hi(arena, graph, rdbg, l_tree, cv_type_key, l_restype);
              
              EVAL_IRTree *r_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, r.mode, r.tree, r_restype);
              r_tree = eval_irtree_convert_hi(arena, graph, rdbg, r_tree, cv_type_key, r_restype);
              
              EVAL_IRTree *new_tree = eval_irtree_binary_op(arena, op, cv_group, l_tree, r_tree);
              
              result.tree     = new_tree;
              result.type_key = final_type_key;
              result.mode     = EVAL_EvalMode_Value;
            }
          }break;
          
          case EVAL_ArithPath_PtrAdd:
          {
            // setup which side is the pointer
            EVAL_IRTreeAndType *ptr = &l;
            EVAL_IRTreeAndType *integer = &r;
            B32 ptr_is_decay = l_is_decay;
            if (ptr_arithmetic_mul_rptr){
              ptr = &r;
              integer = &l;
              ptr_is_decay = r_is_decay;
            }
            
            TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, ptr->type_key);
            U64 direct_type_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, direct);
            
            // generate ir tree
            EVAL_IRTree *ptr_tree = ptr->tree;
            if (!ptr_is_decay){
              ptr_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, ptr->mode, ptr_tree, ptr->type_key);
            }
            
            EVAL_IRTree *integer_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, integer->mode, integer->tree, integer->type_key);
            if (direct_type_size > 1){
              EVAL_IRTree *const_tree = eval_irtree_const_u(arena, direct_type_size);
              integer_tree = eval_irtree_binary_op_u(arena, RADDBG_EvalOp_Mul, integer_tree, const_tree);
            }
            
            TG_Key ptr_type = ptr->type_key;
            if (ptr_is_decay){
              ptr_type = tg_cons_type_make(graph, TG_Kind_Ptr, direct, 0);
            }
            
            EVAL_IRTree *new_tree = eval_irtree_binary_op(arena, op, RADDBG_EvalTypeGroup_U, ptr_tree, integer_tree);
            
            result.tree     = new_tree;
            result.type_key = ptr_type;
            result.mode     = EVAL_EvalMode_Value;
          }break;
          
          case EVAL_ArithPath_PtrSub:
          {
            TG_Key direct = tg_direct_from_graph_raddbg_key(graph, rdbg, l_restype);
            U64 direct_type_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, direct);
            
            // generate ir tree
            EVAL_IRTree *l_tree = l.tree;
            if (!l_is_decay){
              l_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, l.mode, l.tree, l_restype);
            }
            
            EVAL_IRTree *r_tree = r.tree;
            if (!r_is_decay){
              r_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, r.mode, r.tree, r_restype);
            }
            
            EVAL_IRTree *op_tree = eval_irtree_binary_op(arena, op, RADDBG_EvalTypeGroup_U, l_tree, r_tree);
            
            EVAL_IRTree *new_tree = op_tree;
            if (direct_type_size > 1){
              EVAL_IRTree *const_tree = eval_irtree_const_u(arena, direct_type_size);
              new_tree = eval_irtree_binary_op(arena, RADDBG_EvalOp_Div, RADDBG_EvalTypeGroup_U, new_tree, const_tree);
            }
            
            result.tree = new_tree;
            result.type_key = tg_key_basic(TG_Kind_U64);
            result.mode = EVAL_EvalMode_Value;
          }break;
        }
      }
    }break;
    
    case EVAL_ExprKind_Ternary:
    {
      EVAL_Expr *exprc = expr->children[0];
      EVAL_Expr *exprl = expr->children[1];
      EVAL_Expr *exprr = expr->children[2];
      
      EVAL_IRTreeAndType c = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprc, eout);
      EVAL_IRTreeAndType l = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprl, eout);
      EVAL_IRTreeAndType r = eval_irtree_and_type_from_expr(arena, graph, rdbg, exprr, eout);
      
      if (l.tree->op != 0 && r.tree->op != 0 && c.tree->op != 0){
        
        TG_Key c_restype = eval_type_unwrap(graph, rdbg, c.type_key);
        TG_Key l_restype = eval_type_unwrap(graph, rdbg, l.type_key);
        TG_Key r_restype = eval_type_unwrap(graph, rdbg, r.type_key);
        TG_Kind c_restype_kind = tg_kind_from_key(c_restype);
        TG_Kind l_restype_kind = tg_kind_from_key(l_restype);
        TG_Kind r_restype_kind = tg_kind_from_key(r_restype);
        
        // analyze situation
        B32 can_generate = 0;
        TG_Key final_type = zero_struct;
        if (eval_kind_is_integer(c_restype_kind)){
          eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "Conditional term must be an integer type.");
        }
        else{
          if (eval_kind_is_basic_or_enum(l_restype_kind) &&
              eval_kind_is_basic_or_enum(r_restype_kind)){
            can_generate = 1;
            final_type = eval_type_coerce(graph, rdbg, l_restype, r_restype);
          }
          else{
            if (eval_type_match(graph, rdbg, l_restype, r_restype)){
              if (l_restype_kind == TG_Kind_Ptr){
                can_generate = 1;
                final_type = l_restype;
              }
              else{
                eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "(Not supported) Conditional value not basic type or pointer.");
              }
            }
            else{
              eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "Left and right terms must have matching types.");
            }
          }
        }
        
        // generate ir tree
        if (can_generate){
          EVAL_IRTree *c_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, c.mode, c.tree, c_restype);
          
          EVAL_IRTree *l_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, l.mode, l.tree, l_restype);
          l_tree = eval_irtree_convert_hi(arena, graph, rdbg, l_tree, final_type, l_restype);
          
          EVAL_IRTree *r_tree = eval_irtree_resolve_to_value(arena, graph, rdbg, r.mode, r.tree, r_restype);
          r_tree = eval_irtree_convert_hi(arena, graph, rdbg, r_tree, final_type, r_restype);
          
          EVAL_IRTree *new_tree = eval_irtree_conditional(arena, c_tree, l_tree, r_tree);
          
          result.tree = new_tree;
          result.type_key = final_type;
          result.mode = EVAL_EvalMode_Value;
        }
      }
    }break;
    
    case EVAL_ExprKind_LeafBytecode:
    {
      EVAL_IRTree *new_tree = eval_irtree_bytecode_no_copy(arena, expr->bytecode);
      TG_Key final_type_key = expr->type_key;
      EVAL_EvalMode mode = expr->mode;
      
      result.tree     = new_tree;
      result.type_key = final_type_key;
      result.mode     = mode;
    }break;
    
    case EVAL_ExprKind_LeafMember:
    {
      eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "(internal) Leaf member not expected here.");
    }break;
    
    case EVAL_ExprKind_LeafU64:
    {
      U64 val = expr->u64;
      EVAL_IRTree *new_tree = eval_irtree_const_u(arena, val);
      
      TG_Key type_key = zero_struct;
      if (val <= max_S32){
        type_key = tg_key_basic(TG_Kind_S32);
      }
      else if (val <= max_S64){
        type_key = tg_key_basic(TG_Kind_S64);
      }
      else{
        type_key = tg_key_basic(TG_Kind_U64);
      }
      
      result.tree     = new_tree;
      result.type_key = type_key;
      result.mode     = EVAL_EvalMode_Value;
    }break;
    
    case EVAL_ExprKind_LeafF64:
    {
      U64 val = expr->u64;
      EVAL_IRTree *new_tree = eval_irtree_const_u(arena, val);
      
      result.tree     = new_tree;
      result.type_key = tg_key_basic(TG_Kind_F64);
      result.mode     = EVAL_EvalMode_Value;
    }break;
    
    case EVAL_ExprKind_LeafF32:
    {
      U32 val = expr->u32;
      EVAL_IRTree *new_tree = eval_irtree_const_u(arena, val);
      
      result.tree     = new_tree;
      result.type_key = tg_key_basic(TG_Kind_F32);
      result.mode     = EVAL_EvalMode_Value;
    }break;
    
    case EVAL_ExprKind_TypeIdent:
    case EVAL_ExprKind_Ptr:
    case EVAL_ExprKind_Array:
    case EVAL_ExprKind_Func:
    {
      eval_errorf(arena, eout, EVAL_ErrorKind_MalformedInput, expr->location, "Type expression not expected.");
    }break;
  }
  
  return(result);
}

internal void
eval_oplist_from_irtree(Arena *arena, EVAL_IRTree *tree, EVAL_OpList *out){
  U32 op = tree->op;
  switch (op){
    case RADDBG_EvalOp_Stop:
    case RADDBG_EvalOp_Skip:
    {
      // TODO: error - invalid ir-tree op
    }break;
    
    case EVAL_IRExtKind_Bytecode:
    {
      eval_oplist_push_bytecode(arena, out, tree->bytecode);
    }break;
    
    case RADDBG_EvalOp_Cond:
    {
      // split out each of the children
      EVAL_OpList prt_cond = {0};
      eval_oplist_from_irtree(arena, tree->children[0], &prt_cond);
      
      EVAL_OpList prt_left = {0};
      eval_oplist_from_irtree(arena, tree->children[1], &prt_left);
      
      EVAL_OpList prt_right = {0};
      eval_oplist_from_irtree(arena, tree->children[2], &prt_right);
      
      // put together like so:
      //  1. <prt_cond> , Op_Cond (sizeof(2))
      //  2. <prt_right>, Op_Skip (sizeof(3))
      //  3. <ptr_left>
      //  4.
      
      // modify prt_right in place to create step 2
      eval_oplist_push_op(arena, &prt_right, RADDBG_EvalOp_Skip, prt_left.encoded_size);
      
      // merge 1 into out
      eval_oplist_concat_in_place(out, &prt_cond);
      eval_oplist_push_op(arena, out, RADDBG_EvalOp_Cond, prt_right.encoded_size);
      
      // merge 2 into out
      eval_oplist_concat_in_place(out, &prt_right);
      
      // merge 3 into out
      eval_oplist_concat_in_place(out, &prt_left);
    }break;
    
    default:
    {
      if (op >= RADDBG_EvalOp_COUNT){
        // TODO: error - invalid ir-tree op
      }
      else{
        // handle all children
        U8 ctrlbits = raddbg_eval_opcode_ctrlbits[op];
        U64 child_count = RADDBG_POPN_FROM_CTRLBITS(ctrlbits);
        EVAL_IRTree**child = tree->children;
        for (U64 i = 0; i < child_count; i += 1, child += 1){
          eval_oplist_from_irtree(arena, *child, out);
        }
        
        // emit op to compute this node
        eval_oplist_push_op(arena, out, (RADDBG_EvalOp)tree->op, tree->p);
      }
    }break;
  }
}



