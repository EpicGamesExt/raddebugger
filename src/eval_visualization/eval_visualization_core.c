// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/eval_visualization.meta.c"

////////////////////////////////
//~ rjf: Nil/Identity View Rule Hooks

EV_EXPAND_RULE_INFO_FUNCTION_DEF(nil)
{
  EV_ExpandInfo info = {0};
  return info;
}

////////////////////////////////
//~ rjf: Key Functions

#if !defined(XXH_IMPLEMENTATION)
# define XXH_IMPLEMENTATION
# define XXH_STATIC_LINKING_ONLY
# include "third_party/xxHash/xxhash.h"
#endif

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
  U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
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
       kind == E_TypeKind_Set ||
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
ev_expand_rule_table_push(Arena *arena, EV_ExpandRuleTable *table, EV_ExpandRule *info)
{
  if(table->slots_count == 0)
  {
    table->slots_count = 512;
    table->slots = push_array(arena, EV_ExpandRuleSlot, table->slots_count);
  }
  U64 hash = ev_hash_from_seed_string(5381, info->string);
  U64 slot_idx = hash%table->slots_count;
  EV_ExpandRuleSlot *slot = &table->slots[slot_idx];
  EV_ExpandRuleNode *n = push_array(arena, EV_ExpandRuleNode, 1);
  SLLQueuePush(slot->first, slot->last, n);
  MemoryCopyStruct(&n->v, info);
  n->v.string = push_str8_copy(arena, n->v.string);
}

internal void
ev_select_expand_rule_table(EV_ExpandRuleTable *table)
{
  ev_view_rule_info_table = table;
}

internal EV_ExpandRule *
ev_expand_rule_from_string(String8 string)
{
  EV_ExpandRule *info = &ev_nil_expand_rule;
  if(ev_view_rule_info_table != 0 && ev_view_rule_info_table->slots_count != 0)
  {
    U64 hash = ev_hash_from_seed_string(5381, string);
    U64 slot_idx = hash%ev_view_rule_info_table->slots_count;
    EV_ExpandRuleSlot *slot = &ev_view_rule_info_table->slots[slot_idx];
    EV_ExpandRuleNode *node = 0;
    for(EV_ExpandRuleNode *n = slot->first; n != 0; n = n->next)
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
  }
  return info;
}

////////////////////////////////
//~ rjf: Expression Resolution (Dynamic Overrides, View Rule Application)

#if 0 // TODO(rjf): @cfg
internal E_Expr *
ev_resolved_from_expr(Arena *arena, E_Expr *expr)
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
              expr = e_expr_ref_cast(arena, ptr_to_derived_type_key, expr);
            }
          }
        }
      }
    }
    scratch_end(scratch);
  }
  ProfEnd();
  return expr;
}
#endif

////////////////////////////////
//~ rjf: Upgrading Expressions w/ Tags From All Sources

internal void
ev_keyed_expr_push_tags(Arena *arena, EV_View *view, EV_Block *block, EV_Key key, E_Expr *expr)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // rjf: push inherited tags first (we want these to be found first, since tags are applied
  // in order, and explicit ones should always be strongest)
  for(EV_Block *b = block; b != &ev_nil_block; b = b->parent)
  {
    for(E_Expr *src_tag = b->expr->first_tag; src_tag != &e_expr_nil; src_tag = src_tag->next)
    {
      e_expr_push_tag(expr, e_expr_copy(arena, src_tag));
    }
  }
  
  // rjf: push explicitly-attached tags (via key) next
  String8 tag_expr = push_str8_copy(arena, ev_view_rule_from_key(view, key));
  E_TokenArray tag_expr_tokens = e_token_array_from_text(scratch.arena, tag_expr);
  E_Parse tag_expr_parse = e_parse_expr_from_text_tokens(arena, tag_expr, &tag_expr_tokens);
  for(E_Expr *tag = tag_expr_parse.expr, *next = &e_expr_nil; tag != &e_expr_nil; tag = next)
  {
    next = tag->next;
    e_expr_push_tag(expr, tag);
  }
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: Block Building

internal EV_BlockTree
ev_block_tree_from_string(Arena *arena, EV_View *view, String8 filter, String8 string)
{
  ProfBeginFunction();
  EV_BlockTree tree = {&ev_nil_block};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: produce root expression
    EV_Key root_key = ev_key_root();
    EV_Key root_row_key = ev_key_make(ev_hash_from_key(root_key), 1);
    E_TokenArray root_tokens = e_token_array_from_text(scratch.arena, string);
    E_Parse root_parse = e_parse_expr_from_text_tokens(arena, string, &root_tokens);
    E_Expr *root_expr = root_parse.expr;
    ev_keyed_expr_push_tags(arena, view, &ev_nil_block, root_row_key, root_expr);
    
    //- rjf: generate root block
    tree.root = push_array(arena, EV_Block, 1);
    MemoryCopyStruct(tree.root, &ev_nil_block);
    tree.root->key        = root_key;
    tree.root->string     = string;
    tree.root->expr       = root_expr;
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
      U64 split_relative_idx;
      B32 default_expanded;
    };
    Task start_task = {0, tree.root, tree.root->expr, 1, 0};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      // rjf: get task key
      EV_Key key = ev_key_make(ev_hash_from_key(t->parent_block->key), t->child_id);
      
      // rjf: obtain expansion node & expansion state
      EV_ExpandNode *expand_node = ev_expand_node_from_key(view, key);
      B32 is_expanded = (expand_node != 0 && expand_node->expanded);
      if(t->default_expanded)
      {
        is_expanded ^= 1;
      }
      
      // rjf: skip if not expanded
      if(!is_expanded)
      {
        continue;
      }
      
      // rjf: unpack expr
      E_IRTreeAndType expr_irtree = e_irtree_and_type_from_expr(scratch.arena, t->expr);
      
      // rjf: skip if type info disallows expansion
      if(!ev_type_key_and_mode_is_expandable(expr_irtree.type_key, expr_irtree.mode))
      {
        continue;
      }
      
      // rjf: get expr's lookup rule
      E_LookupRuleTagPair lookup_rule_and_tag = e_lookup_rule_tag_pair_from_expr_irtree(t->expr, &expr_irtree);
      E_LookupRule *lookup_rule = lookup_rule_and_tag.rule;
      E_Expr *lookup_rule_tag = lookup_rule_and_tag.tag;
      
      // rjf: get expr's expansion rule
      EV_ExpandRuleTagPair expand_rule_and_tag = ev_expand_rule_tag_pair_from_expr_irtree(t->expr, &expr_irtree);
      EV_ExpandRule *expand_rule = expand_rule_and_tag.rule;
      E_Expr *expand_rule_tag = expand_rule_and_tag.tag;
      
      // rjf: get top-level lookup/expansion info
      E_LookupInfo lookup_info = lookup_rule->info(arena, &expr_irtree, filter);
      EV_ExpandInfo expand_info = expand_rule->info(arena, view, filter, t->expr, expand_rule_tag);
      
      // rjf: determine expansion info
      U64 expansion_row_count = lookup_info.named_expr_count ? lookup_info.named_expr_count : lookup_info.idxed_expr_count;
      if(expand_rule != &ev_nil_expand_rule)
      {
        expansion_row_count = expand_info.row_count;
      }
      
      // rjf: generate block for expansion
      EV_Block *expansion_block = &ev_nil_block;
      if(expansion_row_count != 0)
      {
        expansion_block = push_array(arena, EV_Block, 1);
        MemoryCopyStruct(expansion_block, &ev_nil_block);
        DLLPushBack_NPZ(&ev_nil_block, t->parent_block->first, t->parent_block->last, expansion_block, next, prev);
        expansion_block->parent                   = t->parent_block;
        expansion_block->key                      = key;
        expansion_block->split_relative_idx       = t->split_relative_idx;
        expansion_block->expr                     = t->expr;
        expansion_block->lookup_tag               = lookup_rule_tag;
        expansion_block->expand_tag               = expand_rule_tag;
        expansion_block->lookup_rule              = lookup_rule;
        expansion_block->expand_rule              = expand_rule;
        expansion_block->lookup_rule_user_data    = lookup_info.user_data;
        expansion_block->expand_rule_user_data    = expand_info.user_data;
        expansion_block->row_count                = expansion_row_count;
        expansion_block->single_item              = expand_info.single_item;
        expansion_block->rows_default_expanded    = expand_info.rows_default_expanded;
        tree.total_row_count += expansion_row_count;
        tree.total_item_count += expand_info.single_item ? 1 : expansion_row_count;
      }
      
      // rjf: gather children expansions from expansion state
      U64 child_count = 0;
      EV_Key *child_keys = 0;
      U64 *child_nums = 0;
      if(!child_count && !expand_info.rows_default_expanded && expand_node != 0 && expansion_row_count != 0)
      {
        // rjf: count children
        for(EV_ExpandNode *child = expand_node->first; child != 0; child = child->next, child_count += 1){}
        
        // rjf: gather children keys & numbers
        B32 needs_sort = 0;
        child_keys = push_array(scratch.arena, EV_Key, child_count);
        child_nums = push_array(scratch.arena, U64, child_count);
        {
          U64 idx = 0;
          for(EV_ExpandNode *child = expand_node->first; child != 0; child = child->next, idx += 1)
          {
            child_keys[idx] = child->key;
            child_nums[idx] = lookup_rule->num_from_id(child->key.child_id, lookup_info.user_data);
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
      }
      
      // rjf: gather children expansions from inverse of expansion state
      if(!child_count && (expand_info.rows_default_expanded || (expand_node == 0 && !expand_info.rows_default_expanded)))
      {
        child_count = expand_info.row_count;
        child_keys  = push_array(scratch.arena, EV_Key, child_count);
        child_nums  = push_array(scratch.arena, U64,    child_count);
        for(U64 idx = 0; idx < child_count; idx += 1)
        {
          U64 child_id = lookup_rule->id_from_num(idx+1, lookup_info.user_data);
          child_keys[idx] = ev_key_make(ev_hash_from_key(key), child_id);
          child_nums[idx] = idx+1;
        }
      }
      
      // rjf: iterate children expansions & generate recursion tasks
      for(U64 idx = 0; idx < child_count; idx += 1)
      {
        U64 split_num = child_nums[idx];
        U64 split_relative_idx = split_num - 1;
        if(split_relative_idx >= expansion_row_count)
        {
          continue;
        }
        if(expand_info.rows_default_expanded || ev_expansion_from_key(view, child_keys[idx]))
        {
          Rng1U64 child_range = r1u64(split_relative_idx, split_relative_idx+1);
          E_Expr *child_expr = &e_expr_nil;
          String8 child_string = {0};
          lookup_rule->range(arena, t->expr, r1u64(split_relative_idx, split_relative_idx+1), &child_expr, &child_string, lookup_info.user_data);
          if(child_expr != &e_expr_nil)
          {
            EV_Key child_key = child_keys[idx];
            ev_keyed_expr_push_tags(arena, view, expansion_block, child_key, child_expr);
            Task *task = push_array(scratch.arena, Task, 1);
            SLLQueuePush(first_task, last_task, task);
            task->parent_block       = expansion_block;
            task->expr               = child_expr;
            task->child_id           = child_key.child_id;
            task->split_relative_idx = split_relative_idx;
            task->default_expanded   = expand_info.rows_default_expanded;
          }
        }
      }
    }
    scratch_end(scratch);
  }
  ProfEnd();
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
  U64 base_num = 1;
  for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
  {
    U64 range_size = n->v.block->single_item ? 1 : dim_1u64(n->v.range);
    Rng1U64 global_range = r1u64(base_num, base_num + range_size);
    if(contains_1u64(global_range, num))
    {
      U64 relative_num = (num - base_num) + n->v.range.min + 1;
      U64 child_id     = n->v.block->lookup_rule->id_from_num(relative_num, n->v.block->lookup_rule_user_data);
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
  U64 base_num = 1;
  for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
  {
    U64 hash = ev_hash_from_key(n->v.block->key);
    if(hash == key.parent_hash)
    {
      U64 relative_num = n->v.block->lookup_rule->num_from_id(key.child_id, n->v.block->lookup_rule_user_data);
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

internal U64
ev_vnum_from_num(EV_BlockRangeList *block_ranges, U64 num)
{
  U64 vnum = 0;
  {
    U64 base_vnum = 1;
    U64 base_num = 1;
    for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
    {
      U64 next_base_num = base_num + (n->v.block->single_item ? 1 : dim_1u64(n->v.range));
      if(base_num <= num && num < next_base_num)
      {
        U64 relative_vnum = (n->v.block->single_item ? 0 : (num - base_num));
        vnum = base_vnum + relative_vnum;
        break;
      }
      base_num = next_base_num;
      base_vnum += dim_1u64(n->v.range);
    }
    if(vnum == 0)
    {
      vnum = base_vnum;
    }
  }
  return vnum;
}

internal U64
ev_num_from_vnum(EV_BlockRangeList *block_ranges, U64 vnum)
{
  U64 num = 0;
  {
    U64 base_vnum = 1;
    U64 base_num = 1;
    for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
    {
      U64 next_base_vnum = base_vnum + dim_1u64(n->v.range);
      if(base_vnum <= vnum && vnum < next_base_vnum)
      {
        U64 relative_num = (n->v.block->single_item ? 0 : (vnum - base_vnum));
        num = base_num + relative_num;
        break;
      }
      base_vnum = next_base_vnum;
      base_num += (n->v.block->single_item ? 1 : dim_1u64(n->v.range));
    }
  }
  return num;
}

////////////////////////////////
//~ rjf: Row Building

internal EV_WindowedRowList
ev_windowed_row_list_from_block_range_list(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, Rng1U64 vnum_range)
{
  EV_WindowedRowList rows = {0};
  {
    U64 base_vnum = 1;
    for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
    {
      // rjf: unpack this block/range pair
      Rng1U64 block_relative_range = n->v.range;
      U64 block_num_visual_rows = dim_1u64(block_relative_range);
      Rng1U64 block_global_range = r1u64(base_vnum, base_vnum + block_num_visual_rows);
      
      // rjf: get skip/chop of global range
      U64 num_skipped = 0;
      U64 num_chopped = 0;
      {
        if(vnum_range.min > block_global_range.min)
        {
          num_skipped = (vnum_range.min - block_global_range.min);
          num_skipped = Min(num_skipped, block_num_visual_rows);
        }
        if(vnum_range.max < block_global_range.max)
        {
          num_chopped = (block_global_range.max - vnum_range.max);
          num_chopped = Min(num_chopped, block_num_visual_rows);
        }
      }
      
      // rjf: get block-relative *windowed* range
      Rng1U64 block_relative_range__windowed = r1u64(block_relative_range.min + num_skipped,
                                                     block_relative_range.max - num_chopped);
      
      // rjf: sum & advance
      base_vnum += block_num_visual_rows;
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
        B32 is_root = 0;
        U64 range_exprs_count = dim_1u64(block_relative_range__windowed);
        E_Expr **range_exprs = push_array(arena, E_Expr *, range_exprs_count);
        String8 *range_exprs_strings = push_array(arena, String8 ,range_exprs_count);
        for EachIndex(idx, range_exprs_count)
        {
          range_exprs[idx] = &e_expr_nil;
        }
        if(n->v.block->lookup_rule == &e_lookup_rule__nil)
        {
          is_root = 1;
        }
        else
        {
          n->v.block->lookup_rule->range(arena, n->v.block->expr, block_relative_range__windowed, range_exprs, range_exprs_strings, n->v.block->lookup_rule_user_data);
        }
        
        // rjf: no expansion operator applied -> push row for block expression; pass through block info
        if(is_root)
        {
          EV_WindowedRowNode *row_node = push_array(arena, EV_WindowedRowNode, 1);
          SLLQueuePush(rows.first, rows.last, row_node);
          rows.count += 1;
          row_node->visual_size_skipped = num_skipped;
          row_node->visual_size_chopped = num_chopped;
          EV_Row *row = &row_node->row;
          row->block         = n->v.block;
          row->key           = ev_key_make(ev_hash_from_key(row->block->key), 1);
          row->visual_size   = n->v.block->single_item ? (n->v.block->row_count - (num_skipped + num_chopped)) : 1;
          row->string        = n->v.block->string;
          row->expr          = n->v.block->expr;
        }
        
        // rjf: expansion operator applied -> call, and add rows for all expressions in the viewable range
        else for EachIndex(idx, range_exprs_count)
        {
          U64 child_num = block_relative_range.min + num_skipped + idx + 1;
          U64 child_id = n->v.block->lookup_rule->id_from_num(child_num, n->v.block->lookup_rule_user_data);
          EV_Key row_key = ev_key_make(ev_hash_from_key(n->v.block->key), child_id);
          E_Expr *row_expr = range_exprs[idx];
          ev_keyed_expr_push_tags(arena, view, n->v.block, row_key, row_expr);
          EV_WindowedRowNode *row_node = push_array(arena, EV_WindowedRowNode, 1);
          SLLQueuePush(rows.first, rows.last, row_node);
          rows.count += 1;
          EV_Row *row = &row_node->row;
          row->block                = n->v.block;
          row->key                  = row_key;
          row->visual_size          = 1;
          row->string               = range_exprs_strings[idx];
          row->expr                 = row_expr;
        }
      }
    }
  }
  return rows;
}

internal EV_Row *
ev_row_from_num(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, U64 num)
{
  U64 vidx = ev_vnum_from_num(block_ranges, num);
  EV_WindowedRowList rows = ev_windowed_row_list_from_block_range_list(arena, view, filter, block_ranges, r1u64(vidx, vidx+1));
  EV_Row *result = 0;
  if(rows.first != 0)
  {
    result = &rows.first->row;
  }
  else
  {
    result = push_array(arena, EV_Row, 1);
    result->block = &ev_nil_block;
    result->expr = &e_expr_nil;
  }
  return result;
}

internal EV_WindowedRowList
ev_rows_from_num_range(Arena *arena, EV_View *view, String8 filter, EV_BlockRangeList *block_ranges, Rng1U64 num_range)
{
  Rng1U64 vnum_range = r1u64(ev_vnum_from_num(block_ranges, num_range.min), ev_vnum_from_num(block_ranges, num_range.max)+1);
  EV_WindowedRowList rows = ev_windowed_row_list_from_block_range_list(arena, view, filter, block_ranges, vnum_range);
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
      result = push_str8f(arena, ".%S", e_string_from_expr(arena, notable_expr->last));
    }break;
  }
  return result;
}

internal B32
ev_row_is_expandable(EV_Row *row)
{
  B32 result = 0;
  {
    Temp scratch = scratch_begin(0, 0);
    E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr);
    
    // rjf: determine if view rules force expandability
    if(!result)
    {
      EV_ExpandRuleTagPair expand_rule_and_tag = ev_expand_rule_tag_pair_from_expr_irtree(row->expr, &irtree);
      if(expand_rule_and_tag.rule != &ev_nil_expand_rule)
      {
        result = 1;
      }
    }
    
    // rjf: determine if type info force expandability
    if(!result)
    {
      E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr);
      result = ev_type_key_and_mode_is_expandable(irtree.type_key, irtree.mode);
    }
    scratch_end(scratch);
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
  F64 f64 = 0;
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
      B32 type_is_unsigned = (E_TypeKind_UChar8 <= type_kind && type_kind <= E_TypeKind_UChar32);
      String8 char_str = ev_string_from_ascii_value(arena, eval.value.s64);
      if(char_str.size != 0)
      {
        if(flags & EV_StringFlag_ReadOnlyDisplayRules)
        {
          String8 imm_string = (type_is_unsigned
                                ? str8_from_u64(arena, eval.value.u64, radix, min_digits, digit_group_separator)
                                : str8_from_s64(arena, eval.value.s64, radix, min_digits, digit_group_separator));
          result = push_str8f(arena, "'%S' (%S)", char_str, imm_string);
        }
        else
        {
          result = push_str8f(arena, "'%S'", char_str);
        }
      }
      else
      {
        result = (type_is_unsigned
                  ? str8_from_u64(arena, eval.value.u64, radix, min_digits, digit_group_separator)
                  : str8_from_s64(arena, eval.value.s64, radix, min_digits, digit_group_separator));
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
    
    case E_TypeKind_F32:{f64 = (F64)eval.value.f32;}goto f64_path;
    case E_TypeKind_F64:{f64 = eval.value.f64;}goto f64_path;
    f64_path:;
    {
      result = push_str8f(arena, "%.*f", min_digits ? min_digits : 16, f64);
      U64 num_to_chop = 0;
      for(U64 num_trimmed = 0; num_trimmed < result.size; num_trimmed += 1)
      {
        if(result.str[result.size - 1 - num_trimmed] != '0')
        {
          if(result.str[result.size - 1 - num_trimmed] == '.' && num_to_chop > 0)
          {
            num_to_chop -= 1;
          }
          break;
        }
        num_to_chop += 1;
      }
      result = str8_chop(result, num_to_chop);
    }break;
    case E_TypeKind_Bool:{result = push_str8f(arena, "%s", eval.value.u64 ? "true" : "false");}break;
    case E_TypeKind_Ptr: {result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_LRef:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_RRef:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_Function:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    
    case E_TypeKind_Enum:
    {
      Temp scratch = scratch_begin(&arena, 1);
      E_Type *type = e_type_from_key__cached(type_key);
      String8 constant_name = {0};
      for(U64 val_idx = 0; val_idx < type->count; val_idx += 1)
      {
        if(eval.value.u64 == type->enum_vals[val_idx].val)
        {
          constant_name = type->enum_vals[val_idx].name;
          break;
        }
      }
      String8 numeric_value_string = str8_from_u64(scratch.arena, eval.value.u64, radix, min_digits, digit_group_separator);
      if(flags & EV_StringFlag_ReadOnlyDisplayRules)
      {
        if(constant_name.size != 0)
        {
          result = push_str8f(arena, "%S (%S)", numeric_value_string, constant_name);
        }
        else
        {
          result = push_str8_copy(arena, numeric_value_string);
        }
      }
      else if(constant_name.size != 0)
      {
        result = push_str8_copy(arena, constant_name);
      }
      else
      {
        result = push_str8_copy(arena, numeric_value_string);
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

////////////////////////////////
//~ rjf: Expression & IR-Tree => Expand Rule

internal EV_ExpandRuleTagPair
ev_expand_rule_tag_pair_from_expr_irtree(E_Expr *expr, E_IRTreeAndType *irtree)
{
  EV_ExpandRuleTagPair result = {&ev_nil_expand_rule, &e_expr_nil};
  {
    // rjf: first try explicitly-stored tags
    if(result.rule == &ev_nil_expand_rule)
    {
      for(E_Expr *tag = expr->first_tag; tag != &e_expr_nil; tag = tag->next)
      {
        EV_ExpandRule *candidate = ev_expand_rule_from_string(tag->string);
        if(candidate != &ev_nil_expand_rule)
        {
          result.rule = candidate;
          result.tag = tag;
          break;
        }
      }
    }
    
    // rjf: next try implicit set name -> rule mapping
    if(result.rule == &ev_nil_expand_rule)
    {
      E_TypeKind type_kind = e_type_kind_from_key(irtree->type_key);
      if(type_kind == E_TypeKind_Set)
      {
        E_Type *type = e_type_from_key__cached(irtree->type_key);
        String8 name = type->name;
        EV_ExpandRule *candidate = ev_expand_rule_from_string(name);
        if(candidate != &ev_nil_expand_rule)
        {
          result.rule = candidate;
        }
      }
    }
    
    // rjf: next try auto hook map
    if(result.rule == &ev_nil_expand_rule)
    {
      E_ExprList tags = e_auto_hook_tag_exprs_from_type_key__cached(irtree->type_key);
      for(E_ExprNode *n = tags.first; n != 0; n = n->next)
      {
        EV_ExpandRule *candidate = ev_expand_rule_from_string(n->v->string);
        if(candidate != &ev_nil_expand_rule)
        {
          result.rule = candidate;
          result.tag = n->v;
          break;
        }
      }
    }
  }
  return result;
}
