// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Key Functions

internal EV_Key
ev_key_make(U64 parent_hash, U64 child_num)
{
  EV_Key key;
  {
    key.parent_hash = parent_hash;
    key.child_num = child_num;
  }
  return key;
}

internal EV_Key
ev_key_zero(void)
{
  EV_Key key = {0};
  return key;
}

internal B32
ev_key_match(EV_Key a, EV_Key b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

internal U64
ev_hash_from_seed_string(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
ev_hash_from_key(EV_Key key)
{
  U64 data[] =
  {
    key.child_num,
  };
  U64 hash = ev_hash_from_seed_string(key.parent_hash, str8((U8 *)data, sizeof(data)));
  return hash;
}

////////////////////////////////
//~ rjf: View Functions

//- rjf: creation / deletion

internal EV_View *
ev_view_alloc(void)
{
  Arena *arena = arena_alloc();
  EV_View *view = push_array(arena, EV_View, 1);
  view->arena = arena;
  view->expand_slots_count = 256;
  view->expand_slots = push_array(arena, EV_ExpandSlot, view->expand_slots_count);
  view->key_view_rule_slots_count = 256;
  view->key_view_rule_slots = push_array(arena, EV_KeyViewRuleSlot, view->key_view_rule_slots_count);
  return view;
}

internal void
ev_view_release(EV_View *view)
{
  arena_release(view->arena);
}

//- rjf: lookups / mutations

internal EV_ExpandNode *
ev_expand_node_from_key(EV_View *view, EV_Key key)
{
  U64 hash = ev_hash_from_key(key);
  U64 slot_idx = hash%view->expand_slots_count;
  EV_ExpandSlot *slot = &view->expand_slots[slot_idx];
  EV_ExpandNode *node = 0;
  for(EV_ExpandNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(ev_key_match(n->key, key))
    {
      node = n;
      break;
    }
  }
  return node;
}

internal B32
ev_expansion_from_key(EV_View *view, EV_Key key)
{
  EV_ExpandNode *node = ev_expand_node_from_key(view, key);
  return (node != 0 && node->expanded);
}

internal String8
ev_view_rule_from_key(EV_View *view, EV_Key key)
{
  String8 result = {0};
  
  //- rjf: key -> hash * slot idx * slot
  U64 hash = ev_hash_from_key(key);
  U64 slot_idx = hash%view->key_view_rule_slots_count;
  EV_KeyViewRuleSlot *slot = &view->key_view_rule_slots[slot_idx];
  
  //- rjf: slot -> existing node
  EV_KeyViewRuleNode *existing_node = 0;
  for(EV_KeyViewRuleNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(ev_key_match(n->key, key))
    {
      existing_node = n;
      break;
    }
  }
  
  //- rjf: node -> result
  if(existing_node != 0)
  {
    result = str8(existing_node->buffer, existing_node->buffer_string_size);
  }
  
  return result;
}

internal void
ev_key_set_expansion(Arena *arena, EV_View *view, EV_Key parent_key, EV_Key key, B32 expanded)
{
  // rjf: map keys => nodes
  EV_ExpandNode *parent_node = ev_expand_node_from_key(view, parent_key);
  EV_ExpandNode *node = ev_expand_node_from_key(view, key);
  
  // rjf: make node if we don't have one, and we need one
  if(node == 0 && expanded)
  {
    node = view->free_expand_node;
    if(node != 0)
    {
      SLLStackPop(view->free_expand_node);
      MemoryZeroStruct(node);
    }
    else
    {
      node = push_array(arena, EV_ExpandNode, 1);
    }
    
    // rjf: link into table
    U64 hash = ev_hash_from_key(key);
    U64 slot = hash % view->expand_slots_count;
    DLLPushBack_NP(view->expand_slots[slot].first, view->expand_slots[slot].last, node, hash_next, hash_prev);
    
    // rjf: link into parent
    if(parent_node != 0)
    {
      EV_ExpandNode *prev = 0;
      for(EV_ExpandNode *n = parent_node->first; n != 0; n = n->next)
      {
        if(n->key.child_num < key.child_num)
        {
          prev = n;
        }
        else
        {
          break;
        }
      }
      DLLInsert_NP(parent_node->first, parent_node->last, prev, node, next, prev);
      node->parent = parent_node;
    }
  }
  
  // rjf: fill
  if(node != 0)
  {
    node->key = key;
    node->expanded = expanded;
  }
  
  // rjf: unlink node & free if we don't need it anymore
  if(expanded == 0 && node != 0 && node->first == 0)
  {
    // rjf: unlink from table
    U64 hash = ev_hash_from_key(key);
    U64 slot = hash % view->expand_slots_count;
    DLLRemove_NP(view->expand_slots[slot].first, view->expand_slots[slot].last, node, hash_next, hash_prev);
    
    // rjf: unlink from tree
    if(parent_node != 0)
    {
      DLLRemove_NP(parent_node->first, parent_node->last, node, next, prev);
    }
    
    // rjf: free
    SLLStackPush(view->free_expand_node, node);
  }
}

internal void
ev_key_set_view_rule(EV_View *view, EV_Key key, String8 view_rule_string)
{
  //- rjf: key -> hash * slot idx * slot
  U64 hash = ev_hash_from_key(key);
  U64 slot_idx = hash%view->key_view_rule_slots_count;
  EV_KeyViewRuleSlot *slot = &view->key_view_rule_slots[slot_idx];
  
  //- rjf: slot -> existing node
  EV_KeyViewRuleNode *existing_node = 0;
  for(EV_KeyViewRuleNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(ev_key_match(n->key, key))
    {
      existing_node = n;
      break;
    }
  }
  
  //- rjf: existing node * new node -> node
  EV_KeyViewRuleNode *node = existing_node;
  if(node == 0)
  {
    node = push_array(view->arena, EV_KeyViewRuleNode, 1);
    DLLPushBack_NP(slot->first, slot->last, node, hash_next, hash_prev);
    node->key = key;
    node->buffer_cap = 512;
    node->buffer = push_array(view->arena, U8, node->buffer_cap);
  }
  
  //- rjf: mutate node
  if(node != 0)
  {
    node->buffer_string_size = ClampTop(view_rule_string.size, node->buffer_cap);
    MemoryCopy(node->buffer, view_rule_string.str, node->buffer_string_size);
  }
}

////////////////////////////////
//~ rjf: View Rule Info Table Building / Selection / Lookups

internal void
ev_view_rule_info_table_push(Arena *arena, EV_ViewRuleInfoTable *table, EV_ViewRuleInfo *info)
{
  if(table->slots_count == 0)
  {
    table->slots_count = 512;
    table->slots = push_array(arena, EV_ViewRuleInfoSlot, table->slots_count);
  }
  U64 hash = ev_hash_from_seed_string(5381, info->string);
  U64 slot_idx = hash%table->slots_count;
  EV_ViewRuleInfoSlot *slot = &table->slots[slot_idx];
  EV_ViewRuleInfoNode *n = push_array(arena, EV_ViewRuleInfoNode, 1);
  SLLQueuePush(slot->first, slot->last, n);
  MemoryCopyStruct(&n->v, info);
  n->v.string = push_str8_copy(arena, n->v.string);
}

internal void
ev_select_view_rule_info_table(EV_ViewRuleInfoTable *table)
{
  ev_view_rule_info_table = table;
}

internal EV_ViewRuleInfo *
ev_view_rule_info_from_string(String8 string)
{
  EV_ViewRuleInfo *info = &ev_nil_view_rule_info;
  U64 hash = ev_hash_from_seed_string(5381, info->string);
  U64 slot_idx = hash%ev_view_rule_info_table->slots_count;
  EV_ViewRuleInfoSlot *slot = &ev_view_rule_info_table->slots[slot_idx];
  EV_ViewRuleInfoNode *node = 0;
  for(EV_ViewRuleInfoNode *n = slot->first; n != 0; n = n->next)
  {
    if(str8_match(n->v.string, string, 0))
    {
      node = n;
      break;
    }
  }
  if(node != 0)
  {
    info = &node->v;
  }
  return info;
}

////////////////////////////////
//~ rjf: View Rule Instance List Building

internal void
ev_view_rule_list_push_tree(Arena *arena, EV_ViewRuleList *list, MD_Node *root)
{
  EV_ViewRuleNode *n = push_array(arena, EV_ViewRuleNode, 1);
  n->v.root = root;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal void
ev_view_rule_list_push_string(Arena *arena, EV_ViewRuleList *list, String8 string)
{
  MD_Node *root = md_tree_from_string(arena, string);
  for(MD_EachNode(tln, root->first))
  {
    ev_view_rule_list_push_tree(arena, list, tln);
  }
}

internal EV_ViewRuleList *
ev_view_rule_list_from_string(Arena *arena, String8 string)
{
  EV_ViewRuleList *dst = push_array(arena, EV_ViewRuleList, 1);
  ev_view_rule_list_push_string(arena, dst, string);
  return dst;
}

internal EV_ViewRuleList *
ev_view_rule_list_from_inheritance(Arena *arena, EV_ViewRuleList *src)
{
  EV_ViewRuleList *dst = push_array(arena, EV_ViewRuleList, 1);
  for(EV_ViewRuleNode *n = src->first; n != 0; n = n->next)
  {
    EV_ViewRuleInfo *info = ev_view_rule_info_from_string(n->v.root->string);
    if(info->flags & EV_ViewRuleInfoFlag_Inherited)
    {
      ev_view_rule_list_push_tree(arena, dst, n->v.root);
    }
  }
  return dst;
}

internal EV_ViewRuleList *
ev_view_rule_list_copy(Arena *arena, EV_ViewRuleList *src)
{
  EV_ViewRuleList *dst = push_array(arena, EV_ViewRuleList, 1);
  for(EV_ViewRuleNode *n = src->first; n != 0; n = n->next)
  {
    ev_view_rule_list_push_tree(arena, dst, n->v.root);
  }
  return dst;
}

////////////////////////////////
//~ rjf: View Rule Expression Resolution

internal E_Expr *
ev_expr_from_expr_view_rules(Arena *arena, E_Expr *expr, EV_ViewRuleList *view_rules)
{
  for(EV_ViewRuleNode *n = view_rules->first; n != 0; n = n->next)
  {
    EV_ViewRuleInfo *info = ev_view_rule_info_from_string(n->v.root->string);
    if(info->flags & EV_ViewRuleInfoFlag_ExprResolution)
    {
      expr = info->expr_resolution(arena, expr, n->v.root);
    }
  }
  return expr;
}

////////////////////////////////
//~ rjf: Block Building

internal EV_Block *
ev_block_begin(Arena *arena, EV_BlockKind kind, EV_Key parent_key, EV_Key key, S32 depth)
{
  EV_BlockNode *n = push_array(arena, EV_BlockNode, 1);
  n->v.kind       = kind;
  n->v.parent_key = parent_key;
  n->v.key        = key;
  n->v.depth      = depth;
  n->v.expr       = &e_expr_nil;
  return &n->v;
}

internal EV_Block *
ev_block_split_and_continue(Arena *arena, EV_BlockList *list, EV_Block *split_block, U64 split_idx)
{
  U64 total_count = split_block->semantic_idx_range.max;
  split_block->visual_idx_range.max = split_block->semantic_idx_range.max = split_idx;
  ev_block_end(list, split_block);
  EV_Block *continue_block = ev_block_begin(arena, split_block->kind, split_block->parent_key, split_block->key, split_block->depth);
  continue_block->string            = split_block->string;
  continue_block->expr              = split_block->expr;
  continue_block->visual_idx_range  = continue_block->semantic_idx_range = r1u64(split_idx+1, total_count);
  continue_block->view_rules        = split_block->view_rules;
  continue_block->members           = split_block->members;
  continue_block->enum_vals         = split_block->enum_vals;
  continue_block->fzy_target        = split_block->fzy_target;
  continue_block->fzy_backing_items = split_block->fzy_backing_items;
  return continue_block;
}

internal void
ev_block_end(EV_BlockList *list, EV_Block *block)
{
  EV_BlockNode *n = CastFromMember(EV_BlockNode, v, block);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  list->total_visual_row_count += dim_1u64(block->visual_idx_range);
  list->total_semantic_row_count += dim_1u64(block->semantic_idx_range);
}

internal void
ev_append_expr_blocks__rec(Arena *arena, EV_View *view, EV_Key parent_key, EV_Key key, String8 string, E_Expr *expr, EV_ViewRuleList *view_rules, S32 depth, EV_BlockList *list_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: apply expr resolution view rules
  expr = ev_expr_from_expr_view_rules(arena, expr, view_rules);
  
  //- rjf: determine if this key is expanded
  EV_ExpandNode *node = ev_expand_node_from_key(view, key);
  B32 parent_is_expanded = (node != 0 && node->expanded);
  
  //- rjf: push block for expression root
  {
    EV_Block *block = ev_block_begin(arena, EV_BlockKind_Root, parent_key, key, depth);
    block->string                      = string;
    block->expr                        = expr;
    block->view_rules                  = view_rules;
    block->visual_idx_range            = r1u64(key.child_num-1, key.child_num+0);
    block->semantic_idx_range          = r1u64(key.child_num-1, key.child_num+0);
    ev_block_end(list_out, block);
  }
  
  //- rjf: determine view rule to generate children blocks
  EV_ViewRuleInfo *block_prod_view_rule_info = ev_view_rule_info_from_string(str8_lit("default"));
  MD_Node *block_prod_view_rule_params = &md_nil_node;
  if(parent_is_expanded)
  {
    for(EV_ViewRuleNode *n = view_rules->first; n != 0; n = n->next)
    {
      EV_ViewRuleInfo *tln_info = ev_view_rule_info_from_string(n->v.root->string);
      if(tln_info->flags & EV_ViewRuleInfoFlag_VizBlockProd)
      {
        block_prod_view_rule_info = tln_info;
        block_prod_view_rule_params = n->v.root;
        break;
      }
    }
  }
  
  //- rjf: do view rule children block generation, if we have an applicable view rule
  if(parent_is_expanded && block_prod_view_rule_info != &ev_nil_view_rule_info)
  {
    block_prod_view_rule_info->block_prod(arena, view, parent_key, key, node, string, expr, view_rules, block_prod_view_rule_params, depth+1, list_out);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal EV_BlockList
ev_block_list_from_view_expr_keys(Arena *arena, EV_View *view, EV_ViewRuleList *view_rules, String8 expr, EV_Key parent_key, EV_Key key)
{
  ProfBeginFunction();
  EV_BlockList blocks = {0};
  {
    E_TokenArray tokens = e_token_array_from_text(arena, expr);
    E_Parse parse = e_parse_expr_from_text_tokens(arena, expr, &tokens);
    U64 parse_opl = parse.last_token >= tokens.v + tokens.count ? expr.size : parse.last_token->range.min;
    U64 expr_comma_pos = str8_find_needle(expr, parse_opl, str8_lit(","), 0);
    U64 passthrough_pos = str8_find_needle(expr, parse_opl, str8_lit("--"), 0);
    String8List default_view_rules = {0};
    if(expr_comma_pos < expr.size && expr_comma_pos < passthrough_pos)
    {
      String8 expr_extension = str8_substr(expr, r1u64(expr_comma_pos+1, passthrough_pos));
      expr_extension = str8_skip_chop_whitespace(expr_extension);
      if(str8_match(expr_extension, str8_lit("x"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(arena, &default_view_rules, "hex");
      }
      else if(str8_match(expr_extension, str8_lit("b"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(arena, &default_view_rules, "bin");
      }
      else if(str8_match(expr_extension, str8_lit("o"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(arena, &default_view_rules, "oct");
      }
      else if(expr_extension.size != 0)
      {
        str8_list_pushf(arena, &default_view_rules, "array:{%S}", expr_extension);
      }
    }
    if(passthrough_pos < expr.size)
    {
      String8 passthrough_view_rule = str8_skip_chop_whitespace(str8_skip(expr, passthrough_pos+2));
      if(passthrough_view_rule.size != 0)
      {
        str8_list_push(arena, &default_view_rules, passthrough_view_rule);
      }
    }
    String8 view_rule_string = ev_view_rule_from_key(view, key);
    EV_ViewRuleList *view_rule_list = ev_view_rule_list_copy(arena, view_rules);
    for(String8Node *n = default_view_rules.first; n != 0; n = n->next)
    {
      ev_view_rule_list_push_string(arena, view_rule_list, n->string);
    }
    ev_view_rule_list_push_string(arena, view_rule_list, view_rule_string);
    ev_append_expr_blocks__rec(arena, view, parent_key, key, expr, parse.expr, view_rule_list, 0, &blocks);
  }
  ProfEnd();
  return blocks;
}

internal void
ev_block_list_concat__in_place(EV_BlockList *dst, EV_BlockList *to_push)
{
  if(dst->last == 0)
  {
    *dst = *to_push;
  }
  else if(to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->count += to_push->count;
    dst->total_visual_row_count += to_push->total_visual_row_count;
    dst->total_semantic_row_count += to_push->total_semantic_row_count;
  }
  MemoryZeroStruct(to_push);
}

////////////////////////////////
//~ rjf: Block List <-> Row Coordinates

internal S64
ev_row_num_from_block_list_key(EV_BlockList *blocks, EV_Key key)
{
  S64 row_num = 1;
  B32 found = 0;
  for(EV_BlockNode *n = blocks->first; n != 0; n = n->next)
  {
    EV_Block *block = &n->v;
    if(key.parent_hash == block->key.parent_hash)
    {
      B32 this_block_contains_this_key = 0;
      {
        if(block->fzy_backing_items.v != 0)
        {
          U64 item_num = fzy_item_num_from_array_element_idx__linear_search(&block->fzy_backing_items, key.child_num);
          this_block_contains_this_key = (item_num != 0 && contains_1u64(block->semantic_idx_range, item_num-1));
        }
        else
        {
          this_block_contains_this_key = (block->semantic_idx_range.min+1 <= key.child_num && key.child_num < block->semantic_idx_range.max+1);
        }
      }
      if(this_block_contains_this_key)
      {
        found = 1;
        if(block->fzy_backing_items.v != 0)
        {
          U64 item_num = fzy_item_num_from_array_element_idx__linear_search(&block->fzy_backing_items, key.child_num);
          row_num += item_num-1-block->semantic_idx_range.min;
        }
        else
        {
          row_num += key.child_num-1-block->semantic_idx_range.min;
        }
        break;
      }
    }
    if(!found)
    {
      row_num += (S64)dim_1u64(block->semantic_idx_range);
    }
  }
  if(!found)
  {
    row_num = 0;
  }
  return row_num;
}

internal EV_Key
ev_key_from_block_list_row_num(EV_BlockList *blocks, S64 row_num)
{
  EV_Key key = {0};
  S64 scan_y = 1;
  for(EV_BlockNode *n = blocks->first; n != 0; n = n->next)
  {
    EV_Block *vb = &n->v;
    Rng1S64 vb_row_num_range = r1s64(scan_y, scan_y + (S64)dim_1u64(vb->semantic_idx_range));
    if(contains_1s64(vb_row_num_range, row_num))
    {
      key = vb->key;
      if(vb->fzy_backing_items.v != 0)
      {
        U64 item_idx = (U64)((row_num - vb_row_num_range.min) + vb->semantic_idx_range.min);
        if(item_idx < vb->fzy_backing_items.count)
        {
          key.child_num = vb->fzy_backing_items.v[item_idx].idx;
        }
      }
      else
      {
        key.child_num = vb->semantic_idx_range.min + (row_num - vb_row_num_range.min) + 1;
      }
      break;
    }
    scan_y += dim_1s64(vb_row_num_range);
  }
  return key;
}

internal EV_Key
ev_parent_key_from_block_list_row_num(EV_BlockList *blocks, S64 row_num)
{
  EV_Key key = {0};
  S64 scan_y = 1;
  for(EV_BlockNode *n = blocks->first; n != 0; n = n->next)
  {
    EV_Block *vb = &n->v;
    Rng1S64 vb_row_num_range = r1s64(scan_y, scan_y + (S64)dim_1u64(vb->semantic_idx_range));
    if(contains_1s64(vb_row_num_range, row_num))
    {
      key = vb->parent_key;
      break;
    }
    scan_y += dim_1s64(vb_row_num_range);
  }
  return key;
}

////////////////////////////////
//~ rjf: Block * Index -> Expressions

internal E_Expr *
ev_expr_from_block_index(Arena *arena, EV_Block *block, U64 index)
{
  E_Expr *result = block->expr;
  switch(block->kind)
  {
    default:{}break;
    case EV_BlockKind_Members:
    {
      E_MemberArray *members = &block->members;
      if(index < members->count)
      {
        E_Member *member = &members->v[index];
        E_Expr *dot_expr = e_expr_ref_member_access(arena, block->expr, member->name);
        result = dot_expr;
      }
    }break;
    case EV_BlockKind_EnumMembers:
    {
      E_EnumValArray *enum_vals = &block->enum_vals;
      if(index < enum_vals->count)
      {
        E_EnumVal *val = &enum_vals->v[index];
        E_Expr *dot_expr = e_expr_ref_member_access(arena, block->expr, val->name);
        result = dot_expr;
      }
    }break;
    case EV_BlockKind_Elements:
    {
      E_Expr *idx_expr = e_expr_ref_array_index(arena, block->expr, index);
      result = idx_expr;
    }break;
    case EV_BlockKind_DebugInfoTable:
    {
      // rjf: unpack row info
      FZY_Item *item = &block->fzy_backing_items.v[index];
      EV_Key parent_key = block->parent_key;
      EV_Key key = block->key;
      key.child_num = block->fzy_backing_items.v[index].idx;
      
      // rjf: determine module to which this item belongs
      E_Module *module = e_parse_ctx->primary_module;
      U64 base_idx = 0;
      {
        for(U64 module_idx = 0; module_idx < e_parse_ctx->modules_count; module_idx += 1)
        {
          U64 all_items_count = 0;
          rdi_section_raw_table_from_kind(e_parse_ctx->modules[module_idx].rdi, block->fzy_target, &all_items_count);
          if(base_idx <= item->idx && item->idx < base_idx + all_items_count)
          {
            module = &e_parse_ctx->modules[module_idx];
            break;
          }
          base_idx += all_items_count;
        }
      }
      
      // rjf: build expr
      E_Expr *item_expr = &e_expr_nil;
      {
        RDI_SectionKind section = block->fzy_target;
        U64 element_idx = block->fzy_backing_items.v[index].idx - base_idx;
        switch(section)
        {
          default:{}break;
          case RDI_SectionKind_Procedures:
          {
            RDI_Procedure *procedure = rdi_element_from_name_idx(module->rdi, Procedures, element_idx);
            RDI_Scope *scope = rdi_element_from_name_idx(module->rdi, Scopes, procedure->root_scope_idx);
            U64 voff = *rdi_element_from_name_idx(module->rdi, ScopeVOffData, scope->voff_range_first);
            E_OpList oplist = {0};
            e_oplist_push_op(arena, &oplist, RDI_EvalOp_ModuleOff, e_value_u64(voff));
            String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
            U32 type_idx = procedure->type_idx;
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_LeafBytecode, 0);
            item_expr->mode     = E_Mode_Value;
            item_expr->space    = module->space;
            item_expr->type_key = type_key;
            item_expr->bytecode = bytecode;
            item_expr->string.str = rdi_string_from_idx(module->rdi, procedure->name_string_idx, &item_expr->string.size);
          }break;
          case RDI_SectionKind_GlobalVariables:
          {
            RDI_GlobalVariable *gvar = rdi_element_from_name_idx(module->rdi, GlobalVariables, element_idx);
            U64 voff = gvar->voff;
            E_OpList oplist = {0};
            e_oplist_push_op(arena, &oplist, RDI_EvalOp_ModuleOff, e_value_u64(voff));
            String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
            U32 type_idx = gvar->type_idx;
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_LeafBytecode, 0);
            item_expr->mode     = E_Mode_Offset;
            item_expr->space    = module->space;
            item_expr->type_key = type_key;
            item_expr->bytecode = bytecode;
            item_expr->string.str = rdi_string_from_idx(module->rdi, gvar->name_string_idx, &item_expr->string.size);
          }break;
          case RDI_SectionKind_ThreadVariables:
          {
            RDI_ThreadVariable *tvar = rdi_element_from_name_idx(module->rdi, ThreadVariables, element_idx);
            E_OpList oplist = {0};
            e_oplist_push_op(arena, &oplist, RDI_EvalOp_TLSOff, e_value_u64(tvar->tls_off));
            String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
            U32 type_idx = tvar->type_idx;
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_LeafBytecode, 0);
            item_expr->mode     = E_Mode_Offset;
            item_expr->space    = module->space;
            item_expr->type_key = type_key;
            item_expr->bytecode = bytecode;
            item_expr->string.str = rdi_string_from_idx(module->rdi, tvar->name_string_idx, &item_expr->string.size);
          }break;
          case RDI_SectionKind_UDTs:
          {
            RDI_UDT *udt = rdi_element_from_name_idx(module->rdi, UDTs, element_idx);
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, udt->self_type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), udt->self_type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_TypeIdent, 0);
            item_expr->type_key = type_key;
          }break;
        }
      }
      
      result = item_expr;
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Row Lists

internal EV_Row *
ev_row_list_push_new(Arena *arena, EV_View *view, EV_WindowedRowList *rows, EV_Block *block, EV_Key key, E_Expr *expr)
{
  // rjf: push
  EV_Row *row = push_array(arena, EV_Row, 1);
  SLLQueuePush(rows->first, rows->last, row);
  rows->count += 1;
  
  // rjf: pick view rule list; resolve expression if needed
  EV_ViewRuleList *view_rules = 0;
  switch(block->kind)
  {
    default:
    {
      view_rules = ev_view_rule_list_from_inheritance(arena, block->view_rules);
      String8 row_view_rules = ev_view_rule_from_key(view, key);
      if(row_view_rules.size != 0)
      {
        ev_view_rule_list_push_string(arena, view_rules, row_view_rules);
      }
      expr = ev_expr_from_expr_view_rules(arena, expr, view_rules);
    }break;
    case EV_BlockKind_Root:
    case EV_BlockKind_Canvas:
    {
      view_rules = block->view_rules;
    }break;
  }
  
  // rjf: fill
  row->depth        = block->depth;
  row->parent_key   = block->parent_key;
  row->key          = key;
  row->size_in_rows = 1;
  row->string       = block->string;
  row->expr         = expr;
  if(row->expr->kind == E_ExprKind_MemberAccess)
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr->first);
    E_TypeKey type = irtree.type_key;
    E_MemberArray data_members = e_type_data_members_from_key(scratch.arena, type);
    E_Member *member = e_type_member_from_array_name(&data_members, row->expr->last->string);
    if(member != 0)
    {
      row->member = e_type_member_copy(arena, member);
    }
    scratch_end(scratch);
  }
  row->view_rules = view_rules;
  return row;
}

internal EV_WindowedRowList
ev_windowed_row_list_from_block_list(Arena *arena, EV_View *view, Rng1S64 visible_range, EV_BlockList *blocks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //////////////////////////////
  //- rjf: produce windowed rows, per block
  //
  U64 visual_idx_off = 0;
  U64 semantic_idx_off = 0;
  EV_WindowedRowList list = {0};
  for(EV_BlockNode *n = blocks->first; n != 0; n = n->next)
  {
    EV_Block *block = &n->v;
    
    //////////////////////////////
    //- rjf: extract block info
    //
    U64 block_num_visual_rows     = dim_1u64(block->visual_idx_range);
    U64 block_num_semantic_rows   = dim_1u64(block->semantic_idx_range);
    Rng1S64 block_visual_range    = r1s64(visual_idx_off, visual_idx_off + block_num_visual_rows);
    Rng1S64 block_semantic_range  = r1s64(semantic_idx_off, semantic_idx_off + block_num_semantic_rows);
    
    //////////////////////////////
    //- rjf: get skip/chop of block's index range
    //
    U64 num_skipped_visual = 0;
    U64 num_chopped_visual = 0;
    {
      if(visible_range.min > block_visual_range.min)
      {
        num_skipped_visual = (visible_range.min - block_visual_range.min);
        num_skipped_visual = Min(num_skipped_visual, block_num_visual_rows);
      }
      if(visible_range.max < block_visual_range.max)
      {
        num_chopped_visual = (block_visual_range.max - visible_range.max);
        num_chopped_visual = Min(num_chopped_visual, block_num_visual_rows);
      }
    }
    
    //////////////////////////////
    //- rjf: get visible idx range & invisible counts
    //
    Rng1U64 visible_idx_range = block->visual_idx_range;
    {
      visible_idx_range.min += num_skipped_visual;
      visible_idx_range.max -= num_chopped_visual;
    }
    
    //////////////////////////////
    //- rjf: sum & advance
    //
    list.count_before_visual += num_skipped_visual;
    if(block_num_visual_rows != 0)
    {
      list.count_before_semantic += block_num_semantic_rows * num_skipped_visual / block_num_visual_rows;
    }
    visual_idx_off += block_num_visual_rows;
    semantic_idx_off += block_num_semantic_rows;
    
    //////////////////////////////
    //- rjf: produce rows, depending on block's kind
    //
    if(visible_idx_range.max > visible_idx_range.min) switch(block->kind)
    {
      default:{}break;
      
      //////////////////////////////
      //- rjf: single rows, piping in info from the originating block
      //
      case EV_BlockKind_Null:
      case EV_BlockKind_Root:
      {
        ev_row_list_push_new(arena, view, &list, block, block->key, block->expr);
      }break;
      
      //////////////////////////////
      //- rjf: canvas -> produce blank row, sized by the idx range specified in the block
      //
      case EV_BlockKind_Canvas:
      if(num_skipped_visual < block_num_visual_rows)
      {
        EV_Key key = ev_key_make(ev_hash_from_key(block->parent_key), 1);
        EV_Row *row = ev_row_list_push_new(arena, view, &list, block, key, block->expr);
        row->size_in_rows        = dim_1u64(intersect_1u64(visible_idx_range, r1u64(0, dim_1u64(block->visual_idx_range))));
        row->skipped_size_in_rows= (visible_idx_range.min > block->visual_idx_range.min) ? visible_idx_range.min - block->visual_idx_range.min : 0;
        row->chopped_size_in_rows= (visible_idx_range.max < block->visual_idx_range.max) ? block->visual_idx_range.max - visible_idx_range.max : 0;
      }break;
      
      //////////////////////////////
      //- rjf: all elements of a debug info table -> produce rows for visible range
      //
      case EV_BlockKind_DebugInfoTable:
      for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max; idx += 1)
      {
        FZY_Item *item = &block->fzy_backing_items.v[idx];
        EV_Key parent_key = block->parent_key;
        EV_Key key = block->key;
        key.child_num = block->fzy_backing_items.v[idx].idx;
        E_Expr *row_expr = ev_expr_from_block_index(arena, block, idx);
        ev_row_list_push_new(arena, view, &list, block, key, row_expr);
      }break;
      
      //////////////////////////////
      //- rjf: members/elements/enum-members
      //
      case EV_BlockKind_Members:
      case EV_BlockKind_EnumMembers:
      case EV_BlockKind_Elements:
      {
        for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max; idx += 1)
        {
          EV_Key key = ev_key_make(ev_hash_from_key(block->parent_key), idx+1);
          E_Expr *expr = ev_expr_from_block_index(arena, block, idx);
          ev_row_list_push_new(arena, view, &list, block, key, expr);
        }
      }break;
    }
  }
  scratch_end(scratch);
  ProfEnd();
  return list;
}

internal String8
ev_expr_string_from_row(Arena *arena, EV_Row *row)
{
  String8 result = row->string;
  if(result.size == 0) switch(row->expr->kind)
  {
    default:
    {
      result = e_string_from_expr(arena, row->expr);
    }break;
    case E_ExprKind_ArrayIndex:
    {
      result = push_str8f(arena, "[%S]", e_string_from_expr(arena, row->expr->last));
    }break;
    case E_ExprKind_MemberAccess:
    {
      result = push_str8f(arena, ".%S", e_string_from_expr(arena, row->expr->last));
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Stringification

//- rjf: leaf stringification

internal String8
ev_string_from_ascii_value(Arena *arena, U8 val)
{
  String8 result = {0};
  switch(val)
  {
    case 0x00:{result = str8_lit("\\0");}break;
    case 0x07:{result = str8_lit("\\a");}break;
    case 0x08:{result = str8_lit("\\b");}break;
    case 0x0c:{result = str8_lit("\\f");}break;
    case 0x0a:{result = str8_lit("\\n");}break;
    case 0x0d:{result = str8_lit("\\r");}break;
    case 0x09:{result = str8_lit("\\t");}break;
    case 0x0b:{result = str8_lit("\\v");}break;
    case 0x3f:{result = str8_lit("\\?");}break;
    case '"': {result = str8_lit("\\\"");}break;
    case '\'':{result = str8_lit("\\'");}break;
    case '\\':{result = str8_lit("\\\\");}break;
    default:
    if(32 <= val && val < 255)
    {
      result = push_str8f(arena, "%c", val);
    }break;
  }
  return result;
}

internal String8
ev_string_from_hresult_facility_code(U32 code)
{
  String8 result = {0};
  switch(code)
  {
    default:{}break;
    case 0x1:{result = str8_lit("RPC");}break;
    case 0x2:{result = str8_lit("DISPATCH");}break;
    case 0x3:{result = str8_lit("STORAGE");}break;
    case 0x4:{result = str8_lit("ITF");}break;
    case 0x7:{result = str8_lit("WIN32");}break;
    case 0x8:{result = str8_lit("WINDOWS");}break;
    case 0x9:{result = str8_lit("SECURITY|SSPI");}break;
    case 0xA:{result = str8_lit("CONTROL");}break;
    case 0xB:{result = str8_lit("CERT");}break;
    case 0xC:{result = str8_lit("INTERNET");}break;
    case 0xD:{result = str8_lit("MEDIASERVER");}break;
    case 0xE:{result = str8_lit("MSMQ");}break;
    case 0xF:{result = str8_lit("SETUPAPI");}break;
    case 0x10:{result = str8_lit("SCARD");}break;
    case 0x11:{result = str8_lit("COMPLUS");}break;
    case 0x12:{result = str8_lit("AAF");}break;
    case 0x13:{result = str8_lit("URT");}break;
    case 0x14:{result = str8_lit("ACS");}break;
    case 0x15:{result = str8_lit("DPLAY");}break;
    case 0x16:{result = str8_lit("UMI");}break;
    case 0x17:{result = str8_lit("SXS");}break;
    case 0x18:{result = str8_lit("WINDOWS_CE");}break;
    case 0x19:{result = str8_lit("HTTP");}break;
    case 0x1A:{result = str8_lit("USERMODE_COMMONLOG");}break;
    case 0x1B:{result = str8_lit("WER");}break;
    case 0x1F:{result = str8_lit("USERMODE_FILTER_MANAGER");}break;
    case 0x20:{result = str8_lit("BACKGROUNDCOPY");}break;
    case 0x21:{result = str8_lit("CONFIGURATION|WIA");}break;
    case 0x22:{result = str8_lit("STATE_MANAGEMENT");}break;
    case 0x23:{result = str8_lit("METADIRECTORY");}break;
    case 0x24:{result = str8_lit("WINDOWSUPDATE");}break;
    case 0x25:{result = str8_lit("DIRECTORYSERVICE");}break;
    case 0x26:{result = str8_lit("GRAPHICS");}break;
    case 0x27:{result = str8_lit("SHELL|NAP");}break;
    case 0x28:{result = str8_lit("TPM_SERVICES");}break;
    case 0x29:{result = str8_lit("TPM_SOFTWARE");}break;
    case 0x2A:{result = str8_lit("UI");}break;
    case 0x2B:{result = str8_lit("XAML");}break;
    case 0x2C:{result = str8_lit("ACTION_QUEUE");}break;
    case 0x30:{result = str8_lit("WINDOWS_SETUP|PLA");}break;
    case 0x31:{result = str8_lit("FVE");}break;
    case 0x32:{result = str8_lit("FWP");}break;
    case 0x33:{result = str8_lit("WINRM");}break;
    case 0x34:{result = str8_lit("NDIS");}break;
    case 0x35:{result = str8_lit("USERMODE_HYPERVISOR");}break;
    case 0x36:{result = str8_lit("CMI");}break;
    case 0x37:{result = str8_lit("USERMODE_VIRTUALIZATION");}break;
    case 0x38:{result = str8_lit("USERMODE_VOLMGR");}break;
    case 0x39:{result = str8_lit("BCD");}break;
    case 0x3A:{result = str8_lit("USERMODE_VHD");}break;
    case 0x3C:{result = str8_lit("SDIAG");}break;
    case 0x3D:{result = str8_lit("WINPE|WEBSERVICES");}break;
    case 0x3E:{result = str8_lit("WPN");}break;
    case 0x3F:{result = str8_lit("WINDOWS_STORE");}break;
    case 0x40:{result = str8_lit("INPUT");}break;
    case 0x42:{result = str8_lit("EAP");}break;
    case 0x50:{result = str8_lit("WINDOWS_DEFENDER");}break;
    case 0x51:{result = str8_lit("OPC");}break;
    case 0x52:{result = str8_lit("XPS");}break;
    case 0x53:{result = str8_lit("RAS");}break;
    case 0x54:{result = str8_lit("POWERSHELL|MBN");}break;
    case 0x55:{result = str8_lit("EAS");}break;
    case 0x62:{result = str8_lit("P2P_INT");}break;
    case 0x63:{result = str8_lit("P2P");}break;
    case 0x64:{result = str8_lit("DAF");}break;
    case 0x65:{result = str8_lit("BLUETOOTH_ATT");}break;
    case 0x66:{result = str8_lit("AUDIO");}break;
    case 0x6D:{result = str8_lit("VISUALCPP");}break;
    case 0x70:{result = str8_lit("SCRIPT");}break;
    case 0x71:{result = str8_lit("PARSE");}break;
    case 0x78:{result = str8_lit("BLB");}break;
    case 0x79:{result = str8_lit("BLB_CLI");}break;
    case 0x7A:{result = str8_lit("WSBAPP");}break;
    case 0x80:{result = str8_lit("BLBUI");}break;
    case 0x81:{result = str8_lit("USN");}break;
    case 0x82:{result = str8_lit("USERMODE_VOLSNAP");}break;
    case 0x83:{result = str8_lit("TIERING");}break;
    case 0x85:{result = str8_lit("WSB_ONLINE");}break;
    case 0x86:{result = str8_lit("ONLINE_ID");}break;
    case 0x99:{result = str8_lit("DLS");}break;
    case 0xA0:{result = str8_lit("SOS");}break;
    case 0xB0:{result = str8_lit("DEBUGGERS");}break;
    case 0xE7:{result = str8_lit("USERMODE_SPACES");}break;
    case 0x100:{result = str8_lit("DMSERVER|RESTORE|SPP");}break;
    case 0x101:{result = str8_lit("DEPLOYMENT_SERVICES_SERVER");}break;
    case 0x102:{result = str8_lit("DEPLOYMENT_SERVICES_IMAGING");}break;
    case 0x103:{result = str8_lit("DEPLOYMENT_SERVICES_MANAGEMENT");}break;
    case 0x104:{result = str8_lit("DEPLOYMENT_SERVICES_UTIL");}break;
    case 0x105:{result = str8_lit("DEPLOYMENT_SERVICES_BINLSVC");}break;
    case 0x107:{result = str8_lit("DEPLOYMENT_SERVICES_PXE");}break;
    case 0x108:{result = str8_lit("DEPLOYMENT_SERVICES_TFTP");}break;
    case 0x110:{result = str8_lit("DEPLOYMENT_SERVICES_TRANSPORT_MANAGEMENT");}break;
    case 0x116:{result = str8_lit("DEPLOYMENT_SERVICES_DRIVER_PROVISIONING");}break;
    case 0x121:{result = str8_lit("DEPLOYMENT_SERVICES_MULTICAST_SERVER");}break;
    case 0x122:{result = str8_lit("DEPLOYMENT_SERVICES_MULTICAST_CLIENT");}break;
    case 0x125:{result = str8_lit("DEPLOYMENT_SERVICES_CONTENT_PROVIDER");}break;
    case 0x131:{result = str8_lit("LINGUISTIC_SERVICES");}break;
    case 0x375:{result = str8_lit("WEB");}break;
    case 0x376:{result = str8_lit("WEB_SOCKET");}break;
    case 0x446:{result = str8_lit("AUDIOSTREAMING");}break;
    case 0x600:{result = str8_lit("ACCELERATOR");}break;
    case 0x701:{result = str8_lit("MOBILE");}break;
    case 0x7CC:{result = str8_lit("WMAAECMA");}break;
    case 0x801:{result = str8_lit("WEP");}break;
    case 0x802:{result = str8_lit("SYNCENGINE");}break;
    case 0x878:{result = str8_lit("DIRECTMUSIC");}break;
    case 0x879:{result = str8_lit("DIRECT3D10");}break;
    case 0x87A:{result = str8_lit("DXGI");}break;
    case 0x87B:{result = str8_lit("DXGI_DDI");}break;
    case 0x87C:{result = str8_lit("DIRECT3D11");}break;
    case 0x888:{result = str8_lit("LEAP");}break;
    case 0x889:{result = str8_lit("AUDCLNT");}break;
    case 0x898:{result = str8_lit("WINCODEC_DWRITE_DWM");}break;
    case 0x899:{result = str8_lit("DIRECT2D");}break;
    case 0x900:{result = str8_lit("DEFRAG");}break;
    case 0x901:{result = str8_lit("USERMODE_SDBUS");}break;
    case 0x902:{result = str8_lit("JSCRIPT");}break;
    case 0xA01:{result = str8_lit("PIDGENX");}break;
  }
  return result;
}

internal String8
ev_string_from_hresult_code(U32 code)
{
  String8 result = {0};
  switch(code)
  {
    default:{}break;
    case 0x00000000: {result = str8_lit("S_OK: Operation successful");}break;
    case 0x00000001: {result = str8_lit("S_FALSE: Operation successful but returned no results");}break;
    case 0x80004004: {result = str8_lit("E_ABORT: Operation aborted");}break;
    case 0x80004005: {result = str8_lit("E_FAIL: Unspecified failure");}break;
    case 0x80004002: {result = str8_lit("E_NOINTERFACE: No such interface supported");}break;
    case 0x80004001: {result = str8_lit("E_NOTIMPL: Not implemented");}break;
    case 0x80004003: {result = str8_lit("E_POINTER: Pointer that is not valid");}break;
    case 0x8000FFFF: {result = str8_lit("E_UNEXPECTED: Unexpected failure");}break;
    case 0x80070005: {result = str8_lit("E_ACCESSDENIED: General access denied error");}break;
    case 0x80070006: {result = str8_lit("E_HANDLE: Handle that is not valid");}break;
    case 0x80070057: {result = str8_lit("E_INVALIDARG: One or more arguments are not valid");}break;
    case 0x8007000E: {result = str8_lit("E_OUTOFMEMORY: Failed to allocate necessary memory");}break;
  }
  return result;
}

internal String8
ev_string_from_simple_typed_eval(Arena *arena, EV_StringFlags flags, U32 radix, E_Eval eval)
{
  String8 result = {0};
  E_TypeKey type_key = e_type_unwrap(eval.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  U64 type_byte_size = e_type_byte_size_from_key(type_key);
  U8 digit_group_separator = 0;
  if(!(flags & EV_StringFlag_ReadOnlyDisplayRules))
  {
    digit_group_separator = 0;
  }
  switch(type_kind)
  {
    default:{}break;
    
    case E_TypeKind_Handle:
    {
      result = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
    }break;
    
    case E_TypeKind_HResult:
    {
      if(flags & EV_StringFlag_ReadOnlyDisplayRules)
      {
        Temp scratch = scratch_begin(&arena, 1);
        U32 hresult_value = (U32)eval.value.u64;
        U32 is_error   = !!(hresult_value & (1ull<<31));
        U32 error_code = (hresult_value);
        U32 facility   = (hresult_value & 0x7ff0000) >> 16;
        String8 value_string = str8_from_s64(scratch.arena, eval.value.u64, radix, 0, digit_group_separator);
        String8 facility_string = ev_string_from_hresult_facility_code(facility);
        String8 error_string = ev_string_from_hresult_code(error_code);
        result = push_str8f(arena, "%S%s%s%S%s%s%S%s",
                            error_string,
                            error_string.size != 0 ? " " : "",
                            facility_string.size != 0 ? "[" : "",
                            facility_string,
                            facility_string.size != 0 ? "] ": "",
                            error_string.size != 0 ? "(" : "",
                            value_string,
                            error_string.size != 0 ? ")" : "");
        scratch_end(scratch);
      }
      else
      {
        result = str8_from_s64(arena, eval.value.u64, radix, 0, digit_group_separator);
      }
    }break;
    
    case E_TypeKind_Char8:
    case E_TypeKind_Char16:
    case E_TypeKind_Char32:
    case E_TypeKind_UChar8:
    case E_TypeKind_UChar16:
    case E_TypeKind_UChar32:
    {
      String8 char_str = ev_string_from_ascii_value(arena, eval.value.s64);
      if(char_str.size != 0)
      {
        if(flags & EV_StringFlag_ReadOnlyDisplayRules)
        {
          String8 imm_string = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
          result = push_str8f(arena, "'%S' (%S)", char_str, imm_string);
        }
        else
        {
          result = push_str8f(arena, "'%S'", char_str);
        }
      }
      else
      {
        result = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
      }
    }break;
    
    case E_TypeKind_S8:
    case E_TypeKind_S16:
    case E_TypeKind_S32:
    case E_TypeKind_S64:
    {
      result = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
    }break;
    
    case E_TypeKind_U8:
    case E_TypeKind_U16:
    case E_TypeKind_U32:
    case E_TypeKind_U64:
    {
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      result = str8_from_u64(arena, eval.value.u64, radix, min_digits, digit_group_separator);
    }break;
    
    case E_TypeKind_U128:
    {
      Temp scratch = scratch_begin(&arena, 1);
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      String8 upper64 = str8_from_u64(scratch.arena, eval.value.u128.u64[0], radix, min_digits, digit_group_separator);
      String8 lower64 = str8_from_u64(scratch.arena, eval.value.u128.u64[1], radix, min_digits, digit_group_separator);
      result = push_str8f(arena, "%S:%S", upper64, lower64);
      scratch_end(scratch);
    }break;
    
    case E_TypeKind_F32: {result = push_str8f(arena, "%f", eval.value.f32);}break;
    case E_TypeKind_F64: {result = push_str8f(arena, "%f", eval.value.f64);}break;
    case E_TypeKind_Bool:{result = push_str8f(arena, "%s", eval.value.u64 ? "true" : "false");}break;
    case E_TypeKind_Ptr: {result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_LRef:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_RRef:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_Function:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    
    case E_TypeKind_Enum:
    {
      Temp scratch = scratch_begin(&arena, 1);
      E_Type *type = e_type_from_key(scratch.arena, type_key);
      String8 constant_name = {0};
      for(U64 val_idx = 0; val_idx < type->count; val_idx += 1)
      {
        if(eval.value.u64 == type->enum_vals[val_idx].val)
        {
          constant_name = type->enum_vals[val_idx].name;
          break;
        }
      }
      if(flags & EV_StringFlag_ReadOnlyDisplayRules)
      {
        if(constant_name.size != 0)
        {
          result = push_str8f(arena, "0x%I64x (%S)", eval.value.u64, constant_name);
        }
        else
        {
          result = push_str8f(arena, "0x%I64x (%I64u)", eval.value.u64, eval.value.u64);
        }
      }
      else if(constant_name.size != 0)
      {
        result = push_str8_copy(arena, constant_name);
      }
      else
      {
        result = push_str8f(arena, "0x%I64x (%I64u)", eval.value.u64, eval.value.u64);
      }
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal String8
ev_escaped_from_raw_string(Arena *arena, String8 raw)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List parts = {0};
  U64 start_split_idx = 0;
  for(U64 idx = 0; idx <= raw.size; idx += 1)
  {
    U8 byte = (idx < raw.size) ? raw.str[idx] : 0;
    B32 split = 1;
    String8 separator_replace = {0};
    switch(byte)
    {
      default:{split = 0;}break;
      case 0:    {}break;
      case '\a': {separator_replace = str8_lit("\\a");}break;
      case '\b': {separator_replace = str8_lit("\\b");}break;
      case '\f': {separator_replace = str8_lit("\\f");}break;
      case '\n': {separator_replace = str8_lit("\\n");}break;
      case '\r': {separator_replace = str8_lit("\\r");}break;
      case '\t': {separator_replace = str8_lit("\\t");}break;
      case '\v': {separator_replace = str8_lit("\\v");}break;
      case '\\': {separator_replace = str8_lit("\\\\");}break;
      case '"':  {separator_replace = str8_lit("\\\"");}break;
      case '?':  {separator_replace = str8_lit("\\?");}break;
    }
    if(split)
    {
      String8 substr = str8_substr(raw, r1u64(start_split_idx, idx));
      start_split_idx = idx+1;
      str8_list_push(scratch.arena, &parts, substr);
      if(separator_replace.size != 0)
      {
        str8_list_push(scratch.arena, &parts, separator_replace);
      }
    }
  }
  StringJoin join = {0};
  String8 result = str8_list_join(arena, &parts, &join);
  scratch_end(scratch);
  return result;
}
