// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Bundled Evaluation Functions

#if 0
internal E_Eval
e_eval_from_expr(Arena *arena, E_Expr *expr)
{
  ProfBeginFunction();
  E_IRTreeAndType     irtree   = e_push_irtree_and_type_from_expr(arena, expr);
  E_OpList            oplist   = e_oplist_from_irtree(arena, irtree.root);
  String8             bytecode = e_bytecode_from_oplist(arena, &oplist);
  E_Interpretation    interp   = e_interpret(bytecode);
  E_Eval eval =
  {
    .value           = interp.value,
    .space           = interp.space,
    .expr            = expr,
    .irtree          = irtree,
    .bytecode        = bytecode,
    .code            = interp.code,
  };
  e_msg_list_concat_in_place(&eval.msgs, &irtree.msgs);
  if(E_InterpretationCode_Good < eval.code && eval.code < E_InterpretationCode_COUNT)
  {
    e_msg(arena, &eval.msgs, E_MsgKind_InterpretationError, 0, e_interpretation_code_display_strings[eval.code]);
  }
  ProfEnd();
  return eval;
}

internal E_Eval
e_eval_from_string(Arena *arena, String8 string)
{
  ProfBeginFunction();
  ProfBegin("e_eval_from_string (%.*s)", str8_varg(string));
  E_Parse             parse    = e_parse_from_string(string);
  E_IRTreeAndType     irtree   = e_irtree_and_type_from_expr(parse.expr);
  E_OpList            oplist   = e_oplist_from_irtree(arena, irtree.root);
  String8             bytecode = e_bytecode_from_oplist(arena, &oplist);
  E_Interpretation    interp   = e_interpret(bytecode);
  E_Eval eval =
  {
    .value           = interp.value,
    .space           = interp.space,
    .expr            = parse.expr,
    .irtree          = irtree,
    .bytecode        = bytecode,
    .code            = interp.code,
  };
  e_msg_list_concat_in_place(&eval.msgs, &irtree.msgs);
  if(E_InterpretationCode_Good < eval.code && eval.code < E_InterpretationCode_COUNT)
  {
    e_msg(arena, &eval.msgs, E_MsgKind_InterpretationError, 0, e_interpretation_code_display_strings[eval.code]);
  }
  ProfEnd();
  ProfEnd();
  return eval;
}

internal E_Eval
e_eval_from_stringf(Arena *arena, char *fmt, ...)
{
  Temp scratch = scratch_begin(&arena, 1);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  E_Eval eval = e_eval_from_string(arena, string);
  va_end(args);
  scratch_end(scratch);
  return eval;
}

internal E_Eval
e_dynamically_typed_eval_from_eval(E_Eval eval)
{
  E_TypeKey type_key = eval.irtree.type_key;
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  if(e_type_state != 0 &&
     e_interpret_ctx != 0 &&
     e_interpret_ctx->space_read != 0 &&
     e_interpret_ctx->module_base != 0 &&
     type_kind == E_TypeKind_Ptr)
  {
    Temp scratch = scratch_begin(0, 0);
    E_TypeKey ptee_type_key = e_type_key_unwrap(type_key, E_TypeUnwrapFlag_AllDecorative);
    E_TypeKind ptee_type_kind = e_type_kind_from_key(ptee_type_key);
    if(ptee_type_kind == E_TypeKind_Struct || ptee_type_kind == E_TypeKind_Class)
    {
      E_Type *ptee_type = e_type_from_key__cached(ptee_type_key);
      B32 has_vtable = 0;
      for(U64 idx = 0; idx < ptee_type->count; idx += 1)
      {
        if(ptee_type->members[idx].kind == E_MemberKind_VirtualMethod)
        {
          has_vtable = 1;
          break;
        }
      }
      if(has_vtable)
      {
        U64 ptr_vaddr = eval.value.u64;
        U64 addr_size = e_type_byte_size_from_key(e_type_key_unwrap(type_key, E_TypeUnwrapFlag_AllDecorative));
        U64 class_base_vaddr = 0;
        U64 vtable_vaddr = 0;
        if(e_space_read(eval.space, &class_base_vaddr, r1u64(ptr_vaddr, ptr_vaddr+addr_size)) &&
           e_space_read(eval.space, &vtable_vaddr, r1u64(class_base_vaddr, class_base_vaddr+addr_size)))
        {
          Arch arch = e_base_ctx->primary_module->arch;
          U32 rdi_idx = 0;
          RDI_Parsed *rdi = 0;
          U64 module_base = 0;
          for(U64 idx = 0; idx < e_base_ctx->modules_count; idx += 1)
          {
            if(contains_1u64(e_base_ctx->modules[idx].vaddr_range, vtable_vaddr))
            {
              arch = e_base_ctx->modules[idx].arch;
              rdi_idx = (U32)idx;
              rdi = e_base_ctx->modules[idx].rdi;
              module_base = e_base_ctx->modules[idx].vaddr_range.min;
              break;
            }
          }
          if(rdi != 0)
          {
            U64 vtable_voff = vtable_vaddr - module_base;
            U64 global_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_GlobalVMap, vtable_voff);
            RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, global_idx);
            if(global_var->link_flags & RDI_LinkFlag_TypeScoped)
            {
              RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, global_var->container_idx);
              RDI_TypeNode *type = rdi_element_from_name_idx(rdi, TypeNodes, udt->self_type_idx);
              E_TypeKey derived_type_key = e_type_key_ext(e_type_kind_from_rdi(type->kind), udt->self_type_idx, rdi_idx);
              E_TypeKey ptr_to_derived_type_key = e_type_key_cons_ptr(arch, derived_type_key, 1, 0);
              eval.irtree.type_key = ptr_to_derived_type_key;
            }
          }
        }
      }
    }
    scratch_end(scratch);
  }
  return eval;
}

internal E_Value
e_value_from_string(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  E_Eval eval = e_eval_from_string(scratch.arena, string);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  E_Value result = value_eval.value;
  scratch_end(scratch);
  return result;
}

internal E_Value
e_value_from_stringf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  E_Value result = e_value_from_string(string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal E_Value
e_value_from_expr(E_Expr *expr)
{
  Temp scratch = scratch_begin(0, 0);
  E_Eval eval = e_eval_from_expr(scratch.arena, expr);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  E_Value result = value_eval.value;
  scratch_end(scratch);
  return result;
}

internal E_Value
e_value_from_eval(E_Eval eval)
{
  E_Eval value_eval = e_value_eval_from_eval(eval);
  E_Value result = value_eval.value;
  return result;
}

internal E_Eval
e_eval_wrap(Arena *arena, E_Eval eval, String8 string)
{
  E_IRTreeAndType *prev_overridden_irtree = e_ir_state->overridden_irtree;
  e_ir_state->overridden_irtree = &eval.irtree;
  E_Eval wrapped_eval = e_eval_from_string(arena, string);
  e_ir_state->overridden_irtree = prev_overridden_irtree;
  return wrapped_eval;
}

internal E_Eval
e_eval_wrapf(Arena *arena, E_Eval eval, char *fmt, ...)
{
  Temp scratch = scratch_begin(&arena, 1);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  E_Eval result = e_eval_wrap(arena, eval, string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

#endif

internal U64
e_base_offset_from_eval(E_Eval eval)
{
  if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(e_type_key_unwrap(eval.irtree.type_key, E_TypeUnwrapFlag_AllDecorative))))
  {
    eval = e_value_eval_from_eval(eval);
  }
  return eval.value.u64;
}

internal Rng1U64
e_range_from_eval(E_Eval eval)
{
  U64 size = 0;
  E_Type *type = e_type_from_key__cached(eval.irtree.type_key);
  if(type->kind == E_TypeKind_Lens)
  {
    for EachIndex(idx, type->count)
    {
      E_Expr *arg = type->args[idx];
      if(arg->kind == E_ExprKind_Define && str8_match(arg->first->string, str8_lit("size"), 0))
      {
        size = e_value_from_expr(arg->first->next).u64;
        break;
      }
    }
  }
  E_TypeKey type_key = e_type_key_unwrap(eval.irtree.type_key, E_TypeUnwrapFlag_AllDecorative);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_key_unwrap(type_key, E_TypeUnwrapFlag_All);
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  if(size == 0 && e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                               direct_type_kind == E_TypeKind_Union ||
                                                               direct_type_kind == E_TypeKind_Class ||
                                                               direct_type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(direct_type_key);
  }
  if(size == 0 && eval.irtree.mode == E_Mode_Offset && (type_kind == E_TypeKind_Struct ||
                                                        type_kind == E_TypeKind_Union ||
                                                        type_kind == E_TypeKind_Class ||
                                                        type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(type_key);
  }
  if(size == 0)
  {
    size = KB(16);
  }
  Rng1U64 result = {0};
  result.min = e_base_offset_from_eval(eval);
  result.max = result.min + size;
  return result;
}


////////////////////////////////
//~ rjf: Debug Logging Functions

internal String8
e_debug_log_from_expr_string(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  char *indent_spaces = "                                                                                                                                ";
  String8List strings = {0};
  
  //- rjf: begin expression
  String8 expr_text = string;
  str8_list_pushf(scratch.arena, &strings, "`%S`\n", expr_text);
  
  //- rjf: parse
  E_Parse parse = e_push_parse_from_string(scratch.arena, expr_text);
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E_Expr *expr;
      S32 indent;
    };
    E_TokenArray tokens = parse.tokens;
    str8_list_pushf(scratch.arena, &strings, "    tokens:\n");
    for EachIndex(idx, tokens.count)
    {
      E_Token token = tokens.v[idx];
      String8 token_string = str8_substr(expr_text, token.range);
      str8_list_pushf(scratch.arena, &strings, "        %S: `%S`\n", e_token_kind_strings[token.kind], token_string);
    }
    str8_list_pushf(scratch.arena, &strings, "    expr:\n");
    Task start_task = {0, parse.expr, 2};
    Task *first_task = &start_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      E_Expr *expr = t->expr;
      str8_list_pushf(scratch.arena, &strings, "%.*s%S", (int)t->indent*4, indent_spaces, e_expr_kind_strings[expr->kind]);
      switch(expr->kind)
      {
        default:{}break;
        case E_ExprKind_LeafU64:
        {
          str8_list_pushf(scratch.arena, &strings, " (%I64u)", expr->value.u64);
        }break;
        case E_ExprKind_LeafIdentifier:
        {
          str8_list_pushf(scratch.arena, &strings, " (`%S`)", expr->string);
        }break;
      }
      str8_list_pushf(scratch.arena, &strings, "\n");
      Task *last_task = t;
      for(E_Expr *child = expr->first; child != &e_expr_nil; child = child->next)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->next = last_task->next;
        last_task->next = task;
        task->expr = child;
        task->indent = t->indent+1;
        last_task = task;
      }
    }
  }
  
  //- rjf: type
  E_IRTreeAndType irtree = e_push_irtree_and_type_from_expr(scratch.arena, 0, 0, 0, parse.expr);
  {
    str8_list_pushf(scratch.arena, &strings, "    type:\n");
    S32 indent = 2;
    for(E_TypeKey type_key = irtree.type_key;
        !e_type_key_match(e_type_key_zero(), type_key);
        type_key = e_type_key_direct(type_key),
        indent += 1)
    {
      E_Type *type = e_type_from_key(scratch.arena, type_key);
      str8_list_pushf(scratch.arena, &strings, "%.*s%S\n", (int)indent*4, indent_spaces, e_type_kind_basic_string_table[type->kind]);
    }
  }
  
  //- rjf: irtree
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E_IRNode *irnode;
      S32 indent;
    };
    str8_list_pushf(scratch.arena, &strings, "    ir_tree:\n");
    Task start_task = {0, irtree.root, 2};
    Task *first_task = &start_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      E_IRNode *irnode = t->irnode;
      str8_list_pushf(scratch.arena, &strings, "%.*s", (int)t->indent*4, indent_spaces);
      switch(irnode->op)
      {
        default:{}break;
#define X(name) case RDI_EvalOp_##name:{str8_list_pushf(scratch.arena, &strings, #name);}break;
        RDI_EvalOp_XList
#undef X
      }
      if(irnode->value.u64 != 0)
      {
        str8_list_pushf(scratch.arena, &strings, " (%I64u)", irnode->value.u64);
      }
      str8_list_pushf(scratch.arena, &strings, "\n");
      Task *last_task = t;
      for(E_IRNode *child = irnode->first; child != &e_irnode_nil; child = child->next)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->next = last_task->next;
        last_task->next = task;
        task->irnode = child;
        task->indent = t->indent+1;
        last_task = task;
      }
    }
  }
  
  str8_list_pushf(scratch.arena, &strings, "\n");
  
  String8 result = str8_list_join(arena, &strings, 0);
  scratch_end(scratch);
  return result;
}
