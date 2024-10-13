// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/eval_visualization.meta.c"

////////////////////////////////
//~ rjf: Nil/Identity View Rule Hooks

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(identity)
{
  return expr;
}

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(nil)
{
  EV_ExpandInfo info = {0};
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(nil)
{
  EV_ExpandRangeInfo info = {0};
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(identity)
{
  return num;
}

EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(identity)
{
  return id;
}

////////////////////////////////
//~ rjf: Key Functions

internal EV_Key
ev_key_make(U64 parent_hash, U64 child_id)
{
  EV_Key key;
  {
    key.parent_hash = parent_hash;
    key.child_id = child_id;
  }
  return key;
}

internal EV_Key
ev_key_zero(void)
{
  EV_Key key = {0};
  return key;
}

internal EV_Key
ev_key_root(void)
{
  EV_Key key = ev_key_make(5381, 1);
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
    key.child_id,
  };
  U64 hash = ev_hash_from_seed_string(key.parent_hash, str8((U8 *)data, sizeof(data)));
  return hash;
}

////////////////////////////////
//~ rjf: Type Info Helpers

//- rjf: type info -> expandability/editablity

internal B32
ev_type_key_and_mode_is_expandable(E_TypeKey type_key, E_Mode mode)
{
  B32 result = 0;
  for(E_TypeKey t = type_key; !result; t = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(t))))
  {
    E_TypeKind kind = e_type_kind_from_key(t);
    if(kind == E_TypeKind_Null || kind == E_TypeKind_Function)
    {
      break;
    }
    if(kind == E_TypeKind_Struct ||
       kind == E_TypeKind_Union ||
       kind == E_TypeKind_Class ||
       kind == E_TypeKind_Array ||
       (kind == E_TypeKind_Enum && mode == E_Mode_Null))
    {
      result = 1;
    }
  }
  return result;
}

internal B32
ev_type_key_is_editable(E_TypeKey type_key)
{
  B32 result = 0;
  for(E_TypeKey t = type_key; !result; t = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(t))))
  {
    E_TypeKind kind = e_type_kind_from_key(t);
    if(kind == E_TypeKind_Null || kind == E_TypeKind_Function)
    {
      break;
    }
    if((E_TypeKind_FirstBasic <= kind && kind <= E_TypeKind_LastBasic) || e_type_kind_is_pointer_or_ref(kind))
    {
      result = 1;
    }
  }
  return result;
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
ev_key_set_expansion(EV_View *view, EV_Key parent_key, EV_Key key, B32 expanded)
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
      node = push_array(view->arena, EV_ExpandNode, 1);
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
        if(n->key.child_id < key.child_id)
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
ev_view_rule_info_table_push_builtins(Arena *arena, EV_ViewRuleInfoTable *table)
{
  for EachEnumVal(EV_ViewRuleKind, kind)
  {
    ev_view_rule_info_table_push(arena, table, &ev_builtin_view_rule_info_table[kind]);
  }
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
  U64 hash = ev_hash_from_seed_string(5381, string);
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
//~ rjf: Automatic Type -> View Rule Table Building / Selection / Lookups

internal void
ev_auto_view_rule_table_push_new(Arena *arena, EV_AutoViewRuleTable *table, E_TypeKey type_key, String8 view_rule, B32 is_required)
{
  if(table->slots_count == 0)
  {
    table->slots_count = 4096;
    table->slots = push_array(arena, EV_AutoViewRuleSlot, table->slots_count);
  }
  U64 hash = e_hash_from_type_key(type_key);
  U64 slot_idx = hash%table->slots_count;
  EV_AutoViewRuleSlot *slot = &table->slots[slot_idx];
  EV_AutoViewRuleNode *node = 0;
  for(EV_AutoViewRuleNode *n = slot->first; n != 0; n = n->next)
  {
    if(e_type_match(n->key, type_key))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = push_array(arena, EV_AutoViewRuleNode, 1);
    node->key = type_key;
    node->view_rule = push_str8_copy(arena, view_rule);
    node->is_required = is_required;
    SLLQueuePush(slot->first, slot->last, node);
  }
}

internal void
ev_select_auto_view_rule_table(EV_AutoViewRuleTable *table)
{
  ev_auto_view_rule_table = table;
}

internal EV_ViewRuleList *
ev_auto_view_rules_from_type_key(Arena *arena, E_TypeKey type_key, B32 gather_required, B32 gather_optional)
{
  EV_ViewRuleList *result = &ev_nil_view_rule_list;
  if(ev_auto_view_rule_table != 0 && ev_auto_view_rule_table->slots_count != 0)
  {
    U64 hash = e_hash_from_type_key(type_key);
    U64 slot_idx = hash%ev_auto_view_rule_table->slots_count;
    EV_AutoViewRuleSlot *slot = &ev_auto_view_rule_table->slots[slot_idx];
    EV_AutoViewRuleNode *node = 0;
    for(EV_AutoViewRuleNode *n = slot->first; n != 0; n = n->next)
    {
      if(e_type_match(n->key, type_key) && ((n->is_required && gather_required) || (!n->is_required && gather_optional)))
      {
        if(result == &ev_nil_view_rule_list)
        {
          result = push_array(arena, EV_ViewRuleList, 1);
        }
        ev_view_rule_list_push_string(arena, result, n->view_rule);
      }
    }
  }
  return result;
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
  if(string.size != 0)
  {
    MD_Node *root = md_tree_from_string(arena, string);
    for MD_EachNode(tln, root->first)
    {
      ev_view_rule_list_push_tree(arena, list, tln);
    }
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
ev_view_rule_list_from_expr_fastpaths(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // rjf: parse expression
  E_TokenArray tokens = e_token_array_from_text(scratch.arena, string);
  E_Parse parse = e_parse_expr_from_text_tokens(scratch.arena, string, &tokens);
  
  // rjf: extract view rules, encoded via fastpaths in expression string
  String8List fastpath_view_rules = {0};
  {
    U64 parse_opl = (parse.last_token >= tokens.v + tokens.count ? string.size : parse.last_token->range.min);
    U64 comma_pos = str8_find_needle(string, parse_opl, str8_lit(","), 0);
    U64 passthrough_pos = str8_find_needle(string, 0, str8_lit("--"), 0);
    if(comma_pos < string.size && comma_pos < passthrough_pos)
    {
      String8 comma_extension = str8_skip_chop_whitespace(str8_substr(string, r1u64(comma_pos+1, passthrough_pos)));
      if(str8_match(comma_extension, str8_lit("x"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(scratch.arena, &fastpath_view_rules, "hex");
      }
      else if(str8_match(comma_extension, str8_lit("b"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(scratch.arena, &fastpath_view_rules, "bin");
      }
      else if(str8_match(comma_extension, str8_lit("o"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(scratch.arena, &fastpath_view_rules, "oct");
      }
      else if(comma_extension.size != 0)
      {
        str8_list_pushf(scratch.arena, &fastpath_view_rules, "array:{%S}", comma_extension);
      }
    }
    if(passthrough_pos < string.size)
    {
      String8 passthrough_view_rule = str8_skip_chop_whitespace(str8_skip(string, passthrough_pos+2));
      if(passthrough_view_rule.size != 0)
      {
        str8_list_push(scratch.arena, &fastpath_view_rules, passthrough_view_rule);
      }
    }
  }
  
  // rjf: convert strings to parsed view rules
  EV_ViewRuleList *view_rule_list = push_array(arena, EV_ViewRuleList, 1);
  for(String8Node *n = fastpath_view_rules.first; n != 0; n = n->next)
  {
    ev_view_rule_list_push_string(arena, view_rule_list, push_str8_copy(arena, n->string));
  }
  
  scratch_end(scratch);
  return view_rule_list;
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

internal void
ev_view_rule_list_concat_in_place(EV_ViewRuleList *dst, EV_ViewRuleList **src)
{
  if(dst->first && src[0] != &ev_nil_view_rule_list && src[0]->first)
  {
    dst->last->next = src[0]->first;
    dst->last = src[0]->last;
    dst->count += src[0]->count;
  }
  else if(!dst->first)
  {
    MemoryCopyStruct(dst, *src);
  }
  *src = &ev_nil_view_rule_list;
}

////////////////////////////////
//~ rjf: Expression Resolution (Dynamic Overrides, View Rule Application)

internal E_Expr *
ev_resolved_from_expr(Arena *arena, E_Expr *expr, EV_ViewRuleList *view_rules)
{
  ProfBeginFunction();
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_Eval eval = e_eval_from_expr(scratch.arena, expr);
    E_TypeKey type_key = eval.type_key;
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
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
              E_TypeKey ptr_to_derived_type_key = e_type_key_cons_ptr(arch, derived_type_key, 0);
              expr = e_expr_ref_cast(arena, ptr_to_derived_type_key, expr);
            }
          }
        }
      }
    }
    scratch_end(scratch);
  }
  for(EV_ViewRuleNode *n = view_rules->first; n != 0; n = n->next)
  {
    EV_ViewRuleInfo *info = ev_view_rule_info_from_string(n->v.root->string);
    if(info->expr_resolution != 0)
    {
      expr = info->expr_resolution(arena, expr, n->v.root);
    }
  }
  ProfEnd();
  return expr;
}

////////////////////////////////
//~ rjf: Block Building

internal EV_BlockTree
ev_block_tree_from_expr(Arena *arena, EV_View *view, String8 filter, String8 string, E_Expr *expr, EV_ViewRuleList *view_rules)
{
  EV_BlockTree tree = {&ev_nil_block};
  {
    Temp scratch = scratch_begin(&arena, 1);
    EV_ViewRuleInfo *default_expand_view_rule_info = ev_view_rule_info_from_string(str8_lit("default"));
    
    //- rjf: form complete set of view rules
    EV_ViewRuleList *top_level_view_rules = ev_view_rule_list_copy(arena, view_rules);
    {
      E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, expr);
      EV_ViewRuleList *auto_view_rules = ev_auto_view_rules_from_type_key(arena, irtree.type_key, 1, 1);
      ev_view_rule_list_concat_in_place(top_level_view_rules, &auto_view_rules);
    }
    
    //- rjf: generate root block
    tree.root = push_array(arena, EV_Block, 1);
    MemoryCopyStruct(tree.root, &ev_nil_block);
    tree.root->key        = ev_key_root();
    tree.root->string     = string;
    tree.root->expr       = ev_resolved_from_expr(arena, expr, top_level_view_rules);
    tree.root->view_rules = top_level_view_rules;
    tree.root->row_count  = 1;
    tree.total_row_count += 1;
    tree.total_item_count += 1;
    
    //- rjf: iterate all expansions & generate blocks for each
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      EV_Block *parent_block;
      E_Expr *expr;
      U64 child_id;
      EV_ViewRuleList *view_rules;
      U64 split_relative_idx;
    };
    Task start_task = {0, tree.root, tree.root->expr, 1, top_level_view_rules, 0};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      // rjf: get task key
      EV_Key key = ev_key_make(ev_hash_from_key(t->parent_block->key), t->child_id);
      
      // rjf: obtain expansion node
      EV_ExpandNode *expand_node = ev_expand_node_from_key(view, key);
      B32 is_expanded = (expand_node != 0 && expand_node->expanded);
      
      // rjf: skip if not expanded
      if(!is_expanded)
      {
        continue;
      }
      
      // rjf: get expansion view rule info
      EV_ViewRuleInfo *expand_view_rule_info = default_expand_view_rule_info;
      MD_Node *expand_params = &md_nil_node;
      for(EV_ViewRuleNode *n = t->view_rules->first; n != 0; n = n->next)
      {
        EV_ViewRuleInfo *info = ev_view_rule_info_from_string(n->v.root->string);
        if(info->expr_expand_info != 0 && info->expr_expand_info != EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_NAME(nil))
        {
          expand_view_rule_info = info;
          expand_params = n->v.root;
        }
      }
      
      // rjf: get expansion info
      EV_ExpandInfo expand_info = expand_view_rule_info->expr_expand_info(arena, view, filter, t->expr, expand_params);
      
      // rjf: generate block for expansion
      EV_Block *expansion_block = &ev_nil_block;
      if(expand_info.row_count != 0)
      {
        expansion_block = push_array(arena, EV_Block, 1);
        MemoryCopyStruct(expansion_block, &ev_nil_block);
        DLLPushBack_NPZ(&ev_nil_block, t->parent_block->first, t->parent_block->last, expansion_block, next, prev);
        expansion_block->parent                   = t->parent_block;
        expansion_block->key                      = key;
        expansion_block->split_relative_idx       = t->split_relative_idx;
        expansion_block->expr                     = t->expr;
        expansion_block->view_rules               = t->view_rules;
        expansion_block->expand_view_rule_info    = expand_view_rule_info;
        expansion_block->expand_view_rule_params  = expand_params;
        expansion_block->expand_view_rule_info_user_data = expand_info.user_data;
        expansion_block->row_count                = expand_info.row_count;
        expansion_block->single_item              = expand_info.single_item;
        tree.total_row_count += expand_info.row_count;
        tree.total_item_count += expand_info.single_item ? 1 : expand_info.row_count;
      }
      
      // rjf: iterate children expansions, recurse
      // TODO(rjf): need to iterate these in index order, rather than "child_id" (which needs to be renamed to "child_id") order
      if(expand_node != 0 && expand_info.row_count != 0 && expand_view_rule_info->expr_expand_range_info)
      {
        // rjf: count children
        U64 child_count = 0;
        for(EV_ExpandNode *child = expand_node->first; child != 0; child = child->next, child_count += 1){}
        
        // rjf: gather children keys & numbers
        B32 needs_sort = 0;
        EV_Key *child_keys = push_array(scratch.arena, EV_Key, child_count);
        U64 *child_nums = push_array(scratch.arena, U64, child_count);
        {
          U64 idx = 0;
          for(EV_ExpandNode *child = expand_node->first; child != 0; child = child->next, idx += 1)
          {
            child_keys[idx] = child->key;
            child_nums[idx] = expand_view_rule_info->expr_expand_num_from_id(child->key.child_id, expand_info.user_data);
            if(child_nums[idx] != child_keys[idx].child_id)
            {
              needs_sort = 1;
            }
          }
        }
        
        // rjf: sort children by number, if needed
        if(needs_sort)
        {
          for(U64 idx1 = 0; idx1 < child_count; idx1 += 1)
          {
            U64 min_idx2 = 0;
            U64 min_num = child_nums[idx1];
            for(U64 idx2 = idx1+1; idx2 < child_count; idx2 += 1)
            {
              if(child_nums[idx2] < min_num)
              {
                min_idx2 = idx2;
                min_num = child_nums[idx2];
              }
            }
            if(min_idx2 != 0)
            {
              Swap(EV_Key, child_keys[idx1], child_keys[min_idx2]);
              Swap(U64, child_nums[idx1], child_nums[min_idx2]);
            }
          }
        }
        
        // rjf: iterate children expansions & generate recursion tasks
        for(U64 idx = 0; idx < child_count; idx += 1)
        {
          U64 split_num = child_nums[idx];
          U64 split_relative_idx = split_num - 1;
          if(split_relative_idx >= expand_info.row_count)
          {
            continue;
          }
          EV_ExpandRangeInfo child_expand = expand_view_rule_info->expr_expand_range_info(arena, view, filter, t->expr, expand_params, r1u64(split_relative_idx, split_relative_idx+1), expand_info.user_data);
          if(child_expand.row_exprs_count > 0)
          {
            EV_Key child_key = child_keys[idx];
            E_Expr *child_expr = child_expand.row_exprs[0];
            EV_ViewRuleList *child_view_rules = ev_view_rule_list_from_inheritance(arena, t->view_rules);
            String8 child_view_rule_string = ev_view_rule_from_key(view, child_key);
            ev_view_rule_list_push_string(arena, child_view_rules, child_view_rule_string);
            if(child_expand.row_strings[0].size != 0)
            {
              EV_ViewRuleList *fastpath_view_rules = ev_view_rule_list_from_expr_fastpaths(arena, child_expand.row_strings[0]);
              ev_view_rule_list_concat_in_place(child_view_rules, &fastpath_view_rules);
            }
            {
              Temp scratch = scratch_begin(&arena, 1);
              E_IRTreeAndType child_irtree = e_irtree_and_type_from_expr(scratch.arena, child_expr);
              EV_ViewRuleList *child_auto_view_rules = ev_auto_view_rules_from_type_key(arena, child_irtree.type_key, 1, child_view_rule_string.size == 0);
              ev_view_rule_list_concat_in_place(child_view_rules, &child_auto_view_rules);
              scratch_end(scratch);
            }
            E_Expr *child_expr__resolved = ev_resolved_from_expr(arena, child_expr, child_view_rules);
            // TODO(rjf): need to mix in child's view rules
            Task *task = push_array(scratch.arena, Task, 1);
            SLLQueuePush(first_task, last_task, task);
            task->parent_block       = expansion_block;
            task->expr               = child_expr__resolved;
            task->child_id           = child_key.child_id;
            task->view_rules         = child_view_rules;
            task->split_relative_idx = split_relative_idx;
          }
        }
      }
    }
    scratch_end(scratch);
  }
  return tree;
}

internal EV_BlockTree
ev_block_tree_from_string(Arena *arena, EV_View *view, String8 filter, String8 string, EV_ViewRuleList *view_rules)
{
  EV_BlockTree tree = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    E_TokenArray tokens = e_token_array_from_text(scratch.arena, string);
    E_Parse parse = e_parse_expr_from_text_tokens(arena, string, &tokens);
    EV_ViewRuleList *fastpath_view_rules = ev_view_rule_list_from_expr_fastpaths(arena, string);
    EV_ViewRuleList *all_view_rules = ev_view_rule_list_copy(arena, view_rules);
    ev_view_rule_list_concat_in_place(all_view_rules, &fastpath_view_rules);
    tree = ev_block_tree_from_expr(arena, view, filter, string, parse.expr, all_view_rules);
  }
  scratch_end(scratch);
  return tree;
}

internal U64
ev_depth_from_block(EV_Block *block)
{
  U64 depth = 0;
  for(EV_Block *b = block->parent; b != &ev_nil_block; b = b->parent)
  {
    depth += 1;
  }
  return depth;
}

////////////////////////////////
//~ rjf: Block Coordinate Spaces

internal EV_BlockRangeList
ev_block_range_list_from_tree(Arena *arena, EV_BlockTree *block_tree)
{
  EV_BlockRangeList list = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    typedef struct BlockTask BlockTask;
    struct BlockTask
    {
      BlockTask *next;
      EV_Block *block;
      EV_Block *next_child;
      Rng1U64 block_relative_range;
    };
    BlockTask start_task = {0, block_tree->root, block_tree->root->first, r1u64(0, block_tree->root->row_count)};
    for(BlockTask *t = &start_task; t != 0; t = t->next)
    {
      // rjf: get block-relative range, truncated by split position of next child
      Rng1U64 block_relative_range = t->block_relative_range;
      if(t->next_child != &ev_nil_block)
      {
        block_relative_range.max = t->next_child->split_relative_idx+1;
      }
      U64 block_num_visual_rows = dim_1u64(block_relative_range);
      
      // rjf: generate range node 
      if(block_num_visual_rows != 0)
      {
        EV_BlockRangeNode *n = push_array(arena, EV_BlockRangeNode, 1);
        n->v.block = t->block;
        n->v.range = block_relative_range;
        SLLQueuePush(list.first, list.last, n);
        list.count += 1;
      }
      
      // rjf: generate task for child, + for post-child parts of this block
      if(t->next_child != &ev_nil_block)
      {
        // rjf: generate task for child - do *before* remainder (descend block tree depth first)
        BlockTask *child_task = push_array(scratch.arena, BlockTask, 1);
        child_task->next = t->next;
        t->next = child_task;
        child_task->block = t->next_child;
        child_task->next_child = t->next_child->first;
        child_task->block_relative_range = r1u64(0, t->next_child->row_count);
        
        // rjf: generate task for post-child rows, if any, after children
        Rng1U64 remainder_range = r1u64(t->next_child->split_relative_idx+1, t->block_relative_range.max);
        if(remainder_range.max > remainder_range.min)
        {
          BlockTask *remainder_task = push_array(scratch.arena, BlockTask, 1);
          remainder_task->next = child_task->next;
          child_task->next = remainder_task;
          remainder_task->block = t->block;
          remainder_task->next_child = t->next_child->next;
          remainder_task->block_relative_range = remainder_range;
        }
      }
    }
    scratch_end(scratch);
  }
  return list;
}

internal EV_BlockRange
ev_block_range_from_num(EV_BlockRangeList *block_ranges, U64 num)
{
  EV_BlockRange result = {&ev_nil_block};
  U64 base_num = 0;
  for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
  {
    U64 range_size = n->v.block->single_item ? 1 : dim_1u64(n->v.range);
    Rng1U64 global_range = r1u64(base_num, base_num + range_size);
    if(contains_1u64(global_range, num))
    {
      result = n->v;
      break;
    }
    base_num += range_size;
  }
  return result;
}

internal EV_Key
ev_key_from_num(EV_BlockRangeList *block_ranges, U64 num)
{
  EV_Key key = {0};
  if(block_ranges->first)
  {
    key = ev_key_make(ev_hash_from_key(ev_key_root()), 1);
  }
  U64 base_num = 0;
  for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
  {
    U64 range_size = n->v.block->single_item ? 1 : dim_1u64(n->v.range);
    Rng1U64 global_range = r1u64(base_num, base_num + range_size);
    if(contains_1u64(global_range, num))
    {
      U64 relative_num = (num - base_num) + n->v.range.min + 1;
      U64 child_id     = n->v.block->expand_view_rule_info->expr_expand_id_from_num(relative_num, n->v.block->expand_view_rule_info_user_data);
      EV_Key block_key = n->v.block->key;
      key = ev_key_make(ev_hash_from_key(block_key), child_id);
      break;
    }
    base_num += range_size;
  }
  return key;
}

internal U64
ev_num_from_key(EV_BlockRangeList *block_ranges, EV_Key key)
{
  U64 result = 0;
  U64 base_num = 0;
  for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
  {
    U64 hash = ev_hash_from_key(n->v.block->key);
    if(hash == key.parent_hash)
    {
      U64 relative_num = n->v.block->expand_view_rule_info->expr_expand_num_from_id(key.child_id, n->v.block->expand_view_rule_info_user_data);
      Rng1U64 num_range = r1u64(n->v.range.min, n->v.block->single_item ? (n->v.range.min+1) : n->v.range.max);
      if(contains_1u64(num_range, relative_num-1))
      {
        result = base_num + (relative_num - 1 - n->v.range.min);
        break;
      }
    }
    base_num += n->v.block->single_item ? 1 : dim_1u64(n->v.range);
  }
  return result;
}

////////////////////////////////
//~ rjf: Row Building

internal EV_WindowedRowList
ev_windowed_row_list_from_block_range_list(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, Rng1U64 visible_range)
{
  EV_WindowedRowList rows = {0};
  {
    U64 visual_idx_off = 0;
    for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
    {
      // rjf: unpack this block/range pair
      Rng1U64 block_relative_range = n->v.range;
      U64 block_num_visual_rows = dim_1u64(block_relative_range);
      Rng1U64 block_global_range = r1u64(visual_idx_off, visual_idx_off + block_num_visual_rows);
      
      // rjf: get skip/chop of global range
      U64 num_skipped = 0;
      U64 num_chopped = 0;
      {
        if(visible_range.min > block_global_range.min)
        {
          num_skipped = (visible_range.min - block_global_range.min);
          num_skipped = Min(num_skipped, block_num_visual_rows);
        }
        if(visible_range.max < block_global_range.max)
        {
          num_chopped = (block_global_range.max - visible_range.max);
          num_chopped = Min(num_chopped, block_num_visual_rows);
        }
      }
      
      // rjf: get block-relative *windowed* range
      Rng1U64 block_relative_range__windowed = r1u64(block_relative_range.min + num_skipped,
                                                     block_relative_range.max - num_chopped);
      
      // rjf: sum & advance
      visual_idx_off += block_num_visual_rows;
      rows.count_before_visual += num_skipped;
      if(block_num_visual_rows != 0 && num_skipped != 0)
      {
        if(n->v.block->single_item)
        {
          if(num_skipped >= block_num_visual_rows)
          {
            rows.count_before_semantic += 1;
          }
        }
        else
        {
          rows.count_before_semantic += num_skipped;
        }
      }
      
      // rjf: generate rows before next splitting child
      if(block_relative_range__windowed.max > block_relative_range__windowed.min)
      {
        // rjf: get info about expansion range
        EV_ExpandRangeInfo expand_range_info = n->v.block->expand_view_rule_info->expr_expand_range_info(arena, view, filter, n->v.block->expr, n->v.block->expand_view_rule_params, block_relative_range__windowed, n->v.block->expand_view_rule_info_user_data);
        
        // rjf: no expansion operator applied -> push row for block expression; pass through block info
        if(expand_range_info.row_exprs_count == 0)
        {
          E_Member *member = &e_member_nil;
          if(n->v.block->expr->kind == E_ExprKind_MemberAccess)
          {
            Temp scratch = scratch_begin(&arena, 1);
            E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(scratch.arena, n->v.block->expr->first);
            E_TypeKey lhs_type_key = lhs_irtree.type_key;
            E_Member expr_member = e_type_member_from_key_name__cached(lhs_type_key, n->v.block->expr->last->string);
            if(expr_member.kind != E_MemberKind_Null)
            {
              member = push_array(arena, E_Member, 1);
              MemoryCopyStruct(member, &expr_member);
            }
            scratch_end(scratch);
          }
          EV_Row *row = push_array(arena, EV_Row, 1);
          SLLQueuePush(rows.first, rows.last, row);
          rows.count += 1;
          row->block                = n->v.block;
          row->key                  = ev_key_make(ev_hash_from_key(row->block->key), 1);
          row->visual_size          = n->v.block->single_item ? (n->v.block->row_count - (num_skipped + num_chopped)) : 1;
          row->visual_size_skipped  = num_skipped;
          row->visual_size_chopped  = num_chopped;
          row->string               = n->v.block->string;
          row->expr                 = n->v.block->expr;
          row->member               = member;
          row->view_rules           = n->v.block->view_rules;
        }
        
        // rjf: expansion operator applied -> call, and add rows for all expressions in the viewable range
        else
        {
          for EachIndex(idx, expand_range_info.row_exprs_count)
          {
            U64 child_num = block_relative_range.min + num_skipped + idx + 1;
            U64 child_id = n->v.block->expand_view_rule_info->expr_expand_id_from_num(child_num, n->v.block->expand_view_rule_info_user_data);
            EV_Key row_key = ev_key_make(ev_hash_from_key(n->v.block->key), child_id);
            E_Expr *row_expr = expand_range_info.row_exprs[idx];
            EV_ViewRuleList *row_view_rules = ev_view_rule_list_from_inheritance(arena, n->v.block->view_rules);
            String8 row_view_rule_string = ev_view_rule_from_key(view, row_key);
            ev_view_rule_list_push_string(arena, row_view_rules, row_view_rule_string);
            if(expand_range_info.row_strings[idx].size != 0)
            {
              EV_ViewRuleList *fastpath_view_rules = ev_view_rule_list_from_expr_fastpaths(arena, expand_range_info.row_strings[idx]);
              ev_view_rule_list_concat_in_place(row_view_rules, &fastpath_view_rules);
            }
            {
              Temp scratch = scratch_begin(&arena, 1);
              E_IRTreeAndType row_irtree = e_irtree_and_type_from_expr(scratch.arena, row_expr);
              EV_ViewRuleList *row_auto_view_rules = ev_auto_view_rules_from_type_key(arena, row_irtree.type_key, 1, row_view_rule_string.size == 0);
              ev_view_rule_list_concat_in_place(row_view_rules, &row_auto_view_rules);
              scratch_end(scratch);
            }
            E_Expr *row_expr__resolved = ev_resolved_from_expr(arena, row_expr, row_view_rules);
            EV_Row *row = push_array(arena, EV_Row, 1);
            SLLQueuePush(rows.first, rows.last, row);
            rows.count += 1;
            row->block                = n->v.block;
            row->key                  = row_key;
            row->visual_size          = 1;
            row->visual_size_skipped  = 0;
            row->visual_size_chopped  = 0;
            row->string               = expand_range_info.row_strings[idx];
            row->expr                 = row_expr__resolved;
            row->member               = expand_range_info.row_members[idx];
            row->view_rules           = row_view_rules;
            if(row->member == &e_member_nil || row->member == 0)
            {
              if(row->expr->kind == E_ExprKind_MemberAccess)
              {
                Temp scratch = scratch_begin(&arena, 1);
                E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr->first);
                E_TypeKey lhs_type_key = lhs_irtree.type_key;
                E_Member expr_member = e_type_member_from_key_name__cached(lhs_type_key, row->expr->last->string);
                if(expr_member.kind != E_MemberKind_Null)
                {
                  row->member = push_array(arena, E_Member, 1);
                  MemoryCopyStruct(row->member, &expr_member);
                }
                scratch_end(scratch);
              }
            }
            if(expand_range_info.row_view_rules[idx].size != 0)
            {
              ev_key_set_view_rule(view, row->key, expand_range_info.row_view_rules[idx]);
            }
          }
        }
      }
    }
  }
  return rows;
}

internal String8
ev_expr_string_from_row(Arena *arena, EV_Row *row, EV_StringFlags flags)
{
  String8 result = row->string;
  E_Expr *notable_expr = row->expr;
  for(B32 good = 0; !good;)
  {
    switch(notable_expr->kind)
    {
      default:{good = 1;}break;
      case E_ExprKind_Address:
      case E_ExprKind_Deref:
      case E_ExprKind_Cast:
      {
        notable_expr = notable_expr->last;
      }break;
      case E_ExprKind_Ref:
      {
        notable_expr = notable_expr->ref;
      }break;
    }
  }
  if(result.size == 0) switch(notable_expr->kind)
  {
    default:
    {
      result = e_string_from_expr(arena, notable_expr);
    }break;
    case E_ExprKind_ArrayIndex:
    {
      result = push_str8f(arena, "[%S]", e_string_from_expr(arena, notable_expr->last));
    }break;
    case E_ExprKind_MemberAccess:
    {
      if(flags & EV_StringFlag_PrettyNames && row->member->pretty_name.size != 0)
      {
        result = push_str8_copy(arena, row->member->pretty_name);
      }
      else
      {
        result = push_str8f(arena, ".%S", e_string_from_expr(arena, notable_expr->last));
      }
    }break;
  }
  return result;
}

internal B32
ev_row_is_expandable(EV_Row *row)
{
  B32 result = 0;
  {
    // rjf: determine if view rules force expandability
    if(!result)
    {
      for(EV_ViewRuleNode *n = row->view_rules->first; n != 0; n = n->next)
      {
        EV_ViewRuleInfo *info = ev_view_rule_info_from_string(n->v.root->string);
        if(info->flags & EV_ViewRuleInfoFlag_Expandable)
        {
          result = 1;
          break;
        }
      }
    }
    
    // rjf: determine if type info force expandability
    if(!result)
    {
      Temp scratch = scratch_begin(0, 0);
      E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr);
      result = ev_type_key_and_mode_is_expandable(irtree.type_key, irtree.mode);
      scratch_end(scratch);
    }
  }
  return result;
}

internal B32
ev_row_is_editable(EV_Row *row)
{
  B32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr);
  result = ev_type_key_is_editable(irtree.type_key);
  scratch_end(scratch);
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
ev_string_from_simple_typed_eval(Arena *arena, EV_StringFlags flags, U32 radix, U32 min_digits, E_Eval eval)
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
      result = str8_from_s64(arena, eval.value.s64, radix, min_digits, digit_group_separator);
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
        String8 value_string = str8_from_s64(scratch.arena, eval.value.u64, radix, min_digits, digit_group_separator);
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
        result = str8_from_s64(arena, eval.value.u64, radix, min_digits, digit_group_separator);
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
          String8 imm_string = str8_from_s64(arena, eval.value.s64, radix, min_digits, digit_group_separator);
          result = push_str8f(arena, "'%S' (%S)", char_str, imm_string);
        }
        else
        {
          result = push_str8f(arena, "'%S'", char_str);
        }
      }
      else
      {
        result = str8_from_s64(arena, eval.value.s64, radix, min_digits, digit_group_separator);
      }
    }break;
    
    case E_TypeKind_S8:
    case E_TypeKind_S16:
    case E_TypeKind_S32:
    case E_TypeKind_S64:
    {
      result = str8_from_s64(arena, eval.value.s64, radix, min_digits, digit_group_separator);
    }break;
    
    case E_TypeKind_U8:
    case E_TypeKind_U16:
    case E_TypeKind_U32:
    case E_TypeKind_U64:
    {
      result = str8_from_u64(arena, eval.value.u64, radix, min_digits, digit_group_separator);
    }break;
    
    case E_TypeKind_U128:
    {
      Temp scratch = scratch_begin(&arena, 1);
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
