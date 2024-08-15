// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Bundled Evaluation Functions

internal E_Eval
e_eval_from_string(Arena *arena, String8 string)
{
  E_TokenArray     tokens   = e_token_array_from_text(arena, string);
  E_Parse          parse    = e_parse_expr_from_text_tokens(arena, string, &tokens);
  E_IRTreeAndType  irtree   = e_irtree_and_type_from_expr(arena, parse.expr);
  E_OpList         oplist   = e_oplist_from_irtree(arena, irtree.root);
  String8          bytecode = e_bytecode_from_oplist(arena, &oplist);
  E_Interpretation interp   = e_interpret(bytecode);
  E_Eval eval =
  {
    .value    = interp.value,
    .mode     = irtree.mode,
    .space    = irtree.space,
    .type_key = irtree.type_key,
    .code     = interp.code,
    .advance  = parse.last_token >= tokens.v + tokens.count ? string.size : parse.last_token->range.min,
  };
  e_msg_list_concat_in_place(&eval.msgs, &parse.msgs);
  e_msg_list_concat_in_place(&eval.msgs, &irtree.msgs);
  if(E_InterpretationCode_Good < eval.code && eval.code < E_InterpretationCode_COUNT)
  {
    e_msg(arena, &eval.msgs, E_MsgKind_InterpretationError, 0, e_interpretation_code_display_strings[eval.code]);
  }
  return eval;
}

internal E_Eval
e_autoresolved_eval_from_eval(E_Eval eval)
{
  if(e_parse_ctx &&
     e_interpret_ctx &&
     e_parse_ctx->modules_count > 0 &&
     e_interpret_ctx->module_base != 0 &&
     (eval.mode == E_Mode_Value || eval.space == E_Space_Regs) &&
     (e_type_key_match(eval.type_key, e_type_key_basic(E_TypeKind_S64)) ||
      e_type_key_match(eval.type_key, e_type_key_basic(E_TypeKind_U64)) ||
      e_type_key_match(eval.type_key, e_type_key_basic(E_TypeKind_S32)) ||
      e_type_key_match(eval.type_key, e_type_key_basic(E_TypeKind_U32))))
  {
    U64 vaddr = eval.value.u64;
    U64 voff = vaddr - e_interpret_ctx->module_base[0];
    RDI_Parsed *rdi = e_parse_ctx->primary_module->rdi;
    RDI_Scope *scope = rdi_scope_from_voff(rdi, voff);
    RDI_Procedure *procedure = rdi_procedure_from_voff(rdi, voff);
    RDI_GlobalVariable *gvar = rdi_global_variable_from_voff(rdi, voff);
    U32 string_idx = 0;
    if(string_idx == 0) { string_idx = procedure->name_string_idx; }
    if(string_idx == 0) { string_idx = gvar->name_string_idx; }
    if(string_idx != 0)
    {
      eval.type_key = e_type_key_cons_ptr(e_type_key_basic(E_TypeKind_Void));
    }
  }
  return eval;
}

internal E_Eval
e_dynamically_typed_eval_from_eval(E_Eval eval)
{
  E_TypeKey type_key = eval.type_key;
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
      E_Type *ptee_type = e_type_from_key(scratch.arena, ptee_type_key);
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
        U64 addr_size = bit_size_from_arch(e_interpret_ctx->arch)/8;
        U64 class_base_vaddr = 0;
        U64 vtable_vaddr = 0;
        if(e_space_read(eval.space, &class_base_vaddr, r1u64(ptr_vaddr, ptr_vaddr+addr_size)) &&
           e_space_read(eval.space, &vtable_vaddr, r1u64(class_base_vaddr, class_base_vaddr+addr_size)))
        {
          U32 rdi_idx = 0;
          RDI_Parsed *rdi = 0;
          U64 module_base = 0;
          for(U64 idx = 0; idx < e_type_state->ctx->modules_count; idx += 1)
          {
            if(contains_1u64(e_type_state->ctx->modules[idx].vaddr_range, vtable_vaddr))
            {
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
              E_TypeKey ptr_to_derived_type_key = e_type_key_cons_ptr(derived_type_key);
              eval.type_key = ptr_to_derived_type_key;
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
  if(eval.mode == E_Mode_Offset)
  {
    E_TypeKey type_key = e_type_unwrap(eval.type_key);
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
    if(type_kind == E_TypeKind_Array)
    {
      eval.mode = E_Mode_Value;
    }
    else if(e_interpret_ctx->space_read != 0)
    {
      U64 type_byte_size = e_type_byte_size_from_key(type_key);
      Rng1U64 value_vaddr_range = r1u64(eval.value.u64, eval.value.u64 + type_byte_size);
      MemoryZeroStruct(&eval.value);
      if(!e_type_key_match(type_key, e_type_key_zero()) &&
         type_byte_size <= sizeof(E_Value) &&
         e_space_read(eval.space, &eval.value, value_vaddr_range))
      {
        eval.mode = E_Mode_Value;
        
        // rjf: mask&shift, for bitfields
        if(type_kind == E_TypeKind_Bitfield && type_byte_size <= sizeof(U64))
        {
          Temp scratch = scratch_begin(0, 0);
          E_Type *type = e_type_from_key(scratch.arena, type_key);
          U64 valid_bits_mask = 0;
          for(U64 idx = 0; idx < type->count; idx += 1)
          {
            valid_bits_mask |= (1<<idx);
          }
          eval.value.u64 = eval.value.u64 >> type->off;
          eval.value.u64 = eval.value.u64 & valid_bits_mask;
          eval.type_key = type->direct_type_key;
          scratch_end(scratch);
        }
        
        // rjf: manually sign-extend
        switch(type_kind)
        {
          default: break;
          case E_TypeKind_S8:  {eval.value.s64 = (S64)*((S8 *)&eval.value.u64);}break;
          case E_TypeKind_S16: {eval.value.s64 = (S64)*((S16 *)&eval.value.u64);}break;
          case E_TypeKind_S32: {eval.value.s64 = (S64)*((S32 *)&eval.value.u64);}break;
        }
      }
    }
  }
  return eval;
  
  // TODO(rjf): @spaces check regs path
#if 0
  switch(eval.mode)
  {
    //- rjf: no work to be done. already in value mode
    default:
    case E_Mode_Value:{}break;
    
    //- rjf: address => resolve into value, if leaf
    case E_Mode_Offset:
    {
      E_TypeKey type_key = e_type_unwrap(eval.type_key);
      E_TypeKind type_kind = e_type_kind_from_key(type_key);
      if(type_kind == E_TypeKind_Array)
      {
        eval.mode = E_Mode_Value;
      }
      else if(e_interpret_ctx->memory_read != 0)
      {
        U64 type_byte_size = e_type_byte_size_from_key(type_key);
        Rng1U64 value_vaddr_range = r1u64(eval.value.u64, eval.value.u64 + type_byte_size);
        MemoryZeroStruct(&eval.value);
        if(!e_type_key_match(type_key, e_type_key_zero()) &&
           type_byte_size <= sizeof(E_Value) &&
           e_interpret_ctx->memory_read(e_interpret_ctx->memory_read_user_data, &eval.value, value_vaddr_range))
        {
          eval.mode = E_Mode_Value;
          
          // rjf: mask&shift, for bitfields
          if(type_kind == E_TypeKind_Bitfield && type_byte_size <= sizeof(U64))
          {
            Temp scratch = scratch_begin(0, 0);
            E_Type *type = e_type_from_key(scratch.arena, type_key);
            U64 valid_bits_mask = 0;
            for(U64 idx = 0; idx < type->count; idx += 1)
            {
              valid_bits_mask |= (1<<idx);
            }
            eval.value.u64 = eval.value.u64 >> type->off;
            eval.value.u64 = eval.value.u64 & valid_bits_mask;
            eval.type_key = type->direct_type_key;
            scratch_end(scratch);
          }
          
          // rjf: manually sign-extend
          switch(type_kind)
          {
            default: break;
            case E_TypeKind_S8:  {eval.value.s64 = (S64)*((S8 *)&eval.value.u64);}break;
            case E_TypeKind_S16: {eval.value.s64 = (S64)*((S16 *)&eval.value.u64);}break;
            case E_TypeKind_S32: {eval.value.s64 = (S64)*((S32 *)&eval.value.u64);}break;
          }
        }
      }
    }break;
    
    //- rjf: register => resolve into value
    case E_Mode_Reg:
    {
      E_TypeKey type_key = eval.type_key;
      U64 type_byte_size = e_type_byte_size_from_key(type_key);
      U64 reg_off = eval.value.u64;
      MemoryZeroStruct(&eval.value);
      MemoryCopy(&eval.value.u256, ((U8 *)e_interpret_ctx->reg_data + reg_off), Min(type_byte_size, sizeof(U64)*4));
      eval.mode = E_Mode_Value;
    }break;
  }
#endif
  
  return eval;
}

internal E_Eval
e_element_eval_from_array_eval_index(E_Eval eval, U64 index)
{
  E_Eval result = {0};
  result.mode     = eval.mode;
  result.space    = eval.space;
  result.type_key = e_type_direct_from_key(eval.type_key);
  result.code     = eval.code;
  result.msgs     = eval.msgs;
  U64 element_size = e_type_byte_size_from_key(result.type_key);
  switch(eval.mode)
  {
    default:{}break;
    case E_Mode_Value:
    if(element_size <= sizeof(E_Value) &&
       index < sizeof(E_Value)/element_size)
    {
      MemoryCopy((U8 *)(&result.value.u512[0]),
                 (U8 *)(&eval.value.u512[0]) + index*element_size,
                 element_size);
    }break;
    case E_Mode_Offset:
    {
      result.value.u64 = eval.value.u64 + element_size*index;
    }break;
  }
  return result;
}
