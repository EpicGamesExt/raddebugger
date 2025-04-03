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
  if(ctx->primary_module == 0) { ctx->primary_module = &e_module_nil; }
  if(ctx->regs_map == 0)       { ctx->regs_map = &e_string2num_map_nil; }
  if(ctx->reg_alias_map == 0)  { ctx->reg_alias_map = &e_string2num_map_nil; }
  if(ctx->locals_map == 0)     { ctx->locals_map = &e_string2num_map_nil; }
  if(ctx->member_map == 0)     { ctx->member_map = &e_string2num_map_nil; }
  if(ctx->macro_map == 0)      { ctx->macro_map = push_array(e_ir_state->arena, E_String2ExprMap, 1); ctx->macro_map[0] = e_string2expr_map_make(e_ir_state->arena, 512); }
  e_ir_state->ctx = ctx;
  e_ir_state->thread_ip_procedure = rdi_procedure_from_voff(ctx->primary_module->rdi, ctx->thread_ip_voff);
  e_ir_state->used_tag_map = push_array(e_ir_state->arena, E_UsedTagMap, 1);
  e_ir_state->used_tag_map->slots_count = 64;
  e_ir_state->used_tag_map->slots = push_array(e_ir_state->arena, E_UsedTagSlot, e_ir_state->used_tag_map->slots_count);
  e_ir_state->type_auto_hook_cache_map = push_array(e_ir_state->arena, E_TypeAutoHookCacheMap, 1);
  e_ir_state->type_auto_hook_cache_map->slots_count = 256;
  e_ir_state->type_auto_hook_cache_map->slots = push_array(e_ir_state->arena, E_TypeAutoHookCacheSlot, e_ir_state->type_auto_hook_cache_map->slots_count);
  e_ir_state->string_id_gen = 0;
  e_ir_state->string_id_map = push_array(e_ir_state->arena, E_StringIDMap, 1);
  e_ir_state->string_id_map->id_slots_count = 1024;
  e_ir_state->string_id_map->id_slots = push_array(e_ir_state->arena, E_StringIDSlot, e_ir_state->string_id_map->id_slots_count);
  e_ir_state->string_id_map->hash_slots_count = 1024;
  e_ir_state->string_id_map->hash_slots = push_array(e_ir_state->arena, E_StringIDSlot, e_ir_state->string_id_map->hash_slots_count);
  String8 builtin_view_rule_names[] =
  {
    str8_lit_comp("bswap"),
    str8_lit_comp("array"),
  };
  for EachElement(idx, builtin_view_rule_names)
  {
    E_Expr *expr = e_push_expr(e_ir_state->arena, E_ExprKind_LeafOffset, 0);
    expr->type_key = e_type_key_cons(.kind = E_TypeKind_Stub, .name = str8_lit("view_rule"));
    expr->value.u64 = e_id_from_string(builtin_view_rule_names[idx]);
    e_string2expr_map_insert(e_ir_state->arena, ctx->macro_map, builtin_view_rule_names[idx], expr);
  }
}

////////////////////////////////
//~ rjf: File System Lookup Rules

typedef struct E_FolderAccel E_FolderAccel;
struct E_FolderAccel
{
  String8 folder_path;
  String8Array folders;
  String8Array files;
};

E_LOOKUP_INFO_FUNCTION_DEF(folder)
{
  E_LookupInfo info = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: evaluate lhs file path ID
    E_OpList lhs_oplist = e_oplist_from_irtree(scratch.arena, lhs->root);
    String8 lhs_bytecode = e_bytecode_from_oplist(scratch.arena, &lhs_oplist);
    E_Interpretation lhs_interp = e_interpret(lhs_bytecode);
    E_Value lhs_value = lhs_interp.value;
    U64 lhs_string_id = lhs_value.u64;
    String8 folder_path = e_string_from_id(lhs_string_id);
    
    //- rjf: compute filter - omit common prefixes (common parent paths)
    String8 local_filter = filter;
    {
      U64 folder_pos_in_filter = str8_find_needle(filter, 0, folder_path, StringMatchFlag_CaseInsensitive|StringMatchFlag_SlashInsensitive);
      if(folder_pos_in_filter < filter.size)
      {
        local_filter = str8_skip(local_filter, folder_pos_in_filter+folder_path.size);
        local_filter = str8_skip_chop_slashes(local_filter);
      }
      else
      {
        MemoryZeroStruct(&local_filter);
      }
    }
    
    //- rjf: gather & filter files in this folder
    String8List folder_paths = {0};
    String8List file_paths = {0};
    {
      OS_FileIter *iter = os_file_iter_begin(scratch.arena, folder_path, 0);
      for(OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, iter, &info);)
      {
        FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, local_filter, info.name);
        if(matches.count == matches.needle_part_count)
        {
          if(info.props.flags & FilePropertyFlag_IsFolder)
          {
            str8_list_push(scratch.arena, &folder_paths, push_str8_copy(arena, info.name));
          }
          else
          {
            str8_list_push(scratch.arena, &file_paths, push_str8_copy(arena, info.name));
          }
        }
      }
      os_file_iter_end(iter);
    }
    
    //- rjf: build accelerator
    E_FolderAccel *accel = push_array(arena, E_FolderAccel, 1);
    accel->folder_path = push_str8_copy(arena, folder_path);
    accel->folders = str8_array_from_list(arena, &folder_paths);
    accel->files = str8_array_from_list(arena, &file_paths);
    info.user_data = accel;
    info.idxed_expr_count = accel->folders.count + accel->files.count;
    scratch_end(scratch);
  }
  return info;
}

E_LOOKUP_RANGE_FUNCTION_DEF(folder)
{
  E_FolderAccel *accel = (E_FolderAccel *)user_data;
  U64 out_idx = 0;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_Expr *expr = &e_expr_nil;
    String8 expr_string = {0};
    if(0 <= idx && idx < accel->folders.count)
    {
      String8 folder_name = accel->folders.v[idx - 0];
      String8 folder_path = push_str8f(scratch.arena, "%S%s%S", accel->folder_path, accel->folder_path.size != 0 ? "/" : "", folder_name);
      expr = e_push_expr(arena, E_ExprKind_LeafValue, 0);
      expr->type_key = e_type_key_cons(.kind = E_TypeKind_Stub, .name = str8_lit("folder"));
      expr->space = e_space_make(E_SpaceKind_FileSystem);
      expr->value.u64 = e_id_from_string(folder_path);
      expr_string = push_str8f(arena, "\"%S\"", escaped_from_raw_str8(scratch.arena, folder_name));
    }
    else if(accel->folders.count <= idx && idx < accel->folders.count + accel->files.count)
    {
      String8 file_name = accel->files.v[idx - accel->folders.count];
      String8 file_path = push_str8f(scratch.arena, "%S%s%S", accel->folder_path, accel->folder_path.size != 0 ? "/" : "", file_name);
      expr = e_push_expr(arena, E_ExprKind_LeafValue, 0);
      expr->type_key = e_type_key_cons(.kind = E_TypeKind_Stub, .name = str8_lit("file"));
      expr->space = e_space_make(E_SpaceKind_FileSystem);
      expr->value.u64 = e_id_from_string(file_path);
      expr_string = push_str8f(arena, "\"%S\"", escaped_from_raw_str8(scratch.arena, file_name));
    }
    exprs[out_idx] = expr;
    exprs_strings[out_idx] = expr_string;
    scratch_end(scratch);
  }
}

E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(folder)
{
  U64 id = 0;
  E_FolderAccel *accel = (E_FolderAccel *)user_data;
  String8 name = {0};
  if(0 < num && num <= accel->folders.count)
  {
    name = accel->folders.v[num-1];
  }
  else if(accel->folders.count < num && num <= accel->folders.count+accel->files.count)
  {
    name = accel->files.v[num-accel->folders.count-1];
  }
  id = e_hash_from_string(5381, name);
  return id;
}

E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(folder)
{
  U64 num = 0;
  E_FolderAccel *accel = (E_FolderAccel *)user_data;
  for(U64 idx = 0; idx < accel->folders.count+accel->files.count; idx += 1)
  {
    String8 name = {0};
    if(0 <= idx && idx < accel->folders.count)
    {
      name = accel->folders.v[idx];
    }
    else if(accel->folders.count <= idx && idx < accel->folders.count+accel->files.count)
    {
      name = accel->files.v[idx-accel->folders.count];
    }
    U64 hash = e_hash_from_string(5381, name);
    if(hash == id)
    {
      num = idx+1;
      break;
    }
  }
  return num;
}

typedef struct E_FileAccel E_FileAccel;
struct E_FileAccel
{
  String8 file_path;
  FileProperties props;
  String8Array fields;
};

E_LOOKUP_INFO_FUNCTION_DEF(file)
{
  E_FileAccel *accel = push_array(arena, E_FileAccel, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: evaluate lhs file path ID
    E_OpList lhs_oplist = e_oplist_from_irtree(scratch.arena, lhs->root);
    String8 lhs_bytecode = e_bytecode_from_oplist(scratch.arena, &lhs_oplist);
    E_Interpretation lhs_interp = e_interpret(lhs_bytecode);
    E_Value lhs_value = lhs_interp.value;
    U64 lhs_string_id = lhs_value.u64;
    
    //- rjf: get file path
    String8 file_path = e_string_from_id(lhs_string_id);
    
    //- rjf: build field list
    String8List fields = {0};
    str8_list_pushf(arena, &fields, "size");
    str8_list_pushf(arena, &fields, "last_modified_time");
    str8_list_pushf(arena, &fields, "creation_time");
    str8_list_pushf(arena, &fields, "data");
    
    //- rjf: fill accel
    accel->file_path = push_str8_copy(arena, file_path);
    accel->props = os_properties_from_file_path(file_path);
    accel->fields = str8_array_from_list(arena, &fields);
    
    scratch_end(scratch);
  }
  E_LookupInfo info = {accel, accel->fields.count};
  return info;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(file)
{
  E_LookupAccess result = {{&e_irnode_nil}}; 
  if(kind == E_ExprKind_MemberAccess)
  {
    E_FileAccel *accel = (E_FileAccel *)user_data;
    String8 member_name = rhs->string;
    if(str8_match(member_name, str8_lit("size"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_FileSystem);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.irtree_and_type.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, accel->props.size));
      result.irtree_and_type.type_key = e_type_key_basic(E_TypeKind_U64);
      result.irtree_and_type.mode = E_Mode_Value;
    }
    else if(str8_match(member_name, str8_lit("last_modified_time"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_FileSystem);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.irtree_and_type.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, accel->props.modified));
      result.irtree_and_type.type_key = e_type_key_basic(E_TypeKind_U64);
      result.irtree_and_type.mode = E_Mode_Value;
    }
    else if(str8_match(member_name, str8_lit("creation_time"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_FileSystem);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.irtree_and_type.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, accel->props.created));
      result.irtree_and_type.type_key = e_type_key_basic(E_TypeKind_U64);
      result.irtree_and_type.mode = E_Mode_Value;
    }
    else if(str8_match(member_name, str8_lit("data"), 0))
    {
      E_Space space = e_space_make(E_SpaceKind_File);
      space.u64_0 = e_id_from_string(accel->file_path);
      result.irtree_and_type.root     = e_irtree_set_space(arena, space, e_irtree_const_u(arena, 0));
      result.irtree_and_type.type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), accel->props.size, 0);
      result.irtree_and_type.mode = E_Mode_Offset;
    }
  }
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(file)
{
  E_FileAccel *accel = (E_FileAccel *)user_data;
  U64 out_idx = 0;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    E_Expr *expr = &e_expr_nil;
    String8 string = {0};
    if(0 <= idx && idx < accel->fields.count)
    {
      String8 name = accel->fields.v[idx];
      expr = e_push_expr(arena, E_ExprKind_MemberAccess, 0);
      E_Expr *rhs = e_push_expr(arena, E_ExprKind_LeafIdentifier, 0);
      rhs->string = push_str8_copy(arena, name);
      e_expr_push_child(expr, e_expr_ref(arena, lhs));
      e_expr_push_child(expr, rhs);
    }
    exprs[out_idx] = expr;
    exprs_strings[out_idx] = string;
  }
}

////////////////////////////////
//~ rjf: Slice Lookup Rules

typedef struct E_SliceAccel E_SliceAccel;
struct E_SliceAccel
{
  Arch arch;
  U64 count;
  U64 base_ptr_vaddr;
  E_TypeKey element_type_key;
};

E_LOOKUP_INFO_FUNCTION_DEF(slice)
{
  E_LookupInfo info = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    // rjf: unpack struct type
    E_TypeKey struct_type_key = e_type_unwrap(lhs->type_key);
    for(;;)
    {
      if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(struct_type_key)))
      {
        struct_type_key = e_type_unwrap(e_type_direct_from_key(struct_type_key));
      }
      else
      {
        break;
      }
    }
    
    // rjf: build info from struct type
    E_TypeKind type_kind = e_type_kind_from_key(struct_type_key);
    if(type_kind == E_TypeKind_Struct || type_kind == E_TypeKind_Class)
    {
      // rjf: unpack members
      E_MemberArray members = e_type_data_members_from_key__cached(struct_type_key);
      
      // rjf: choose base pointer & count members
      E_Member *base_ptr_member = 0;
      E_Member *opl_ptr_member = 0;
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
        else if(base_ptr_member != 0 && opl_ptr_member == 0 && e_type_kind_is_pointer_or_ref(member_type_kind))
        {
          opl_ptr_member = &members.v[idx];
        }
        if(count_member != 0 && base_ptr_member != 0)
        {
          break;
        }
        else if(base_ptr_member != 0 && opl_ptr_member != 0)
        {
          break;
        }
      }
      
      // rjf: determine architecture
      Arch arch = e_type_state->ctx->primary_module->arch;
      if(base_ptr_member != 0)
      {
        E_Type *type = e_type_from_key__cached(base_ptr_member->type_key);
        arch = type->arch;
      }
      
      // rjf: evaluate count member, determine count
      U64 count = 0;
      if(count_member != 0)
      {
        E_Expr *count_member_expr = e_expr_irext_member_access(arena, &e_expr_nil, lhs, count_member->name);
        E_Value count_member_value = e_value_from_expr(count_member_expr);
        count = count_member_value.u64;
      }
      
      // rjf: evaluate base ptr member, determine base address
      U64 base_ptr_vaddr = 0;
      if(base_ptr_member != 0)
      {
        E_Expr *base_ptr_member_expr = e_expr_irext_member_access(arena, &e_expr_nil, lhs, base_ptr_member->name);
        E_Value base_ptr_member_value = e_value_from_expr(base_ptr_member_expr);
        base_ptr_vaddr = base_ptr_member_value.u64;
      }
      
      // rjf: evaluate opl ptr member, determine opl address
      U64 opl_ptr_vaddr = 0;
      if(count_member == 0 && opl_ptr_member != 0)
      {
        E_Expr *opl_ptr_member_expr = e_expr_irext_member_access(arena, &e_expr_nil, lhs, opl_ptr_member->name);
        E_Value opl_ptr_member_value = e_value_from_expr(opl_ptr_member_expr);
        opl_ptr_vaddr = opl_ptr_member_value.u64;
      }
      
      // rjf: determine element type
      E_TypeKey element_type_key = zero_struct;
      if(base_ptr_member != 0)
      {
        element_type_key = e_type_direct_from_key(base_ptr_member->type_key);
      }
      
      // rjf: if no count, but base/opl, swap base/opl if needed, and measure count
      if(count_member == 0 && opl_ptr_member != 0 && base_ptr_member != 0)
      {
        U64 min_vaddr = Min(base_ptr_vaddr, opl_ptr_vaddr);
        U64 max_vaddr = Max(base_ptr_vaddr, opl_ptr_vaddr);
        base_ptr_vaddr = min_vaddr;
        opl_ptr_vaddr = max_vaddr;
        count = (opl_ptr_vaddr - base_ptr_vaddr) / e_type_byte_size_from_key(element_type_key);
      }
      
      // rjf: fill
      if((count_member || opl_ptr_member) && base_ptr_member)
      {
        E_SliceAccel *accel = push_array(arena, E_SliceAccel, 1);
        accel->arch = arch;
        accel->count = count;
        accel->base_ptr_vaddr = base_ptr_vaddr;
        accel->element_type_key = element_type_key;
        info.user_data = accel;
        info.idxed_expr_count = accel->count;
      }
      
      // rjf: fall back to default
      else
      {
        info = E_LOOKUP_INFO_FUNCTION_NAME(default)(arena, lhs, tag, filter);
      }
    }
    scratch_end(scratch);
  }
  return info;
}

E_LOOKUP_RANGE_FUNCTION_DEF(slice)
{
  if(user_data == 0)
  {
    E_LOOKUP_RANGE_FUNCTION_NAME(default)(arena, lhs, tag, filter, idx_range, exprs, exprs_strings, user_data);
  }
  else
  {
    E_SliceAccel *accel = (E_SliceAccel *)user_data;
    U64 out_idx = 0;
    U64 element_type_size = e_type_byte_size_from_key(accel->element_type_key);
    for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
    {
      E_Expr *expr = e_push_expr(arena, E_ExprKind_LeafOffset, 0);
      expr->value.u64 = accel->base_ptr_vaddr + idx*element_type_size;
      expr->type_key = accel->element_type_key;
      exprs[out_idx] = expr;
      exprs_strings[out_idx] = push_str8f(arena, "[%I64u]", idx);
    }
  }
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
           direct_type_kind == E_TypeKind_Union)
        {
          E_MemberArray data_members = e_type_data_members_from_key_filter__cached(direct_type_key, filter);
          lookup_info.named_expr_count = data_members.count;
        }
        else if(direct_type_kind == E_TypeKind_Enum)
        {
          E_Type *direct_type = e_type_from_key__cached(direct_type_key);
          lookup_info.named_expr_count = direct_type->count;
        }
      }
    }
    else if(lhs_type_kind == E_TypeKind_Struct ||
            lhs_type_kind == E_TypeKind_Class ||
            lhs_type_kind == E_TypeKind_Union)
    {
      E_MemberArray data_members = e_type_data_members_from_key_filter__cached(lhs_type_key, filter);
      lookup_info.named_expr_count = data_members.count;
    }
    else if(lhs_type_kind == E_TypeKind_Enum)
    {
      E_Type *direct_type = e_type_from_key__cached(lhs_type_key);
      lookup_info.named_expr_count = direct_type->count;
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
        if(match.kind == E_MemberKind_Query)
        {
          r_query_name = exprr->string;
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
      else if(exprr->kind != E_ExprKind_LeafIdentifier)
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
        E_TypeKey new_tree_type = r_type;
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
        result.irtree_and_type.member   = member;
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
    B32 do_enum_range = 0;
    B32 do_index_range = 0;
    E_TypeKey enum_type_key = zero_struct;
    E_TypeKey struct_type_key = zero_struct;
    E_TypeKind struct_type_kind = E_TypeKind_Null;
    if(e_type_kind_is_pointer_or_ref(lhs_type_kind))
    {
      E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
      if(lhs_type->count == 1 &&
         (direct_type_kind == E_TypeKind_Struct ||
          direct_type_kind == E_TypeKind_Union ||
          direct_type_kind == E_TypeKind_Class))
      {
        struct_type_key = direct_type_key;
        struct_type_kind = direct_type_kind;
        do_struct_range = 1;
      }
      else if(lhs_type->count == 1 && direct_type_kind == E_TypeKind_Enum)
      {
        do_enum_range = 1;
        enum_type_key = direct_type_key;
      }
      else
      {
        do_index_range = 1;
      }
    }
    else if(lhs_type_kind == E_TypeKind_Struct ||
            lhs_type_kind == E_TypeKind_Union ||
            lhs_type_kind == E_TypeKind_Class)
    {
      struct_type_key = lhs_type_key;
      struct_type_kind = lhs_type_kind;
      do_struct_range = 1;
    }
    else if(lhs_type_kind == E_TypeKind_Enum)
    {
      enum_type_key = lhs_type_key;
      do_enum_range = 1;
    }
    else if(lhs_type_kind == E_TypeKind_Stub)
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
      E_MemberArray data_members = e_type_data_members_from_key_filter__cached(struct_type_key, filter);
      Rng1U64 legal_idx_range = r1u64(0, data_members.count);
      Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
      U64 read_range_count = dim_1u64(read_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        U64 member_idx = idx + read_range.min;
        String8 member_name = data_members.v[member_idx].name;
        exprs[idx] = e_expr_irext_member_access(arena, lhs, &lhs_irtree, member_name);
      }
    }
    
    //- rjf: enum case -> the lookup-range will return a range of enum constants
    else if(do_enum_range)
    {
      E_Type *type = e_type_from_key__cached(enum_type_key);
      Rng1U64 legal_idx_range = r1u64(0, type->count);
      Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
      U64 read_range_count = dim_1u64(read_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        U64 member_idx = idx + read_range.min;
        String8 member_name = type->enum_vals[member_idx].name;
        exprs[idx] = e_expr_irext_member_access(arena, lhs, &lhs_irtree, member_name);
      }
    }
    
    //- rjf: ptr case -> the lookup-range will return a range of dereferences
    else if(do_index_range)
    {
      U64 read_range_count = dim_1u64(idx_range);
      for(U64 idx = 0; idx < read_range_count; idx += 1)
      {
        exprs[idx] = e_expr_irext_array_index(arena, lhs, &lhs_irtree, idx_range.min + idx);
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
//~ rjf: Member Filtering Lookup Rules

typedef struct E_MemberFilterAccel E_MemberFilterAccel;
struct E_MemberFilterAccel
{
  E_MemberArray members;
};

E_LOOKUP_INFO_FUNCTION_DEF(only)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_LookupInfo lookup_info = {0};
  {
    //- rjf: extract struct type
    E_TypeKey struct_type_key = zero_struct;
    {
      E_TypeKey lhs_type_key = e_type_unwrap(lhs->type_key);
      E_TypeKind lhs_type_kind = e_type_kind_from_key(lhs_type_key);
      if(e_type_kind_is_pointer_or_ref(lhs_type_kind))
      {
        E_Type *type = e_type_from_key__cached(lhs_type_key);
        if(type->count == 1)
        {
          E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(lhs->type_key));
          E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
          if(direct_type_kind == E_TypeKind_Struct ||
             direct_type_kind == E_TypeKind_Class ||
             direct_type_kind == E_TypeKind_Union)
          {
            struct_type_key = direct_type_key;
          }
        }
      }
      else if(lhs_type_kind == E_TypeKind_Struct ||
              lhs_type_kind == E_TypeKind_Class ||
              lhs_type_kind == E_TypeKind_Union)
      {
        struct_type_key = lhs_type_key;
      }
    }
    
    //- rjf: not struct -> fall back on default
    if(e_type_key_match(struct_type_key, e_type_key_zero()))
    {
      lookup_info = E_LOOKUP_INFO_FUNCTION_NAME(default)(arena, lhs, tag, filter);
    }
    
    //- struct -> filter
    else
    {
      E_MemberArray data_members = e_type_data_members_from_key__cached(struct_type_key);
      E_MemberList data_members_list__filtered = {0};
      for EachIndex(idx, data_members.count)
      {
        B32 fits_filter = 0;
        for(E_Expr *name = tag->first->next; name != &e_expr_nil; name = name->next)
        {
          if(str8_match(name->string, data_members.v[idx].name, 0))
          {
            fits_filter = 1;
            break;
          }
        }
        if(fits_filter)
        {
          e_member_list_push(scratch.arena, &data_members_list__filtered, &data_members.v[idx]);
        }
      }
      E_MemberFilterAccel *accel = push_array(arena, E_MemberFilterAccel, 1);
      accel->members = e_member_array_from_list(arena, &data_members_list__filtered);
      lookup_info.user_data = accel;
      lookup_info.named_expr_count = accel->members.count;
    }
  }
  scratch_end(scratch);
  return lookup_info;
}

E_LOOKUP_INFO_FUNCTION_DEF(omit)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_LookupInfo lookup_info = {0};
  {
    //- rjf: extract struct type
    E_TypeKey struct_type_key = zero_struct;
    {
      E_TypeKey lhs_type_key = e_type_unwrap(lhs->type_key);
      E_TypeKind lhs_type_kind = e_type_kind_from_key(lhs_type_key);
      if(e_type_kind_is_pointer_or_ref(lhs_type_kind))
      {
        E_Type *type = e_type_from_key__cached(lhs_type_key);
        if(type->count == 1)
        {
          E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(lhs->type_key));
          E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
          if(direct_type_kind == E_TypeKind_Struct ||
             direct_type_kind == E_TypeKind_Class ||
             direct_type_kind == E_TypeKind_Union)
          {
            struct_type_key = direct_type_key;
          }
        }
      }
      else if(lhs_type_kind == E_TypeKind_Struct ||
              lhs_type_kind == E_TypeKind_Class ||
              lhs_type_kind == E_TypeKind_Union)
      {
        struct_type_key = lhs_type_key;
      }
    }
    
    //- rjf: not struct -> fall back on default
    if(e_type_key_match(struct_type_key, e_type_key_zero()))
    {
      lookup_info = E_LOOKUP_INFO_FUNCTION_NAME(default)(arena, lhs, tag, filter);
    }
    
    //- struct -> filter
    else
    {
      E_MemberArray data_members = e_type_data_members_from_key__cached(struct_type_key);
      E_MemberList data_members_list__filtered = {0};
      for EachIndex(idx, data_members.count)
      {
        B32 fits_filter = 1;
        for(E_Expr *name = tag->first->next; name != &e_expr_nil; name = name->next)
        {
          if(str8_match(name->string, data_members.v[idx].name, 0))
          {
            fits_filter = 0;
            break;
          }
        }
        if(fits_filter)
        {
          e_member_list_push(scratch.arena, &data_members_list__filtered, &data_members.v[idx]);
        }
      }
      E_MemberFilterAccel *accel = push_array(arena, E_MemberFilterAccel, 1);
      accel->members = e_member_array_from_list(arena, &data_members_list__filtered);
      lookup_info.user_data = accel;
      lookup_info.named_expr_count = accel->members.count;
    }
  }
  scratch_end(scratch);
  return lookup_info;
}

E_LOOKUP_RANGE_FUNCTION_DEF(only_and_omit)
{
  if(user_data == 0)
  {
    E_LOOKUP_RANGE_FUNCTION_NAME(default)(arena, lhs, tag, filter, idx_range, exprs, exprs_strings, user_data);
  }
  else
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(scratch.arena, lhs);
    E_MemberFilterAccel *accel = (E_MemberFilterAccel *)user_data;
    Rng1U64 legal_idx_range = r1u64(0, accel->members.count);
    Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
    U64 read_range_count = dim_1u64(read_range);
    for(U64 idx = 0; idx < read_range_count; idx += 1)
    {
      U64 member_idx = idx + read_range.min;
      String8 member_name = accel->members.v[member_idx].name;
      exprs[idx] = e_expr_irext_member_access(arena, lhs, &lhs_irtree, member_name);
    }
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Lookups

internal E_LookupRuleMap
e_lookup_rule_map_make(Arena *arena, U64 slots_count)
{
  E_LookupRuleMap map = {0};
  map.slots_count = slots_count;
  map.slots = push_array(arena, E_LookupRuleSlot, map.slots_count);
  e_lookup_rule_map_insert_new(arena, &map, str8_lit("default"),
                               .info   = E_LOOKUP_INFO_FUNCTION_NAME(default),
                               .range  = E_LOOKUP_RANGE_FUNCTION_NAME(default),
                               .id_from_num  = E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(default),
                               .num_from_id  = E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(default));
  e_lookup_rule_map_insert_new(arena, &map, str8_lit("folder"),
                               .info   = E_LOOKUP_INFO_FUNCTION_NAME(folder),
                               .range  = E_LOOKUP_RANGE_FUNCTION_NAME(folder),
                               .id_from_num  = E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(folder),
                               .num_from_id  = E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(folder));
  e_lookup_rule_map_insert_new(arena, &map, str8_lit("file"),
                               .info   = E_LOOKUP_INFO_FUNCTION_NAME(file),
                               .access = E_LOOKUP_ACCESS_FUNCTION_NAME(file),
                               .range  = E_LOOKUP_RANGE_FUNCTION_NAME(file));
  e_lookup_rule_map_insert_new(arena, &map, str8_lit("slice"),
                               .info   = E_LOOKUP_INFO_FUNCTION_NAME(slice),
                               .range  = E_LOOKUP_RANGE_FUNCTION_NAME(slice));
  e_lookup_rule_map_insert_new(arena, &map, str8_lit("only"),
                               .info   = E_LOOKUP_INFO_FUNCTION_NAME(only),
                               .range  = E_LOOKUP_RANGE_FUNCTION_NAME(only_and_omit));
  e_lookup_rule_map_insert_new(arena, &map, str8_lit("omit"),
                               .info   = E_LOOKUP_INFO_FUNCTION_NAME(omit),
                               .range  = E_LOOKUP_RANGE_FUNCTION_NAME(only_and_omit));
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

////////////////////////////////
//~ rjf: IR Gen Rules

E_IRGEN_FUNCTION_DEF(bswap)
{
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(arena, expr);
  E_IRNode *root = e_push_irnode(arena, RDI_EvalOp_ByteSwap);
  e_irnode_push_child(root, irtree.root);
  E_IRTreeAndType result = {root, irtree.type_key, irtree.member, irtree.mode, irtree.msgs};
  return result;
}

E_IRGEN_FUNCTION_DEF(array)
{
  E_Expr *ptr_expr = expr->first->next;
  E_Expr *count_expr = ptr_expr->next;
  E_IRTreeAndType result = e_irtree_and_type_from_expr(arena, ptr_expr);
  E_TypeKey element_type_key = e_type_ptee_from_key(result.type_key);
  E_Value count_value = e_value_from_expr(count_expr);
  result.type_key = e_type_key_cons_ptr(e_type_state->ctx->primary_module->arch, element_type_key, count_value.u64, 0);
  return result;
}

E_IRGEN_FUNCTION_DEF(view_rule_noop)
{
  E_Expr *expr_arg = expr->first->next;
  E_IRTreeAndType result = e_irtree_and_type_from_expr(arena, expr_arg);
  return result;
}

internal E_IRGenRuleMap
e_irgen_rule_map_make(Arena *arena, U64 slots_count)
{
  E_IRGenRuleMap map = {0};
  map.slots_count = slots_count;
  map.slots = push_array(arena, E_IRGenRuleSlot, map.slots_count);
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("default"),  .irgen = E_IRGEN_FUNCTION_NAME(default));
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("bswap"),    .irgen = E_IRGEN_FUNCTION_NAME(bswap));
  e_irgen_rule_map_insert_new(arena, &map, str8_lit("array"),    .irgen = E_IRGEN_FUNCTION_NAME(array));
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
    E_Parse parse = e_parse_type_from_text_tokens(scratch.arena, params->type_pattern, tokens);
    E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, parse.exprs.last);
    type_key = irtree.type_key;
  }
  E_AutoHookNode *node = push_array(arena, E_AutoHookNode, 1);
  node->type_string = str8_skip_chop_whitespace(e_type_string_from_key(arena, type_key));
  U8 pattern_split = '?';
  node->type_pattern_parts = str8_split(arena, params->type_pattern, &pattern_split, 1, StringSplitFlag_KeepEmpties);
  node->tag_exprs = e_parse_expr_from_text(arena, push_str8_copy(arena, params->tag_expr_string)).exprs;
  if(!e_type_key_match(e_type_key_zero(), type_key))
  {
    U64 hash = e_hash_from_string(5381, node->type_string);
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
    String8 type_string = str8_skip_chop_whitespace(e_type_string_from_key(scratch.arena, type_key));
    
    //- rjf: gather exact-type-key-matches from the map
    if(map != 0 && map->slots_count != 0)
    {
      U64 hash = e_hash_from_string(5381, type_string);
      U64 slot_idx = hash%map->slots_count;
      for(E_AutoHookNode *n = map->slots[slot_idx].first; n != 0; n = n->hash_next)
      {
        if(str8_match(n->type_string, type_string, 0))
        {
          for(E_Expr *e = n->tag_exprs.first; e != &e_expr_nil; e = e->next)
          {
            e_expr_list_push(arena, &exprs, e);
          }
        }
      }
    }
    
    //- rjf: gather fuzzy matches from all patterns in the map
    if(map != 0 && map->first_pattern != 0)
    {
      for(E_AutoHookNode *auto_hook_node = map->first_pattern;
          auto_hook_node != 0;
          auto_hook_node = auto_hook_node->pattern_order_next)
      {
        B32 fits_this_type_string = 1;
        U64 scan_pos = 0;
        for(String8Node *n = auto_hook_node->type_pattern_parts.first; n != 0; n = n->next)
        {
          if(n->string.size == 0)
          {
            continue;
          }
          U64 pattern_part_pos = str8_find_needle(type_string, scan_pos, n->string, 0);
          if(pattern_part_pos >= type_string.size)
          {
            fits_this_type_string = 0;
            break;
          }
          scan_pos = pattern_part_pos + n->string.size;
        }
        if(fits_this_type_string)
        {
          for(E_Expr *e = auto_hook_node->tag_exprs.first; e != &e_expr_nil; e = e->next)
          {
            e_expr_list_push(arena, &exprs, e);
          }
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
//~ rjf: Evaluated String IDs

internal U64
e_id_from_string(String8 string)
{
  U64 hash = e_hash_from_string(5381, string);
  U64 hash_slot_idx = hash%e_ir_state->string_id_map->hash_slots_count;
  E_StringIDNode *node = 0;
  for(E_StringIDNode *n = e_ir_state->string_id_map->hash_slots[hash_slot_idx].first; n != 0; n = n->hash_next)
  {
    if(str8_match(n->string, string, 0))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    e_ir_state->string_id_gen += 1;
    U64 id = e_ir_state->string_id_gen;
    U64 id_slot_idx = id%e_ir_state->string_id_map->id_slots_count;
    node = push_array(e_ir_state->arena, E_StringIDNode, 1);
    SLLQueuePush_N(e_ir_state->string_id_map->hash_slots[hash_slot_idx].first, e_ir_state->string_id_map->hash_slots[hash_slot_idx].last, node, hash_next);
    SLLQueuePush_N(e_ir_state->string_id_map->id_slots[id_slot_idx].first, e_ir_state->string_id_map->hash_slots[id_slot_idx].last, node, id_next);
    node->id = id;
    node->string = push_str8_copy(e_ir_state->arena, string);
  }
  U64 result = node->id;
  return result;
}

internal String8
e_string_from_id(U64 id)
{
  U64 id_slot_idx = id%e_ir_state->string_id_map->id_slots_count;
  E_StringIDNode *node = 0;
  for(E_StringIDNode *n = e_ir_state->string_id_map->id_slots[id_slot_idx].first; n != 0; n = n->id_next)
  {
    if(n->id == id)
    {
      node = n;
      break;
    }
  }
  String8 result = {0};
  if(node != 0)
  {
    result = node->string;
  }
  return result;
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

//- rjf: rule tag poison checking

internal B32
e_tag_is_poisoned(E_Expr *tag)
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
  return tag_is_poisoned;
}

internal void
e_tag_poison(E_Expr *tag)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&tag));
  U64 slot_idx = hash%e_ir_state->used_tag_map->slots_count;
  E_UsedTagNode *n = push_array(e_ir_state->arena, E_UsedTagNode, 1);
  n->tag = tag;
  DLLPushBack(e_ir_state->used_tag_map->slots[slot_idx].first, e_ir_state->used_tag_map->slots[slot_idx].last, n);
}

internal void
e_tag_unpoison(E_Expr *tag)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&tag));
  U64 slot_idx = hash%e_ir_state->used_tag_map->slots_count;
  for(E_UsedTagNode *n = e_ir_state->used_tag_map->slots[slot_idx].first; n != 0; n = n->next)
  {
    if(n->tag == tag)
    {
      DLLRemove(e_ir_state->used_tag_map->slots[slot_idx].first, e_ir_state->used_tag_map->slots[slot_idx].last, n);
      break;
    }
  }
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
      E_LookupRuleTagPair lhs_lookup_rule_and_tag = e_lookup_rule_tag_pair_from_expr_irtree(lhs, &lhs_irtree);
      ProfScope("lookup via rule '%.*s'", str8_varg(lhs_lookup_rule_and_tag.rule->name))
      {
        e_tag_poison(lhs_lookup_rule_and_tag.tag);
        E_LookupInfo lookup_info = lhs_lookup_rule_and_tag.rule->info(arena, &lhs_irtree, lhs_lookup_rule_and_tag.tag, str8_zero());
        E_LookupAccess lookup_access = lhs_lookup_rule_and_tag.rule->access(arena, expr->kind, lhs, rhs, lhs_lookup_rule_and_tag.tag, lookup_info.user_data);
        result = lookup_access.irtree_and_type;
        e_tag_unpoison(lhs_lookup_rule_and_tag.tag);
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
    case E_ExprKind_LogNot:
    {
      // rjf: unpack operand
      E_Expr *r_expr = expr->first;
      E_IRTreeAndType r_tree = e_irtree_and_type_from_expr(arena, r_expr);
      E_TypeKey r_type = e_type_unwrap(r_tree.type_key);
      E_TypeKind r_type_kind = e_type_kind_from_key(r_type);
      RDI_EvalTypeGroup r_type_group = e_type_group_from_kind(r_type_kind);
      E_TypeKey r_type_promoted = e_type_key_basic(E_TypeKind_Bool);
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
    
    //- rjf: call
    case E_ExprKind_Call:
    {
      E_Expr *lhs = expr->first;
      E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(arena, lhs);
      E_TypeKey lhs_type_key = lhs_irtree.type_key;
      E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
      if(lhs_type->kind == E_TypeKind_Stub)
      {
        Temp scratch = scratch_begin(&arena, 1);
        E_OpList oplist = e_oplist_from_irtree(scratch.arena, lhs_irtree.root);
        String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
        E_Interpretation interp = e_interpret(bytecode);
        String8 name = e_string_from_id(interp.value.u64);
        E_IRGenRule *irgen_rule = e_irgen_rule_from_string(name);
        if(irgen_rule != &e_irgen_rule__default)
        {
          result = irgen_rule->irgen(arena, expr);
        }
        else
        {
          e_msgf(arena, &result.msgs, E_MsgKind_InterpretationError, expr->location, "There is no rule named `%S`.", name);
        }
        scratch_end(scratch);
      }
      else
      {
        e_msgf(arena, &result.msgs, E_MsgKind_InterpretationError, expr->location, "Calling this type is not currently supported.");
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
      
      //- rjf: form namespaceified fallback versions of this lookup string
      String8List namespaceified_strings = {0};
      {
        E_Module *module = e_ir_state->ctx->primary_module;
        RDI_Parsed *rdi = module->rdi;
        RDI_Procedure *procedure = e_ir_state->thread_ip_procedure;
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
      
      //- rjf: try to map name as member - if found, string__redirected := "this", and turn
      // on later implicit-member-lookup generation
      B32 string_is_implicit_member_name = 0;
      if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("member"), 0)))
      {
        E_Module *module = e_ir_state->ctx->primary_module;
        U32 module_idx = (U32)(module - e_ir_state->ctx->modules);
        RDI_Parsed *rdi = module->rdi;
        RDI_Procedure *procedure = e_ir_state->thread_ip_procedure;
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
        E_Module *module = e_ir_state->ctx->primary_module;
        U32 module_idx = (U32)(module - e_ir_state->ctx->modules);
        RDI_Parsed *rdi = module->rdi;
        U64 local_num = e_num_from_string(e_ir_state->ctx->locals_map, string__redirected);
        if(local_num != 0)
        {
          RDI_Local *local = rdi_element_from_name_idx(rdi, Locals, local_num-1);
          
          // rjf: extract local's type key
          RDI_TypeNode *type_node = rdi_element_from_name_idx(rdi, TypeNodes, local->type_idx);
          mapped_type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), local->type_idx, module_idx);
          
          // rjf: extract local's location block
          U64 ip_voff = e_ir_state->ctx->thread_ip_voff;
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
      E_Mode mapped_bytecode_mode = E_Mode_Offset;
      E_Space mapped_bytecode_space = zero_struct;
      String8 mapped_bytecode = {0};
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
        for(U64 module_idx = 0; module_idx < e_ir_state->ctx->modules_count; module_idx += 1)
        {
          E_Module *module = &e_ir_state->ctx->modules[module_idx];
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
        for(U64 module_idx = 0; module_idx < e_ir_state->ctx->modules_count; module_idx += 1)
        {
          E_Module *module = &e_ir_state->ctx->modules[module_idx];
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
        for(U64 module_idx = 0; module_idx < e_ir_state->ctx->modules_count; module_idx += 1)
        {
          E_Module *module = &e_ir_state->ctx->modules[module_idx];
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
        U64 reg_num = e_num_from_string(e_ir_state->ctx->regs_map, string);
        if(reg_num != 0)
        {
          string_mapped = 1;
          REGS_Rng reg_rng = regs_reg_code_rng_table_from_arch(e_parse_state->ctx->primary_module->arch)[reg_num];
          E_OpList oplist = {0};
          e_oplist_push_uconst(arena, &oplist, reg_rng.byte_off);
          mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
          mapped_bytecode_mode = E_Mode_Offset;
          mapped_bytecode_space = e_ir_state->ctx->thread_reg_space;
          mapped_type_key = e_type_key_reg(e_parse_state->ctx->primary_module->arch, reg_num);
        }
      }
      
      //- rjf: try register aliases
      if(!string_mapped && (qualifier.size == 0 || str8_match(qualifier, str8_lit("reg"), 0)))
      {
        U64 alias_num = e_num_from_string(e_ir_state->ctx->reg_alias_map, string);
        if(alias_num != 0)
        {
          string_mapped = 1;
          REGS_Slice alias_slice = regs_alias_code_slice_table_from_arch(e_ir_state->ctx->primary_module->arch)[alias_num];
          REGS_Rng alias_reg_rng = regs_reg_code_rng_table_from_arch(e_ir_state->ctx->primary_module->arch)[alias_slice.code];
          E_OpList oplist = {0};
          e_oplist_push_uconst(arena, &oplist, alias_reg_rng.byte_off + alias_slice.byte_off);
          mapped_bytecode = e_bytecode_from_oplist(arena, &oplist);
          mapped_bytecode_mode = E_Mode_Offset;
          mapped_bytecode_space = e_ir_state->ctx->thread_reg_space;
          mapped_type_key = e_type_key_reg_alias(e_parse_state->ctx->primary_module->arch, alias_num);
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
        result.type_key = mapped_type_key;
        result.mode = E_Mode_Null;
      }
      
      //- rjf: generate IR trees for macros
      if(!generated)
      {
        E_Expr *macro_expr = e_string2expr_lookup(e_ir_state->ctx->macro_map, string);
        if(macro_expr != &e_expr_nil)
        {
          generated = 1;
          e_string2expr_map_inc_poison(e_ir_state->ctx->macro_map, string);
          result = e_irtree_and_type_from_expr(arena, macro_expr);
          e_string2expr_map_dec_poison(e_ir_state->ctx->macro_map, string);
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
      if(!generated)
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
        result.type_key = e_type_key_cons(.kind = E_TypeKind_Stub, .name = str8_lit("file"));
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
          result.type_key = e_type_key_cons(.kind = E_TypeKind_Stub, .name = str8_lit("folder"));
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
      if(lhs->kind != E_ExprKind_LeafIdentifier)
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
  B32 default_is_forced = 0;
  E_IRGenRule *explicit_irgen_rule = &e_irgen_rule__default;
  E_Expr *explicit_irgen_rule_tag = &e_expr_nil;
  for(E_Expr *tag = expr->first_tag; tag != &e_expr_nil; tag = tag->next)
  {
    String8 name = tag->string;
    E_IRGenRule *irgen_rule_candidate = e_irgen_rule_from_string(name);
    if(str8_match(name, e_irgen_rule__default.name, 0))
    {
      default_is_forced = 1;
      break;
    }
    if(irgen_rule_candidate != &e_irgen_rule__default)
    {
      B32 tag_is_poisoned = e_tag_is_poisoned(tag);
      if(!tag_is_poisoned)
      {
        explicit_irgen_rule = irgen_rule_candidate;
        explicit_irgen_rule_tag = tag;
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
    e_tag_poison(t->tag);
    
    // rjf: do this rule's generation
    ProfScope("irgen rule '%.*s'", str8_varg(t->rule->name))
    {
      result = t->rule->irgen(arena, expr);
      if(result.root == &e_irnode_nil && t->rule != &e_irgen_rule__default)
      {
        result = e_irgen_rule__default.irgen(arena, expr);
      }
    }
    
    // rjf: find any auto hooks according to this generation's type
    if(!default_is_forced)
    {
      E_ExprList exprs = e_auto_hook_tag_exprs_from_type_key__cached(result.type_key);
      for(E_ExprNode *n = exprs.first; n != 0; n = n->next)
      {
        for(E_Expr *tag = n->v; tag != &e_expr_nil; tag = tag->next)
        {
          B32 tag_is_poisoned = e_tag_is_poisoned(tag);
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
    e_tag_unpoison(t->tag);
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
  lhs_bytecode->string = e_string_from_expr(arena, lhs);
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
  lhs_bytecode->string = e_string_from_expr(arena, lhs);
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
  rhs_bytecode->string = e_string_from_expr(arena, rhs);
  rhs_bytecode->space = rhs->space;
  rhs_bytecode->mode = rhs_irtree->mode;
  rhs_bytecode->type_key = rhs_irtree->type_key;
  rhs_bytecode->bytecode = e_bytecode_from_oplist(arena, &rhs_oplist);
  e_expr_push_child(root, rhs_bytecode);
  return root;
}

internal E_Expr *
e_expr_irext_cast(Arena *arena, E_Expr *rhs, E_IRTreeAndType *rhs_irtree, E_TypeKey type_key)
{
  E_Expr *root = e_push_expr(arena, E_ExprKind_Cast, 0);
  E_Expr *rhs_bytecode = e_push_expr(arena, E_ExprKind_LeafBytecode, rhs->location);
  E_OpList rhs_oplist = e_oplist_from_irtree(arena, rhs_irtree->root);
  rhs_bytecode->string = e_string_from_expr(arena, rhs);
  rhs_bytecode->space = rhs->space;
  rhs_bytecode->mode = rhs_irtree->mode;
  rhs_bytecode->type_key = rhs_irtree->type_key;
  rhs_bytecode->bytecode = e_bytecode_from_oplist(arena, &rhs_oplist);
  E_Expr *lhs = e_push_expr(arena, E_ExprKind_TypeIdent, 0);
  lhs->type_key = type_key;
  e_expr_push_child(root, lhs);
  e_expr_push_child(root, rhs_bytecode);
  return root;
}

////////////////////////////////
//~ rjf: Expression & IR-Tree => Lookup Rule

internal E_LookupRuleTagPair
e_lookup_rule_tag_pair_from_expr_irtree(E_Expr *expr, E_IRTreeAndType *irtree)
{
  E_LookupRuleTagPair result = {&e_lookup_rule__default, &e_expr_nil};
  {
    // rjf: first try explicitly-stored tags
    B32 default_is_forced = 0;
    if(result.rule == &e_lookup_rule__default)
    {
      for(E_Expr *tag = expr->first_tag; tag != &e_expr_nil; tag = tag->next)
      {
        if(e_tag_is_poisoned(tag)) { continue; }
        if(str8_match(tag->string, e_lookup_rule__default.name, 0))
        {
          result.rule = &e_lookup_rule__default;
          result.tag = &e_expr_nil;
          default_is_forced = 1;
          break;
        }
        E_LookupRule *candidate = e_lookup_rule_from_string(tag->string);
        if(candidate != &e_lookup_rule__nil)
        {
          result.rule = candidate;
          result.tag = tag;
        }
      }
    }
    
    // rjf: next try implicit set name -> rule mapping
    if(!default_is_forced && result.rule == &e_lookup_rule__default)
    {
      E_TypeKind type_kind = e_type_kind_from_key(irtree->type_key);
      if(type_kind == E_TypeKind_Stub)
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
    if(!default_is_forced && result.rule == &e_lookup_rule__default)
    {
      E_ExprList tags = e_auto_hook_tag_exprs_from_type_key__cached(irtree->type_key);
      for(E_ExprNode *n = tags.first; n != 0; n = n->next)
      {
        if(e_tag_is_poisoned(n->v)) { continue; }
        E_LookupRule *candidate = e_lookup_rule_from_string(n->v->string);
        if(candidate != &e_lookup_rule__nil)
        {
          result.rule = candidate;
          result.tag = n->v;
        }
      }
    }
  }
  return result;
}
