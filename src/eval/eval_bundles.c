// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Bundled Evaluation Functions

internal E_Eval
e_eval_from_expr(Arena *arena, E_Expr *expr)
{
  E_ExprChain exprs = {expr, expr};
  E_Eval result = e_eval_from_exprs(arena, exprs);
  return result;
}

internal E_Eval
e_eval_from_exprs(Arena *arena, E_ExprChain exprs)
{
  ProfBeginFunction();
  E_IRTreeAndType     irtree   = e_irtree_and_type_from_expr(arena, exprs.last);
  E_LookupRuleTagPair lookup   = e_lookup_rule_tag_pair_from_expr_irtree(exprs.last, &irtree);
  E_OpList            oplist   = e_oplist_from_irtree(arena, irtree.root);
  String8             bytecode = e_bytecode_from_oplist(arena, &oplist);
  E_Interpretation    interp   = e_interpret(bytecode);
  E_Eval eval =
  {
    .value           = interp.value,
    .space           = interp.space,
    .exprs           = exprs,
    .irtree          = irtree,
    .lookup_rule_tag = lookup,
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
  E_TokenArray     tokens   = e_token_array_from_text(arena, string);
  E_Parse          parse    = e_parse_expr_from_text_tokens(arena, string, &tokens);
  E_Eval           eval     = e_eval_from_exprs(arena, parse.exprs);
  e_msg_list_concat_in_place(&eval.msgs, &parse.msgs);
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
e_autoresolved_eval_from_eval(E_Eval eval)
{
  if(e_parse_state &&
     e_interpret_ctx &&
     e_parse_state->ctx->modules_count > 0 &&
     e_interpret_ctx->module_base != 0 &&
     (e_type_key_match(eval.irtree.type_key, e_type_key_basic(E_TypeKind_S64)) ||
      e_type_key_match(eval.irtree.type_key, e_type_key_basic(E_TypeKind_U64)) ||
      e_type_key_match(eval.irtree.type_key, e_type_key_basic(E_TypeKind_S32)) ||
      e_type_key_match(eval.irtree.type_key, e_type_key_basic(E_TypeKind_U32))))
  {
    U64 vaddr = eval.value.u64;
    U64 voff = vaddr - e_interpret_ctx->module_base[0];
    RDI_Parsed *rdi = e_parse_state->ctx->primary_module->rdi;
    RDI_Scope *scope = rdi_scope_from_voff(rdi, voff);
    RDI_Procedure *procedure = rdi_procedure_from_voff(rdi, voff);
    RDI_GlobalVariable *gvar = rdi_global_variable_from_voff(rdi, voff);
    U32 string_idx = 0;
    if(string_idx == 0) { string_idx = procedure->name_string_idx; }
    if(string_idx == 0) { string_idx = gvar->name_string_idx; }
    if(string_idx != 0)
    {
      eval.irtree.type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, e_type_key_basic(E_TypeKind_Void), 1, 0);
    }
  }
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
    E_TypeKey ptee_type_key = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(type_key)));
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
        U64 addr_size = e_type_byte_size_from_key(e_type_unwrap(type_key));
        U64 class_base_vaddr = 0;
        U64 vtable_vaddr = 0;
        if(e_space_read(eval.space, &class_base_vaddr, r1u64(ptr_vaddr, ptr_vaddr+addr_size)) &&
           e_space_read(eval.space, &vtable_vaddr, r1u64(class_base_vaddr, class_base_vaddr+addr_size)))
        {
          Arch arch = e_type_state->ctx->primary_module->arch;
          U32 rdi_idx = 0;
          RDI_Parsed *rdi = 0;
          U64 module_base = 0;
          for(U64 idx = 0; idx < e_type_state->ctx->modules_count; idx += 1)
          {
            if(contains_1u64(e_type_state->ctx->modules[idx].vaddr_range, vtable_vaddr))
            {
              arch = e_type_state->ctx->modules[idx].arch;
              rdi_idx = (U32)idx;
              rdi = e_type_state->ctx->modules[idx].rdi;
              module_base = e_type_state->ctx->modules[idx].vaddr_range.min;
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

internal E_Eval
e_value_eval_from_eval(E_Eval eval)
{
  ProfBeginFunction();
  if(eval.irtree.mode == E_Mode_Offset)
  {
    E_TypeKey type_key = e_type_unwrap(eval.irtree.type_key);
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
    if(type_kind == E_TypeKind_Array)
    {
      eval.irtree.mode = E_Mode_Value;
    }
    else
    {
      U64 type_byte_size = e_type_byte_size_from_key(type_key);
      Rng1U64 value_vaddr_range = r1u64(eval.value.u64, eval.value.u64 + type_byte_size);
      MemoryZeroStruct(&eval.value);
      if(!e_type_key_match(type_key, e_type_key_zero()) &&
         type_byte_size <= sizeof(E_Value) &&
         e_space_read(eval.space, &eval.value, value_vaddr_range))
      {
        eval.irtree.mode = E_Mode_Value;
        
        // rjf: mask&shift, for bitfields
        if(type_kind == E_TypeKind_Bitfield && type_byte_size <= sizeof(U64))
        {
          Temp scratch = scratch_begin(0, 0);
          E_Type *type = e_type_from_key__cached(type_key);
          U64 valid_bits_mask = 0;
          for(U64 idx = 0; idx < type->count; idx += 1)
          {
            valid_bits_mask |= (1ull<<idx);
          }
          eval.value.u64 = eval.value.u64 >> type->off;
          eval.value.u64 = eval.value.u64 & valid_bits_mask;
          eval.irtree.type_key = type->direct_type_key;
          scratch_end(scratch);
        }
        
        // rjf: manually sign-extend
        switch(type_kind)
        {
          default: break;
          case E_TypeKind_Char8:
          case E_TypeKind_S8:  {eval.value.s64 = (S64)*((S8 *)&eval.value.u64);}break;
          case E_TypeKind_Char16:
          case E_TypeKind_S16: {eval.value.s64 = (S64)*((S16 *)&eval.value.u64);}break;
          case E_TypeKind_Char32:
          case E_TypeKind_S32: {eval.value.s64 = (S64)*((S32 *)&eval.value.u64);}break;
        }
      }
    }
  }
  ProfEnd();
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