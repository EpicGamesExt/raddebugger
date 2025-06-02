// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: `commands` Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(commands)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_MemberAccess)
  {
    String8 cmd_name = expr->first->next->string;
    RD_CmdKindInfo *cmd_info = rd_cmd_kind_info_from_string(cmd_name);
    E_TypeKey cmd_type = e_type_key_cons(.kind = E_TypeKind_U64, .name = str8_lit("command"));
    cmd_type = e_type_key_cons_meta_description(cmd_type, cmd_info->description);
    result.type_key = cmd_type;
    result.mode = E_Mode_Value;
    result.root = e_irtree_set_space(arena, e_space_make(RD_EvalSpaceKind_MetaCmd), e_irtree_const_u(arena, e_id_from_string(cmd_name)));
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(commands)
{
  E_TypeExpandInfo result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List cmd_names = {0};
    E_Type *type = e_type_from_key(eval.irtree.type_key);
    for EachNonZeroEnumVal(RD_CmdKind, k)
    {
      RD_CmdKindInfo *info = &rd_cmd_kind_info_table[k];
      if(info->flags & RD_CmdKindFlag_ListInUI &&
         (!str8_match(type->name, str8_lit("text_pt_commands"), 0) || info->flags & RD_CmdKindFlag_ListInTextPt) &&
         (!str8_match(type->name, str8_lit("text_range_commands"), 0) || info->flags & RD_CmdKindFlag_ListInTextRng) &&
         (!str8_match(type->name, str8_lit("tab_commands"), 0) || info->flags & RD_CmdKindFlag_ListInTab))
      {
        String8 code_name = info->string;
        String8 description = info->description;
        String8 search_tags = info->search_tags;
        String8 display_name = rd_display_from_code_name(code_name);
        FuzzyMatchRangeList desc_matches = fuzzy_match_find(scratch.arena, filter, description);
        FuzzyMatchRangeList name_matches = fuzzy_match_find(scratch.arena, filter, display_name);
        FuzzyMatchRangeList tags_matches = fuzzy_match_find(scratch.arena, filter, search_tags);
        B32 binding_matches_good = 0;
        RD_KeyMapNodePtrList bindings = rd_key_map_node_ptr_list_from_name(scratch.arena, code_name);
        for(RD_KeyMapNodePtr *n = bindings.first; n != 0; n = n->next)
        {
          String8 binding_text = os_string_from_modifiers_key(scratch.arena, n->v->binding.modifiers, n->v->binding.key);
          FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, binding_text);
          if(matches.count == matches.needle_part_count)
          {
            binding_matches_good = 1;
            break;
          }
        }
        if(name_matches.count == name_matches.needle_part_count ||
           desc_matches.count == desc_matches.needle_part_count ||
           tags_matches.count == tags_matches.needle_part_count ||
           binding_matches_good)
        {
          str8_list_push(scratch.arena, &cmd_names, code_name);
        }
      }
    }
    String8Array *accel = push_array(arena, String8Array, 1);
    *accel = str8_array_from_list(arena, &cmd_names);
    result.user_data = accel;
    result.expr_count = accel->count;
    scratch_end(scratch);
  }
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(commands)
{
  U64 out_idx = 0;
  String8Array *accel = (String8Array *)user_data;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    String8 cmd_name = accel->v[idx];
    E_Eval cmd_eval = e_eval_from_stringf("query:commands.%S", cmd_name);
    evals_out[out_idx] = cmd_eval;
  }
}

////////////////////////////////
//~ rjf: `themes` Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(themes)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_ArrayIndex &&
     expr->first->next->kind == E_ExprKind_LeafStringLiteral)
  {
    String8 theme_name = expr->first->next->string;
    E_TypeKey theme_type = e_type_key_cons(.kind = E_TypeKind_U64, .name = str8_lit("theme"));
    result.type_key = theme_type;
    result.mode = E_Mode_Value;
    result.root = e_irtree_set_space(arena, e_space_make(RD_EvalSpaceKind_MetaTheme), e_irtree_const_u(arena, e_id_from_string(theme_name)));
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(themes)
{
  E_TypeExpandInfo result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: gather presets
    String8List names = {0};
    for EachEnumVal(RD_ThemePreset, p)
    {
      String8 name = rd_theme_preset_display_string_table[p];
      FuzzyMatchRangeList name_matches = fuzzy_match_find(scratch.arena, filter, name);
      if(name_matches.count == name_matches.needle_part_count)
      {
        str8_list_push(scratch.arena, &names, name);
      }
    }
    
    //- rjf: gather theme files
    {
      String8 theme_folder = push_str8f(scratch.arena, "%S/raddbg/themes", os_get_process_info()->user_program_data_path);
      OS_FileIter *it = os_file_iter_begin(scratch.arena, theme_folder, OS_FileIterFlag_SkipFolders);
      for(OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, it, &info);)
      {
        String8 name = info.name;
        FuzzyMatchRangeList name_matches = fuzzy_match_find(scratch.arena, filter, name);
        if(name_matches.count == name_matches.needle_part_count)
        {
          str8_list_push(scratch.arena, &names, push_str8_copy(arena, name));
        }
      }
      os_file_iter_end(it);
    }
    
    //- rjf: flatten & build accelerator
    String8Array *accel = push_array(arena, String8Array, 1);
    *accel = str8_array_from_list(arena, &names);
    result.user_data = accel;
    result.expr_count = accel->count;
    scratch_end(scratch);
  }
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(themes)
{
  U64 out_idx = 0;
  String8Array *accel = (String8Array *)user_data;
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    String8 name = accel->v[idx];
    evals_out[out_idx] = e_eval_wrapf(eval, "$[\"%S\"]", name);
  }
}

////////////////////////////////
//~ rjf: `locals` Type Hooks

E_TYPE_EXPAND_INFO_FUNCTION_DEF(locals)
{
  E_TypeExpandInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    E_String2NumMapNodeArray nodes = e_string2num_map_node_array_from_map(scratch.arena, e_ir_ctx->locals_map);
    e_string2num_map_node_array_sort__in_place(&nodes);
    String8List exprs_filtered = {0};
    for EachIndex(idx, nodes.count)
    {
      String8 local_expr_string = nodes.v[idx]->string;
      FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, local_expr_string);
      if(matches.count == matches.needle_part_count)
      {
        str8_list_push(scratch.arena, &exprs_filtered, local_expr_string);
      }
    }
    String8Array *accel = push_array(arena, String8Array, 1);
    *accel = str8_array_from_list(arena, &exprs_filtered);
    result.user_data = accel;
    result.expr_count = accel->count;
  }
  scratch_end(scratch);
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(locals)
{
  String8Array *accel = (String8Array *)user_data;
  Rng1U64 legal_idx_range = r1u64(0, accel->count);
  Rng1U64 read_range = intersect_1u64(idx_range, legal_idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    String8 expr_string = accel->v[read_range.min + idx];
    evals_out[idx] = e_eval_from_string(expr_string);
  }
}

////////////////////////////////
//~ rjf: `registers` Type Hooks

E_TYPE_EXPAND_INFO_FUNCTION_DEF(registers)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_Entity *thread = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, rd_regs()->thread);
  Arch arch = thread->arch;
  U64 reg_count     = regs_reg_code_count_from_arch(arch);
  U64 alias_count   = regs_alias_code_count_from_arch(arch);
  String8 *reg_strings   = regs_reg_code_string_table_from_arch(arch);
  String8 *alias_strings = regs_alias_code_string_table_from_arch(arch);
  String8List exprs_list = {0};
  for(U64 idx = 1; idx < reg_count; idx += 1)
  {
    FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, reg_strings[idx]);
    if(matches.count == matches.needle_part_count)
    {
      str8_list_push(scratch.arena, &exprs_list, reg_strings[idx]);
    }
  }
  for(U64 idx = 1; idx < alias_count; idx += 1)
  {
    FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, alias_strings[idx]);
    if(matches.count == matches.needle_part_count)
    {
      str8_list_push(scratch.arena, &exprs_list, alias_strings[idx]);
    }
  }
  String8Array *accel = push_array(arena, String8Array, 1);
  *accel = str8_array_from_list(arena, &exprs_list);
  E_TypeExpandInfo info = {accel, accel->count};
  scratch_end(scratch);
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(registers)
{
  String8Array *accel = (String8Array *)user_data;
  Rng1U64 legal_idx_range = r1u64(0, accel->count);
  Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    String8 register_name = accel->v[read_range.min + idx];
    String8 register_expr = push_str8f(arena, "reg:%S", register_name);
    evals_out[idx] = e_eval_from_string(register_expr);
  }
}

////////////////////////////////
//~ rjf: Schema Type Hooks

typedef struct RD_SchemaIRExt RD_SchemaIRExt;
struct RD_SchemaIRExt
{
  RD_Cfg *cfg;
  CTRL_Entity *entity;
  MD_NodePtrList schemas;
};

E_TYPE_IREXT_FUNCTION_DEF(schema)
{
  RD_SchemaIRExt *ext = push_array(arena, RD_SchemaIRExt, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, irtree->root);
    String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
    E_Interpretation interpret = e_interpret(bytecode);
    E_TypeKey type_key = irtree->type_key;
    E_Type *type = e_type_from_key(type_key);
    ext->cfg = rd_cfg_from_eval_space(interpret.space);
    ext->entity = rd_ctrl_entity_from_eval_space(interpret.space);
    ext->schemas = rd_schemas_from_name(type->name);
    scratch_end(scratch);
  }
  E_IRExt result = {ext};
  return result;
}

E_TYPE_ACCESS_FUNCTION_DEF(schema)
{
  RD_SchemaIRExt *ext = (RD_SchemaIRExt *)lhs_irtree->user_data;
  E_IRTreeAndType irtree = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_MemberAccess)
  {
    MD_Node *child_schema = &md_nil_node;
    for(MD_NodePtrNode *n = ext->schemas.first; n != 0; n = n->next)
    {
      for MD_EachNode(child, n->v->first)
      {
        if(str8_match(child->string, expr->first->next->string, 0))
        {
          child_schema = child;
          break;
        }
      }
    }
    if(child_schema != &md_nil_node)
    {
      RD_Cfg *cfg = ext->cfg;
      CTRL_Entity *entity = ext->entity;
      RD_Cfg *child = rd_cfg_child_from_string(cfg, child_schema->string);
      E_TypeKey child_type_key = zero_struct;
      B32 wrap_child_w_meta_expr = 0;
      if(0){}
      
      //- rjf: ctrl entity members
      else if(entity != &ctrl_entity_nil && str8_match(child_schema->string, str8_lit("label"), 0))
      {
        child_type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), entity->string.size, E_TypeFlag_IsCodeText);
      }
      else if(entity != &ctrl_entity_nil && str8_match(child_schema->string, str8_lit("exe"), 0))
      {
        child_type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), entity->string.size, E_TypeFlag_IsPathText);
      }
      else if(entity != &ctrl_entity_nil && str8_match(child_schema->string, str8_lit("dbg"), 0))
      {
        CTRL_Entity *dbg = ctrl_entity_child_from_kind(entity, CTRL_EntityKind_DebugInfoPath);
        child_type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), dbg->string.size, E_TypeFlag_IsPathText);
      }
      
      //- rjf: cfg members
      else if(str8_match(child_schema->first->string, str8_lit("code_string"), 0) ||
              str8_match(child_schema->first->string, str8_lit("expr_string"), 0))
      {
        child_type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), child->first->string.size, E_TypeFlag_IsCodeText);
      }
      else if(str8_match(child_schema->first->string, str8_lit("path"), 0) ||
              str8_match(child_schema->first->string, str8_lit("path_pt"), 0))
      {
        child_type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), child->first->string.size, E_TypeFlag_IsPathText);
      }
      
      else if(str8_match(child_schema->first->string, str8_lit("string"), 0))
      {
        child_type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), child->first->string.size, E_TypeFlag_IsPlainText);
      }
      
      //- rjf: catchall cases
      else if(str8_match(child_schema->first->string, str8_lit("u64"), 0))
      {
        child_type_key = e_type_key_basic(E_TypeKind_U64);
        wrap_child_w_meta_expr = 1;
      }
      else if(str8_match(child_schema->first->string, str8_lit("u32"), 0))
      {
        child_type_key = e_type_key_basic(E_TypeKind_U32);
        wrap_child_w_meta_expr = 1;
      }
      else if(str8_match(child_schema->first->string, str8_lit("f32"), 0))
      {
        child_type_key = e_type_key_basic(E_TypeKind_F32);
        wrap_child_w_meta_expr = 1;
      }
      else if(str8_match(child_schema->first->string, str8_lit("bool"), 0))
      {
        child_type_key = e_type_key_basic(E_TypeKind_Bool);
        wrap_child_w_meta_expr = 1;
      }
      else if(str8_match(child_schema->first->string, str8_lit("vaddr_range"), 0))
      {
        Temp scratch = scratch_begin(&arena, 1);
        E_MemberList vaddr_range_members_list = {0};
        e_member_list_push_new(scratch.arena, &vaddr_range_members_list, .type_key = e_type_key_basic(E_TypeKind_U64), .name = str8_lit("min"), .off = 0);
        e_member_list_push_new(scratch.arena, &vaddr_range_members_list, .type_key = e_type_key_basic(E_TypeKind_U64), .name = str8_lit("max"), .off = 8);
        E_MemberArray vaddr_range_members = e_member_array_from_list(scratch.arena, &vaddr_range_members_list);
        child_type_key = e_type_key_cons(.kind = E_TypeKind_Struct, .name = str8_lit("vaddr_range"), .count = vaddr_range_members.count, .members = vaddr_range_members.v);
        scratch_end(scratch);
      }
      else if(str8_match(child_schema->first->string, str8_lit("query"), 0))
      {
        child_type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, child_schema->string);
      }
      
      //- rjf: extend child type with meta-expression information
      if(wrap_child_w_meta_expr)
      {
        Temp scratch = scratch_begin(&arena, 1);
        E_Expr *expr = e_parse_from_string(child->first->string).expr;
        B32 expr_is_simple = 0;
        if(expr->kind == E_ExprKind_LeafU64 ||
           expr->kind == E_ExprKind_LeafF64 ||
           expr->kind == E_ExprKind_LeafF32)
        {
          expr_is_simple = 1;
        }
        if((expr->kind == E_ExprKind_Pos || expr->kind == E_ExprKind_Neg) &&
           expr->first == expr->last &&
           (expr->first->kind == E_ExprKind_LeafU64 ||
            expr->first->kind == E_ExprKind_LeafF64 ||
            expr->first->kind == E_ExprKind_LeafF32))
        {
          expr_is_simple = 1;
        }
        if(expr->kind == E_ExprKind_LeafIdentifier &&
           (str8_match(expr->string, str8_lit("true"), 0) ||
            str8_match(expr->string, str8_lit("false"), 0)))
        {
          expr_is_simple = 1;
        }
        if(!expr_is_simple && expr != &e_expr_nil)
        {
          child_type_key = e_type_key_cons_meta_expr(child_type_key, child->first->string);
        }
        scratch_end(scratch);
      }
      
      //- rjf: extend child type with decorative meta info
      {
        MD_Node *display_name = md_tag_from_string(child_schema, str8_lit("display_name"), 0);
        MD_Node *description = md_tag_from_string(child_schema, str8_lit("description"), 0);
        if(!md_node_is_nil(display_name))
        {
          child_type_key = e_type_key_cons_meta_display_name(child_type_key, display_name->first->string);
        }
        if(!md_node_is_nil(description))
        {
          child_type_key = e_type_key_cons_meta_description(child_type_key, description->first->string);
        }
      }
      
      //- rjf: extend child type with hex lens
      {
        MD_Node *hex = md_tag_from_string(child_schema->first, str8_lit("hex"), 0);
        if(!md_node_is_nil(hex))
        {
          child_type_key = e_type_key_cons(.kind = E_TypeKind_Lens,
                                           .name = str8_lit("hex"),
                                           .direct_key = child_type_key);
        }
      }
      
      //- rjf: extend child type with color lens
      {
        MD_Node *color = md_tag_from_string(child_schema->first, str8_lit("color"), 0);
        if(!md_node_is_nil(color))
        {
          child_type_key = e_type_key_cons(.kind = E_TypeKind_Lens,
                                           .name = str8_lit("color"),
                                           .direct_key = child_type_key);
        }
      }
      
      //- rjf: extend child type with ranges
      {
        MD_Node *range = md_tag_from_string(child_schema->first, str8_lit("range"), 0);
        if(!md_node_is_nil(range))
        {
          E_Expr *min_bound = e_parse_from_string(range->first->string).expr;
          E_Expr *max_bound = e_parse_from_string(range->first->next->string).expr;
          E_Expr *args[] =
          {
            min_bound,
            max_bound,
          };
          child_type_key = e_type_key_cons(.kind = E_TypeKind_Lens,
                                           .name = str8_lit("range1"),
                                           .direct_key = child_type_key,
                                           .count = 2,
                                           .args = args);
        }
      }
      
      //- rjf: evaluate
      E_Space child_eval_space = zero_struct;
      if(cfg != &rd_nil_cfg)
      {
        child_eval_space = e_space_make(RD_EvalSpaceKind_MetaCfg);
        child_eval_space.u64s[0] = cfg->id;
        child_eval_space.u64s[1] = e_id_from_string(child_schema->string);
      }
      else
      {
        child_eval_space = rd_eval_space_from_ctrl_entity(entity, RD_EvalSpaceKind_MetaCtrlEntity);
        child_eval_space.u64s[2] = e_id_from_string(child_schema->string);
      }
      irtree.root     = e_irtree_set_space(arena, child_eval_space, e_push_irnode(arena, RDI_EvalOp_ConstU64));
      irtree.type_key = child_type_key;
      irtree.mode     = E_Mode_Offset;
    }
  }
  return irtree;
}

typedef struct RD_SchemaExpandAccel RD_SchemaExpandAccel;
struct RD_SchemaExpandAccel
{
  String8Array commands;
  MD_Node **children;
  U64 children_count;
};

E_TYPE_EXPAND_INFO_FUNCTION_DEF(schema)
{
  E_TypeExpandInfo result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    // rjf: unpack
    RD_SchemaIRExt *ext = (RD_SchemaIRExt *)eval.irtree.user_data;
    
    // rjf: gather expansion commands
    String8Array commands = {0};
    {
      String8List commands_list = {0};
      for(MD_NodePtrNode *n = ext->schemas.first; n != 0; n = n->next)
      {
        MD_Node *schema = n->v;
        MD_Node *tag = md_tag_from_string(schema, str8_lit("expand_commands"), 0);
        for MD_EachNode(arg, tag->first)
        {
          RD_CmdKindInfo *cmd_kind_info = rd_cmd_kind_info_from_string(arg->string);
          FuzzyMatchRangeList name_matches = fuzzy_match_find(scratch.arena, filter, rd_display_from_code_name(cmd_kind_info->string));
          FuzzyMatchRangeList desc_matches = fuzzy_match_find(scratch.arena, filter, cmd_kind_info->description);
          FuzzyMatchRangeList tags_matches = fuzzy_match_find(scratch.arena, filter, cmd_kind_info->search_tags);
          if(name_matches.count == name_matches.needle_part_count ||
             desc_matches.count == desc_matches.needle_part_count ||
             tags_matches.count == tags_matches.needle_part_count)
          {
            str8_list_push(scratch.arena, &commands_list, arg->string);
          }
        }
      }
      commands = str8_array_from_list(arena, &commands_list);
    }
    
    // rjf: gather expansion children
    typedef struct ExpandChildNode ExpandChildNode;
    struct ExpandChildNode
    {
      ExpandChildNode *next;
      MD_Node *n;
    };
    ExpandChildNode *first_child_node = 0;
    ExpandChildNode *last_child_node = 0;
    U64 child_count = 0;
    for(MD_NodePtrNode *n = ext->schemas.first; n != 0; n = n->next)
    {
      MD_Node *schema = n->v;
      for MD_EachNode(child, schema->first)
      {
        if(!md_node_has_tag(child, str8_lit("no_expand"), 0))
        {
          MD_Node *expand_check = md_tag_from_string(child, str8_lit("expand_if"), 0);
          B32 expand_this_child = 1;
          if(!md_node_is_nil(expand_check)) E_ParentKey(eval.key)
          {
            expand_this_child = !!e_value_from_string(expand_check->first->string).u64;
          }
          if(expand_this_child)
          {
            String8 display_name = md_tag_from_string(child, str8_lit("display_name"), 0)->first->string;
            if(display_name.size == 0)
            {
              display_name = rd_display_from_code_name(child->string);
            }
            String8 desc = md_tag_from_string(child, str8_lit("description"), 0)->first->string;
            FuzzyMatchRangeList name_matches         = fuzzy_match_find(scratch.arena, filter, child->string);
            FuzzyMatchRangeList display_name_matches = fuzzy_match_find(scratch.arena, filter, display_name);
            FuzzyMatchRangeList desc_matches         = fuzzy_match_find(scratch.arena, filter, desc);
            if(name_matches.count == name_matches.needle_part_count ||
               display_name_matches.count == display_name_matches.needle_part_count ||
               desc_matches.count == desc_matches.needle_part_count)
            {
              ExpandChildNode *n = push_array(scratch.arena, ExpandChildNode, 1);
              n->n = child;
              SLLQueuePush(first_child_node, last_child_node, n);
              child_count += 1;
            }
          }
        }
      }
    }
    
    // rjf: flatten expansion member list
    MD_Node **children = push_array(arena, MD_Node *, child_count);
    {
      U64 idx = 0;
      for(ExpandChildNode *n = first_child_node; n != 0; n = n->next, idx += 1)
      {
        children[idx] = n->n;
      }
    }
    
    // rjf: build accelerator for lookups
    RD_SchemaExpandAccel *accel = push_array(arena, RD_SchemaExpandAccel, 1);
    accel->commands = commands;
    accel->children = children;
    accel->children_count = child_count;
    
    // rjf: fill result
    result.user_data = accel;
    result.expr_count = child_count + commands.count;
    
    scratch_end(scratch);
  }
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(schema)
{
  RD_SchemaExpandAccel *accel = (RD_SchemaExpandAccel *)user_data;
  Rng1U64 cmds_idx_range = r1u64(0, accel->commands.count);
  Rng1U64 chld_idx_range = r1u64(cmds_idx_range.max, cmds_idx_range.max + accel->children_count);
  U64 out_idx = 0;
  
  // rjf: read commands
  {
    Rng1U64 read_range = intersect_1u64(idx_range, cmds_idx_range);
    for(U64 idx = read_range.min; idx < read_range.max; idx += 1, out_idx += 1)
    {
      evals_out[out_idx] = e_eval_from_stringf("query:commands.%S", accel->commands.v[idx - cmds_idx_range.min]);
    }
  }
  
  // rjf: read children
  {
    Rng1U64 read_range = intersect_1u64(idx_range, chld_idx_range);
    for(U64 idx = read_range.min; idx < read_range.max; idx += 1, out_idx += 1)
    {
      MD_Node *child_schema = accel->children[idx - chld_idx_range.min];
      evals_out[out_idx] = e_eval_wrapf(eval, "$.%S", child_schema->string);
    }
  }
}

////////////////////////////////
//~ rjf: Config Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(cfgs)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  E_Expr *rhs = expr->first->next;
  if(rhs->kind == E_ExprKind_LeafIdentifier &&
     str8_match(str8_prefix(rhs->string, 1), str8_lit("$"), 0))
  {
    String8 numeric_part = str8_skip(rhs->string, 1);
    RD_CfgID id = u64_from_str8(numeric_part, 16);
    RD_Cfg *cfg = rd_cfg_from_id(id);
    E_Space space = rd_eval_space_from_cfg(cfg);
    result.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, 0));
    result.type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, cfg->string);
    result.mode = E_Mode_Offset;
  }
  return result;
}

////////////////////////////////
//~ rjf: Control Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(control)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  E_Expr *rhs = expr->first->next;
  if(rhs->kind == E_ExprKind_LeafIdentifier &&
     str8_match(str8_prefix(rhs->string, 1), str8_lit("$"), 0))
  {
    CTRL_Handle handle = ctrl_handle_from_string(rhs->string);
    CTRL_Entity *entity = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, handle);
    E_Space space = rd_eval_space_from_ctrl_entity(entity, RD_EvalSpaceKind_MetaCtrlEntity);
    result.root = e_irtree_set_space(arena, space, e_irtree_const_u(arena, 0));
    result.type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, ctrl_entity_kind_code_name_table[entity->kind]);
    result.mode = E_Mode_Offset;
  }
  return result;
}

////////////////////////////////
//~ rjf: Config Collection Type Hooks

typedef struct RD_CfgsIRExt RD_CfgsIRExt;
struct RD_CfgsIRExt
{
  String8 cfg_name;
  String8Array cmds;
  RD_CfgArray cfgs;
  Rng1U64 cmds_idx_range;
  Rng1U64 cfgs_idx_range;
};

E_TYPE_IREXT_FUNCTION_DEF(cfgs_slice)
{
  RD_CfgsIRExt *ext = push_array(arena, RD_CfgsIRExt, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: determine which key we'll be gathering
    E_TypeKey type_key = irtree->type_key;
    E_Type *type = e_type_from_key(type_key);
    String8 cfg_name = rd_singular_from_code_name_plural(type->name);
    
    //- rjf: gather cfgs
    RD_CfgList cfgs_list = rd_cfg_top_level_list_from_string(scratch.arena, cfg_name);
    
    //- rjf: gather commands
    String8List cmds_list = {0};
    {
      MD_NodePtrList schemas = rd_schemas_from_name(cfg_name);
      for(MD_NodePtrNode *n = schemas.first; n != 0; n = n->next)
      {
        MD_Node *schema = n->v;
        MD_Node *collection_cmds_root = md_tag_from_string(schema, str8_lit("collection_commands"), 0);
        for MD_EachNode(cmd, collection_cmds_root->first)
        {
          str8_list_push(arena, &cmds_list, cmd->string);
        }
      }
    }
    
    //- rjf: package & fill
    ext->cfg_name = cfg_name;
    ext->cfgs = rd_cfg_array_from_list(arena, &cfgs_list);
    ext->cmds = str8_array_from_list(arena, &cmds_list);
    
    scratch_end(scratch);
  }
  E_IRExt result = {ext};
  return result;
}

E_TYPE_ACCESS_FUNCTION_DEF(cfgs_slice)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  RD_Cfg *cfg = &rd_nil_cfg;
  RD_CfgsIRExt *ext = (RD_CfgsIRExt *)lhs_irtree->user_data;
  switch(expr->kind)
  {
    default:{}break;
    case E_ExprKind_ArrayIndex:
    {
      E_Value rhs_value = e_value_from_expr(expr->first->next);
      U64 rhs_idx = rhs_value.u64;
      if(0 <= rhs_idx && rhs_idx < ext->cfgs.count)
      {
        cfg = ext->cfgs.v[rhs_idx];
      }
    }break;
    case E_ExprKind_MemberAccess:
    {
      String8 rhs_name = expr->first->next->string;
      RD_CfgID id = 0;
      if(str8_match(str8_prefix(rhs_name, 1), str8_lit("$"), 0))
      {
        id = u64_from_str8(str8_skip(rhs_name, 1), 16);
        cfg = rd_cfg_from_id(id);
      }
    }break;
  }
  if(cfg != &rd_nil_cfg)
  {
    result.root = e_irtree_set_space(arena, rd_eval_space_from_cfg(cfg), e_irtree_const_u(arena, 0));
    result.mode = E_Mode_Offset;
    result.type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, ext->cfg_name);
  }
  return result;
}

typedef struct RD_CfgsExpandAccel RD_CfgsExpandAccel;
struct RD_CfgsExpandAccel
{
  String8Array cmds;
  RD_CfgArray cfgs;
  Rng1U64 cmds_idx_range;
  Rng1U64 cfgs_idx_range;
};

E_TYPE_EXPAND_INFO_FUNCTION_DEF(cfgs_slice)
{
  RD_CfgsExpandAccel *accel = push_array(arena, RD_CfgsExpandAccel, 1);
  E_TypeExpandInfo info = {accel};
  Temp scratch = scratch_begin(&arena, 1);
  {
    //- rjf: unpack
    RD_CfgsIRExt *ext = (RD_CfgsIRExt *)eval.irtree.user_data;
    
    //- rjf: filter cfgs
    RD_CfgArray cfgs__filtered = ext->cfgs;
    if(filter.size != 0)
    {
      RD_CfgList cfgs_list__filtered = {0};
      for EachIndex(idx, ext->cfgs.count)
      {
        RD_Cfg *cfg = ext->cfgs.v[idx];
        DR_FStrList fstrs = rd_title_fstrs_from_cfg(scratch.arena, cfg);
        String8 string = dr_string_from_fstrs(scratch.arena, &fstrs);
        FuzzyMatchRangeList fuzzy_matches = fuzzy_match_find(scratch.arena, filter, string);
        if(fuzzy_matches.count == fuzzy_matches.needle_part_count)
        {
          rd_cfg_list_push(scratch.arena, &cfgs_list__filtered, cfg);
        }
      }
      cfgs__filtered = rd_cfg_array_from_list(arena, &cfgs_list__filtered);
    }
    
    //- rjf: fill
    if(rd_cfg_child_from_string(rd_cfg_from_id(rd_regs()->view), str8_lit("lister")) == &rd_nil_cfg)
    {
      accel->cmds = ext->cmds;
      accel->cmds_idx_range = r1u64(0, accel->cmds.count);
    }
    accel->cfgs = cfgs__filtered;
    accel->cfgs_idx_range = r1u64(accel->cmds_idx_range.max, accel->cmds_idx_range.max + accel->cfgs.count);
    info.expr_count = (accel->cmds.count + accel->cfgs.count);
  }
  scratch_end(scratch);
  return info;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(cfgs_query)
{
  RD_CfgsExpandAccel *accel = push_array(arena, RD_CfgsExpandAccel, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    RD_Cfg *root_cfg = rd_cfg_from_eval_space(eval.space);
    String8 child_key = e_string_from_id(eval.space.u64s[1]);
    String8 child_key_singular = rd_singular_from_code_name_plural(child_key);
    if(child_key_singular.size != 0)
    {
      child_key = child_key_singular;
    }
    String8List cmds = {0};
    MD_NodePtrList schemas = rd_schemas_from_name(child_key);
    for(MD_NodePtrNode *n = schemas.first; n != 0; n = n->next)
    {
      MD_Node *schema = n->v;
      MD_Node *collection_cmds_root = md_tag_from_string(schema, str8_lit("collection_commands"), 0);
      for MD_EachNode(cmd, collection_cmds_root->first)
      {
        str8_list_push(scratch.arena, &cmds, cmd->string);
      }
    }
    RD_CfgList children = rd_cfg_child_list_from_string(scratch.arena, root_cfg, child_key);
    RD_CfgList children__filtered = children;
    if(filter.size != 0)
    {
      MemoryZeroStruct(&children__filtered);
      for(RD_CfgNode *n = children.first; n != 0; n = n->next)
      {
        DR_FStrList cfg_fstrs = rd_title_fstrs_from_cfg(scratch.arena, n->v);
        String8 cfg_string = dr_string_from_fstrs(scratch.arena, &cfg_fstrs);
        FuzzyMatchRangeList ranges = fuzzy_match_find(scratch.arena, filter, cfg_string);
        if(ranges.count == ranges.needle_part_count)
        {
          rd_cfg_list_push(scratch.arena, &children__filtered, n->v);
        }
      }
    }
    accel->cmds = str8_array_from_list(arena, &cmds);
    accel->cmds_idx_range = r1u64(0, accel->cmds.count);
    accel->cfgs = rd_cfg_array_from_list(arena, &children__filtered);
    accel->cfgs_idx_range = r1u64(accel->cmds.count + 0, accel->cmds.count + accel->cfgs.count);
    scratch_end(scratch);
  }
  E_TypeExpandInfo info = {accel, accel->cfgs.count + accel->cmds.count};
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(cfgs_slice)
{
  RD_CfgsExpandAccel *accel = (RD_CfgsExpandAccel *)user_data;
  Rng1U64 cmds_idx_range = accel->cmds_idx_range;
  Rng1U64 cfgs_idx_range = accel->cfgs_idx_range;
  U64 dst_idx = 0;
  
  // rjf: fill commands
  {
    Rng1U64 read_range = intersect_1u64(cmds_idx_range, idx_range);
    U64 read_count = dim_1u64(read_range);
    E_Eval cmds_eval = e_eval_from_stringf("query:commands");
    for(U64 idx = 0; idx < read_count; idx += 1, dst_idx += 1)
    {
      String8 cmd_name = accel->cmds.v[idx + read_range.min - cmds_idx_range.min];
      E_Eval cmd_eval = e_eval_wrapf(cmds_eval, "$.%S", cmd_name);
      evals_out[dst_idx] = cmd_eval;
    }
  }
  
  // rjf: fill cfgs
  {
    Rng1U64 read_range = intersect_1u64(cfgs_idx_range, idx_range);
    U64 read_count = dim_1u64(read_range);
    for(U64 idx = 0; idx < read_count; idx += 1, dst_idx += 1)
    {
      RD_Cfg *cfg = accel->cfgs.v[idx + read_range.min - cfgs_idx_range.min];
      evals_out[dst_idx] = e_eval_from_stringf("query:config.$%I64x", cfg->id);
    }
  }
}

E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(cfgs_slice)
{
  U64 id = 0;
  RD_CfgsExpandAccel *accel = (RD_CfgsExpandAccel *)user_data;
  if(num != 0)
  {
    U64 idx = num-1;
    if(contains_1u64(accel->cfgs_idx_range, idx))
    {
      RD_Cfg *cfg = accel->cfgs.v[idx - accel->cfgs_idx_range.min];
      id = cfg->id;
    }
    else if(contains_1u64(accel->cmds_idx_range, idx))
    {
      id = num;
      id |= (1ull<<63);
    }
  }
  return id;
}

E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(cfgs_slice)
{
  U64 num = 0;
  RD_CfgsExpandAccel *accel = (RD_CfgsExpandAccel *)user_data;
  if(id != 0)
  {
    if(id & (1ull<<63))
    {
      num = id;
      num &= ~(1ull<<63);
    }
    else for EachIndex(idx, accel->cfgs.count)
    {
      if(accel->cfgs.v[idx]->id == id)
      {
        num = idx + accel->cfgs_idx_range.min + 1;
        break;
      }
    }
  }
  return num;
}

////////////////////////////////
//~ rjf: `call_stack` Type Hooks

typedef struct RD_CallStackAccel RD_CallStackAccel;
struct RD_CallStackAccel
{
  Arch arch;
  CTRL_Handle process;
  CTRL_CallStack call_stack;
};

E_TYPE_IREXT_FUNCTION_DEF(call_stack)
{
  RD_CallStackAccel *accel = push_array(arena, RD_CallStackAccel, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, irtree->root);
    String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
    E_Interpretation interp = e_interpret(bytecode);
    CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(interp.space);
    if(entity->kind == CTRL_EntityKind_Thread)
    {
      B32 call_stack_high_priority = ctrl_handle_match(entity->handle, rd_base_regs()->thread);
      accel->arch = entity->arch;
      accel->process = ctrl_process_from_entity(entity)->handle;
      accel->call_stack = ctrl_call_stack_from_thread(rd_state->frame_ctrl_scope, &d_state->ctrl_entity_store->ctx, entity, call_stack_high_priority, call_stack_high_priority ? rd_state->frame_eval_memread_endt_us : 0);
    }
    scratch_end(scratch);
  }
  E_IRExt result = {accel};
  return result;
}

E_TYPE_ACCESS_FUNCTION_DEF(call_stack)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_ArrayIndex)
  {
    RD_CallStackAccel *accel = (RD_CallStackAccel *)lhs_irtree->user_data;
    E_Value rhs_value = e_value_from_expr(expr->first->next);
    CTRL_CallStack *call_stack = &accel->call_stack;
    if(0 <= rhs_value.u64 && rhs_value.u64 < call_stack->frames_count)
    {
      CTRL_Entity *process = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, accel->process);
      CTRL_CallStackFrame *f = &call_stack->frames[rhs_value.u64];
      result.root = e_irtree_set_space(arena, rd_eval_space_from_ctrl_entity(process, RD_EvalSpaceKind_CtrlEntity), e_irtree_const_u(arena, regs_rip_from_arch_block(accel->arch, f->regs)));
      result.type_key = e_type_key_cons(.arch = process->arch, .kind = E_TypeKind_Ptr, .direct_key = e_type_key_basic(E_TypeKind_Function), .count = 1, .depth = f->inline_depth);
      result.mode = E_Mode_Value;
    }
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(call_stack)
{
  RD_CallStackAccel *accel = (RD_CallStackAccel *)eval.irtree.user_data;
  E_TypeExpandInfo result = {0};
  result.user_data = accel;
  result.expr_count = accel->call_stack.frames_count;
  return result;
}

////////////////////////////////
//~ rjf: `environment` Type Hooks

typedef struct RD_EnvironmentAccel RD_EnvironmentAccel;
struct RD_EnvironmentAccel
{
  RD_CfgArray cfgs;
};

E_TYPE_IREXT_FUNCTION_DEF(environment)
{
  RD_EnvironmentAccel *accel = push_array(arena, RD_EnvironmentAccel, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, irtree->root);
    String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
    E_Interpretation interpret = e_interpret(bytecode);
    E_Space space = interpret.space;
    RD_Cfg *target = rd_cfg_from_eval_space(space);
    RD_CfgList env_strings = {0};
    for(RD_Cfg *child = target->first; child != &rd_nil_cfg; child = child->next)
    {
      if(str8_match(child->string, str8_lit("environment"), 0))
      {
        rd_cfg_list_push(scratch.arena, &env_strings, child);
      }
    }
    accel->cfgs = rd_cfg_array_from_list(arena, &env_strings);
    scratch_end(scratch);
  }
  E_IRExt result = {accel};
  return result;
}

E_TYPE_ACCESS_FUNCTION_DEF(environment)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_ArrayIndex)
  {
    RD_EnvironmentAccel *accel = (RD_EnvironmentAccel *)lhs_irtree->user_data;
    RD_CfgArray *cfgs = &accel->cfgs;
    E_Value rhs_value = e_value_from_expr(expr->first->next);
    if(0 <= rhs_value.u64 && rhs_value.u64 < cfgs->count)
    {
      RD_Cfg *cfg = cfgs->v[rhs_value.u64];
      result.root      = e_irtree_set_space(arena, rd_eval_space_from_cfg(cfg), e_irtree_const_u(arena, 0));
      result.type_key  = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), cfg->first->string.size, E_TypeFlag_IsCodeText);
      result.mode      = E_Mode_Offset;
    }
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(environment)
{
  RD_EnvironmentAccel *accel = (RD_EnvironmentAccel *)eval.irtree.user_data;
  E_TypeExpandInfo result = {accel, accel->cfgs.count + 1};
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(environment)
{
  RD_EnvironmentAccel *accel = (RD_EnvironmentAccel *)user_data;
  Rng1U64 legal_idx_range = r1u64(0, accel->cfgs.count);
  Rng1U64 read_range = intersect_1u64(idx_range, legal_idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    U64 cfg_idx = read_range.min + idx;
    if(cfg_idx < accel->cfgs.count)
    {
      evals_out[idx] = e_eval_wrapf(eval, "$[%I64u]", cfg_idx);
    }
  }
}

E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(environment)
{
  U64 id = 0;
  RD_EnvironmentAccel *accel = (RD_EnvironmentAccel *)user_data;
  if(1 <= num && num <= accel->cfgs.count)
  {
    U64 idx = (num-1);
    id = accel->cfgs.v[idx]->id;
  }
  else if(num == accel->cfgs.count+1)
  {
    id = max_U64;
  }
  return id;
}

E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(environment)
{
  U64 num = 0;
  RD_EnvironmentAccel *accel = (RD_EnvironmentAccel *)user_data;
  if(id != 0 && id != max_U64)
  {
    for EachIndex(idx, accel->cfgs.count)
    {
      if(accel->cfgs.v[idx]->id == id)
      {
        num = idx+1;
        break;
      }
    }
  }
  else if(id == max_U64)
  {
    num = accel->cfgs.count + 1;
  }
  return num;
}

////////////////////////////////
//~ rjf: `watches` Type Hooks

typedef struct RD_WatchesAccel RD_WatchesAccel;
struct RD_WatchesAccel
{
  RD_CfgArray cfgs;
};

E_TYPE_IREXT_FUNCTION_DEF(watches)
{
  RD_WatchesAccel *accel = push_array(arena, RD_WatchesAccel, 1);
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, irtree->root);
    String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
    E_Interpretation interpret = e_interpret(bytecode);
    E_Space space = interpret.space;
    RD_Cfg *target = rd_cfg_from_eval_space(space);
    RD_CfgList env_strings = {0};
    for(RD_Cfg *child = target->first; child != &rd_nil_cfg; child = child->next)
    {
      if(str8_match(child->string, str8_lit("watch"), 0))
      {
        rd_cfg_list_push(scratch.arena, &env_strings, child);
      }
    }
    accel->cfgs = rd_cfg_array_from_list(arena, &env_strings);
    scratch_end(scratch);
  }
  E_IRExt result = {accel};
  return result;
}

E_TYPE_ACCESS_FUNCTION_DEF(watches)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_ArrayIndex)
  {
    RD_WatchesAccel *accel = (RD_WatchesAccel *)lhs_irtree->user_data;
    RD_CfgArray *cfgs = &accel->cfgs;
    E_Value rhs_value = e_value_from_expr(expr->first->next);
    if(0 <= rhs_value.u64 && rhs_value.u64 < cfgs->count)
    {
      RD_Cfg *cfg = cfgs->v[rhs_value.u64];
      result.root      = e_irtree_set_space(arena, rd_eval_space_from_cfg(cfg), e_irtree_const_u(arena, 0));
      result.type_key  = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), cfg->first->string.size, E_TypeFlag_IsCodeText);
      result.mode      = E_Mode_Offset;
    }
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(watches)
{
  RD_WatchesAccel *ext = (RD_WatchesAccel *)eval.irtree.user_data;
  RD_WatchesAccel *accel = push_array(arena, RD_WatchesAccel, 1);
  {
    RD_CfgArray cfgs__filtered = ext->cfgs;
    if(filter.size != 0)
    {
      Temp scratch = scratch_begin(&arena, 1);
      RD_CfgList cfgs_list__filtered = {0};
      for EachIndex(idx, ext->cfgs.count)
      {
        RD_Cfg *watch = ext->cfgs.v[idx];
        String8 string = watch->first->string;
        FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, string);
        if(matches.count == matches.needle_part_count)
        {
          rd_cfg_list_push(scratch.arena, &cfgs_list__filtered, watch);
        }
      }
      cfgs__filtered = rd_cfg_array_from_list(arena, &cfgs_list__filtered);
      scratch_end(scratch);
    }
    accel->cfgs = cfgs__filtered;
  }
  E_TypeExpandInfo result = {accel, accel->cfgs.count + 1};
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(watches)
{
  RD_WatchesAccel *accel = (RD_WatchesAccel *)user_data;
  Rng1U64 legal_idx_range = r1u64(0, accel->cfgs.count);
  Rng1U64 read_range = intersect_1u64(idx_range, legal_idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    U64 cfg_idx = read_range.min + idx;
    if(cfg_idx < accel->cfgs.count)
    {
      RD_Cfg *cfg = accel->cfgs.v[cfg_idx];
      evals_out[idx] = e_eval_from_string(cfg->first->string);
    }
  }
}

E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(watches)
{
  U64 id = 0;
  RD_WatchesAccel *accel = (RD_WatchesAccel *)user_data;
  if(1 <= num && num <= accel->cfgs.count)
  {
    U64 idx = (num-1);
    id = accel->cfgs.v[idx]->id;
  }
  else if(num == accel->cfgs.count+1)
  {
    id = max_U64;
  }
  return id;
}

E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(watches)
{
  U64 num = 0;
  RD_WatchesAccel *accel = (RD_WatchesAccel *)user_data;
  if(id != 0 && id != max_U64)
  {
    for EachIndex(idx, accel->cfgs.count)
    {
      if(accel->cfgs.v[idx]->id == id)
      {
        num = idx+1;
        break;
      }
    }
  }
  else if(id == max_U64)
  {
    num = accel->cfgs.count + 1;
  }
  return num;
}

////////////////////////////////
//~ rjf: `unattached_processes` Type Hooks

typedef struct RD_UnattachedProcessesAccel RD_UnattachedProcessesAccel;
struct RD_UnattachedProcessesAccel
{
  DMN_ProcessInfo *infos;
  CTRL_Entity **machines;
  U64 infos_count;
};

E_TYPE_ACCESS_FUNCTION_DEF(unattached_processes)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  if(expr->kind == E_ExprKind_MemberAccess)
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    // rjf: extract pid / name id from access string
    U64 pid = 0;
    U64 string_id = 0;
    {
      String8 pid_string = {0};
      String8 name_id_string = {0};
      U8 split_char = '$';
      String8List parts = str8_split(scratch.arena, expr->first->next->string, &split_char, 1, 0);
      if(parts.first != 0 && parts.first->next != 0)
      {
        pid_string = parts.first->string;
        name_id_string = parts.first->next->string;
        pid = u64_from_str8(pid_string, 16);
        string_id = u64_from_str8(name_id_string, 16);
      }
    }
    
    // rjf: get machine entity from space
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, lhs_irtree->root);
    String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
    E_Interpretation interpret = e_interpret(bytecode);
    CTRL_Entity *machine = rd_ctrl_entity_from_eval_space(interpret.space);
    
    // rjf: build evaluation for this unattached process
    if(machine != &ctrl_entity_nil)
    {
      E_IRNode *value_irnode = e_push_irnode(arena, RDI_EvalOp_ConstU128);
      value_irnode->value.u128.u64[0] = pid;
      value_irnode->value.u128.u64[1] = string_id;
      E_Space space = rd_eval_space_from_ctrl_entity(machine, RD_EvalSpaceKind_MetaUnattachedProcess);
      result.root = e_irtree_set_space(arena, space, value_irnode);
      result.type_key = e_type_key_basic(E_TypeKind_U128);
      result.mode = E_Mode_Value;
    }
    
    scratch_end(scratch);
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(unattached_processes)
{
  E_TypeExpandInfo info = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: evaluate lhs machine, if we have one
    CTRL_Entity *lhs_entity = rd_ctrl_entity_from_eval_space(eval.space);
    
    //- rjf: gather all machines we're searching through
    CTRL_EntityArray machines = {0};
    if(lhs_entity->kind == CTRL_EntityKind_Machine)
    {
      machines.v = &lhs_entity;
      machines.count = 1;
    }
    else
    {
      machines = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, CTRL_EntityKind_Machine);
    }
    
    //- rjf: gather system processes from this machine
    typedef struct Node Node;
    struct Node
    {
      Node *next;
      CTRL_Entity *machine;
      DMN_ProcessInfo info;
    };
    Node *first = 0;
    Node *last = 0;
    U64 count = 0;
    for EachIndex(idx, machines.count)
    {
      CTRL_Entity *machine = machines.v[idx];
      DMN_ProcessIter iter = {0};
      dmn_process_iter_begin(&iter);
      for(DMN_ProcessInfo info = {0}; dmn_process_iter_next(scratch.arena, &iter, &info);)
      {
        B32 passes_filter = 1;
        if(filter.size != 0)
        {
          passes_filter = 0;
          FuzzyMatchRangeList name_matches = fuzzy_match_find(scratch.arena, filter, info.name);
          FuzzyMatchRangeList pid_matches = fuzzy_match_find(scratch.arena, filter, push_str8f(scratch.arena, "%I64u", info.pid));
          if(name_matches.count == name_matches.needle_part_count || pid_matches.count == pid_matches.needle_part_count)
          {
            passes_filter = 1;
          }
        }
        if(passes_filter)
        {
          Node *node = push_array(scratch.arena, Node, 1);
          SLLQueuePush(first, last, node);
          node->machine = machine;
          node->info = info;
          count += 1;
        }
      }
      dmn_process_iter_end(&iter);
    }
    
    //- rjf: list -> array
    U64 infos_count = count;
    DMN_ProcessInfo *infos = push_array(arena, DMN_ProcessInfo, infos_count);
    CTRL_Entity **infos_machines = push_array(arena, CTRL_Entity *, infos_count);
    {
      U64 idx = 0;
      for(Node *n = first; n != 0; n = n->next, idx += 1)
      {
        infos[idx] = n->info;
        infos[idx].name = push_str8_copy(arena, infos[idx].name);
        infos_machines[idx] = n->machine;
      }
    }
    
    //- rjf: build accelerator
    RD_UnattachedProcessesAccel *accel = push_array(arena, RD_UnattachedProcessesAccel, 1);
    accel->infos       = infos;
    accel->infos_count = infos_count;
    accel->machines    = infos_machines;
    info.user_data = accel;
    info.expr_count = infos_count;
    scratch_end(scratch);
  }
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(unattached_processes)
{
  RD_UnattachedProcessesAccel *accel = (RD_UnattachedProcessesAccel *)user_data;
  U64 out_idx = 0;
  E_TypeKey unattached_process_type = e_type_key_cons(.kind = E_TypeKind_U128, .name = str8_lit("unattached_process"));
  for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, out_idx += 1)
  {
    CTRL_Entity *machine = accel->machines[idx];
    String8 string = ctrl_string_from_handle(arena, machine->handle);
    evals_out[out_idx] = e_eval_wrapf(eval, "query:control.%S.unattached_processes.$%I64x$%I64x", string, accel->infos[idx].pid, e_id_from_string(accel->infos[idx].name));
  }
}

////////////////////////////////
//~ rjf: Control Entity List Type Hooks (`processes`, `threads`, etc.)

E_TYPE_ACCESS_FUNCTION_DEF(ctrl_entities)
{
  E_IRTreeAndType result = {&e_irnode_nil};
  {
    CTRL_Entity *entity = &ctrl_entity_nil;
    switch(expr->kind)
    {
      case E_ExprKind_MemberAccess:
      {
        String8 rhs_name = expr->first->next->string;
        CTRL_Handle handle = ctrl_handle_from_string(rhs_name);
        entity = ctrl_entity_from_handle(&d_state->ctrl_entity_store->ctx, handle);
      }break;
      case E_ExprKind_ArrayIndex:
      {
        E_Type *type = e_type_from_key(lhs_irtree->type_key);
        CTRL_EntityKind kind = ctrl_entity_kind_from_string(rd_singular_from_code_name_plural(type->name));
        E_Value rhs_value = e_value_from_expr(expr->first->next);
        U64 rhs_idx = rhs_value.u64;
        CTRL_EntityArray entities = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, kind);
        if(0 <= rhs_idx && rhs_idx < entities.count)
        {
          entity = entities.v[rhs_idx];
        }
      }break;
    }
    if(entity != &ctrl_entity_nil)
    {
      E_Space space = rd_eval_space_from_ctrl_entity(entity, RD_EvalSpaceKind_MetaCtrlEntity);
      String8 name = ctrl_entity_kind_code_name_table[entity->kind];
      E_TypeKey type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, name);
      result.root      = e_irtree_set_space(arena, space, e_irtree_const_u(arena, 0));
      result.type_key  = type_key;
      result.mode      = E_Mode_Offset;
    }
  }
  return result;
}

E_TYPE_EXPAND_INFO_FUNCTION_DEF(ctrl_entities)
{
  E_TypeExpandInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    //- rjf: determine which entity we're looking under
    CTRL_Entity *scoping_entity = &ctrl_entity_nil;
    if(eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity)
    {
      scoping_entity = rd_ctrl_entity_from_eval_space(eval.space);
    }
    
    //- rjf: determine which type of child we're gathering
    E_TypeKey lhs_type_key = eval.irtree.type_key;
    E_Type *lhs_type = e_type_from_key(lhs_type_key);
    String8 name = rd_singular_from_code_name_plural(lhs_type->name);
    CTRL_EntityKind entity_kind = ctrl_entity_kind_from_string(name);
    
    //- rjf: gather array of all entities which fit the bill
    CTRL_EntityArray array = {0};
    if(scoping_entity == &ctrl_entity_nil)
    {
      array = ctrl_entity_array_from_kind(&d_state->ctrl_entity_store->ctx, entity_kind);
    }
    else
    {
      CTRL_EntityList list = {0};
      for(CTRL_Entity *child = scoping_entity->first; child != &ctrl_entity_nil; child = child->next)
      {
        if(child->kind == entity_kind)
        {
          ctrl_entity_list_push(scratch.arena, &list, child);
        }
      }
      array = ctrl_entity_array_from_list(arena, &list);
    }
    
    //- rjf: filter the array
    CTRL_EntityArray array__filtered = array;
    if(filter.size != 0)
    {
      CTRL_EntityList list__filtered = {0};
      for EachIndex(idx, array.count)
      {
        CTRL_Entity *entity = array.v[idx];
        DR_FStrList fstrs = rd_title_fstrs_from_ctrl_entity(scratch.arena, entity, 1);
        String8 title_string = dr_string_from_fstrs(scratch.arena, &fstrs);
        FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, title_string);
        if(matches.count == matches.needle_part_count)
        {
          ctrl_entity_list_push(scratch.arena, &list__filtered, entity);
        }
      }
      array__filtered = ctrl_entity_array_from_list(arena, &list__filtered);
    }
    
    //- rjf: list -> array & fill
    CTRL_EntityArray *accel = push_array(arena, CTRL_EntityArray, 1);
    *accel = array__filtered;
    result.user_data = accel;
    result.expr_count = accel->count;
  }
  scratch_end(scratch);
  return result;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(ctrl_entities)
{
  CTRL_EntityArray *entities = (CTRL_EntityArray *)user_data;
  Rng1U64 legal_range = r1u64(0, entities->count);
  Rng1U64 read_range = intersect_1u64(legal_range, idx_range);
  U64 read_count = dim_1u64(read_range);
  for(U64 out_idx = 0; out_idx < read_count; out_idx += 1)
  {
    Temp scratch = scratch_begin(&arena, 1);
    CTRL_Entity *entity = entities->v[out_idx + read_range.min];
    evals_out[out_idx] = e_eval_from_stringf("query:control.%S", ctrl_string_from_handle(scratch.arena, entity->handle));
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Debug Info Tables Eval Hooks

typedef struct RD_DebugInfoTableLookupAccel RD_DebugInfoTableLookupAccel;
struct RD_DebugInfoTableLookupAccel
{
  RDI_SectionKind section;
  U64 rdis_count;
  RDI_Parsed **rdis;
  DI_SearchItemArray items;
};

E_TYPE_EXPAND_INFO_FUNCTION_DEF(debug_info_table)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // rjf: determine which debug info section we're dealing with
  RDI_SectionKind section = RDI_SectionKind_NULL;
  {
    E_TypeKey lhs_type_key = eval.irtree.type_key;
    E_Type *lhs_type = e_type_from_key(lhs_type_key);
    if(0){}
    else if(str8_match(lhs_type->name, str8_lit("procedures"), 0))       {section = RDI_SectionKind_Procedures;}
    else if(str8_match(lhs_type->name, str8_lit("globals"), 0))          {section = RDI_SectionKind_GlobalVariables;}
    else if(str8_match(lhs_type->name, str8_lit("thread_locals"), 0))    {section = RDI_SectionKind_ThreadVariables;}
    else if(str8_match(lhs_type->name, str8_lit("constants"), 0))        {section = RDI_SectionKind_Constants;}
    else if(str8_match(lhs_type->name, str8_lit("types"), 0))            {section = RDI_SectionKind_UDTs;}
  }
  
  // rjf: gather debug info table items
  RD_DebugInfoTableLookupAccel *accel = push_array(arena, RD_DebugInfoTableLookupAccel, 1);
  if(section != RDI_SectionKind_NULL)
  {
    U64 endt_us = rd_state->frame_eval_memread_endt_us;
    
    //- rjf: unpack context
    DI_KeyList dbgi_keys_list = d_push_active_dbgi_key_list(scratch.arena);
    DI_KeyArray dbgi_keys = di_key_array_from_list(scratch.arena, &dbgi_keys_list);
    U64 rdis_count = dbgi_keys.count;
    RDI_Parsed **rdis = push_array(arena, RDI_Parsed *, rdis_count);
    for(U64 idx = 0; idx < rdis_count; idx += 1)
    {
      rdis[idx] = di_rdi_from_key(rd_state->frame_di_scope, &dbgi_keys.v[idx], endt_us);
    }
    
    //- rjf: query all filtered items from dbgi searching system
    U128 fuzzy_search_key = {d_hash_from_string(str8_struct(&rd_regs()->view)), (U64)section};
    B32 items_stale = 0;
    DI_SearchParams params = {section, dbgi_keys};
    accel->section = section;
    accel->rdis_count = rdis_count;
    accel->rdis = rdis;
    accel->items = di_search_items_from_key_params_query(rd_state->frame_di_scope, fuzzy_search_key, &params, filter, endt_us, &items_stale);
    if(items_stale)
    {
      rd_request_frame();
    }
  }
  E_TypeExpandInfo info = {accel, accel->items.count};
  scratch_end(scratch);
  return info;
}

E_TYPE_EXPAND_RANGE_FUNCTION_DEF(debug_info_table)
{
  Temp scratch = scratch_begin(&arena, 1);
  RD_DebugInfoTableLookupAccel *accel = (RD_DebugInfoTableLookupAccel *)user_data;
  U64 needed_row_count = dim_1u64(idx_range);
  for EachIndex(idx, needed_row_count)
  {
    // rjf: unpack row
    DI_SearchItem *item = &accel->items.v[idx_range.min + idx];
    
    // rjf: skip bad elements
    if(item->dbgi_idx >= accel->rdis_count)
    {
      continue;
    }
    
    // rjf: unpack row info
    RDI_Parsed *rdi = accel->rdis[item->dbgi_idx];
    E_Module *module = &e_base_ctx->modules[item->dbgi_idx];
    
    // rjf: get item's string
    String8 item_string = {0};
    {
      U64 element_idx = item->idx;
      switch(accel->section)
      {
        default:{}break;
        case RDI_SectionKind_Procedures:
        {
          RDI_Procedure *procedure = rdi_element_from_name_idx(module->rdi, Procedures, element_idx);
          RDI_Scope *scope = rdi_element_from_name_idx(module->rdi, Scopes, procedure->root_scope_idx);
          U64 voff = *rdi_element_from_name_idx(module->rdi, ScopeVOffData, scope->voff_range_first);
          E_OpList oplist = {0};
          e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU64, e_value_u64(module->vaddr_range.min + voff));
          String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
          U32 type_idx = procedure->type_idx;
          RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
          E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_base_ctx->modules));
          String8 symbol_name = {0};
          symbol_name.str = rdi_string_from_idx(module->rdi, procedure->name_string_idx, &symbol_name.size);
          item_string = symbol_name;
        }break;
        case RDI_SectionKind_GlobalVariables:
        {
          RDI_GlobalVariable *gvar = rdi_element_from_name_idx(module->rdi, GlobalVariables, element_idx);
          String8 symbol_name = {0};
          symbol_name.str = rdi_string_from_idx(module->rdi, gvar->name_string_idx, &symbol_name.size);
          item_string = symbol_name;
        }break;
        case RDI_SectionKind_ThreadVariables:
        {
          RDI_ThreadVariable *tvar = rdi_element_from_name_idx(module->rdi, ThreadVariables, element_idx);
          String8 symbol_name = {0};
          symbol_name.str = rdi_string_from_idx(module->rdi, tvar->name_string_idx, &symbol_name.size);
          item_string = symbol_name;
        }break;
        case RDI_SectionKind_Constants:
        {
          RDI_Constant *cnst = rdi_element_from_name_idx(module->rdi, Constants, element_idx);
          String8 symbol_name = {0};
          symbol_name.str = rdi_string_from_idx(module->rdi, cnst->name_string_idx, &symbol_name.size);
          item_string = symbol_name;
        }break;
        case RDI_SectionKind_UDTs:
        {
          RDI_UDT *udt = rdi_element_from_name_idx(module->rdi, UDTs, element_idx);
          RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, udt->self_type_idx);
          String8 name = {0};
          name.str = rdi_string_from_idx(module->rdi, type_node->user_defined.name_string_idx, &name.size);
          item_string = name;
        }break;
      }
    }
    
    // rjf: build a valid expression string given item string
    String8 item_expr = item_string;
    {
      B32 string_can_be_evalled = 1;
      E_TokenArray tokens = e_token_array_from_text(scratch.arena, item_string);
      for EachIndex(idx, tokens.count)
      {
        String8 token_string = str8_substr(item_string, tokens.v[idx].range);
        if(tokens.v[idx].kind != E_TokenKind_Identifier &&
           !str8_match(token_string, str8_lit("."), 0))
        {
          string_can_be_evalled = 0;
          break;
        }
      }
      if(!string_can_be_evalled)
      {
        item_expr = push_str8f(scratch.arena, "`%S`", item_string);
      }
    }
    
    // rjf: item's eval
    E_Eval item_eval = e_eval_from_string(item_expr);
    
    // rjf: fill
    evals_out[idx] = item_eval;
    temp_end(scratch);
  }
  scratch_end(scratch);
}

E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(debug_info_table)
{
  RD_DebugInfoTableLookupAccel *accel = (RD_DebugInfoTableLookupAccel *)user_data;
  U64 id = 0;
  if(0 < num && num <= accel->items.count)
  {
    id = accel->items.v[num-1].idx+1;
  }
  return id;
}

E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(debug_info_table)
{
  RD_DebugInfoTableLookupAccel *accel = (RD_DebugInfoTableLookupAccel *)user_data;
  U64 num = di_search_item_num_from_array_element_idx__linear_search(&accel->items, id-1);
  return num;
}
