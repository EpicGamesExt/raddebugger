// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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

internal E_TypeKey
ev_expansion_type_from_key(E_TypeKey type_key)
{
  E_TypeKey result = zero_struct;
  for(E_TypeKey key = type_key;
      !e_type_key_match(key, e_type_key_zero());
      key = e_type_key_direct(key))
  {
    B32 done = 1;
    E_TypeKind kind = e_type_kind_from_key(key);
    
    //- rjf: lenses -> try to see if this lens has special expansion rules. if
    // so, choose the current eval
    if(kind == E_TypeKind_Lens)
    {
      E_Type *type = e_type_from_key(key);
      if(type->expand.info != 0 ||
         ev_expand_rule_from_string(type->name) != &ev_nil_expand_rule)
      {
        done = 1;
        result = key;
      }
      else
      {
        done = 0;
      }
    }
    
    //- rjf: if we have meta-expression tags in the type chain, defer
    // to the next type in the chain.
    else if(E_TypeKind_FirstMeta <= kind && kind <= E_TypeKind_LastMeta)
    {
      done = 0;
    }
    
    //- rjf: break if done
    if(done)
    {
      break;
    }
  }
  return result;
}

internal B32
ev_type_key_and_mode_is_expandable(E_TypeKey type_key, E_Mode mode)
{
  B32 result = 0;
  E_TypeKey ev_expansion_type_key = ev_expansion_type_from_key(type_key);
  if(!e_type_key_match(ev_expansion_type_key, e_type_key_zero()))
  {
    result = 1;
  }
  else
  {
    E_TypeKey default_expansion_type_key = e_default_expansion_type_from_key(type_key);
    E_TypeKind kind = e_type_kind_from_key(default_expansion_type_key);
    if(kind == E_TypeKind_Enum)
    {
      result = (mode == E_Mode_Null);
    }
    else if(kind != E_TypeKind_Null)
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
  B32 done = 0;
  for(E_TypeKey t = type_key; !result && !done; t = e_type_key_direct(t))
  {
    E_TypeKind kind = e_type_kind_from_key(t);
    switch(kind)
    {
      case E_TypeKind_Null:
      case E_TypeKind_Function:
      {
        result = 0;
        done = 1;
      }break;
      default:
      if((E_TypeKind_FirstBasic <= kind && kind <= E_TypeKind_LastBasic) || e_type_kind_is_pointer_or_ref(kind))
      {
        result = 1;
        done = 1;
      }break;
      case E_TypeKind_Array:
      {
        E_Type *type = e_type_from_key(t);
        if(type->flags & E_TypeFlag_IsNotText)
        {
          result = 0;
          done = 1;
        }
        else
        {
          E_TypeKind element_kind = e_type_kind_from_key(e_type_key_unwrap(t, E_TypeUnwrapFlag_All));
          result = (element_kind == E_TypeKind_U8 ||
                    element_kind == E_TypeKind_U16 ||
                    element_kind == E_TypeKind_U32 ||
                    element_kind == E_TypeKind_S8 ||
                    element_kind == E_TypeKind_S16 ||
                    element_kind == E_TypeKind_S32 ||
                    element_kind == E_TypeKind_UChar8 ||
                    element_kind == E_TypeKind_UChar16 ||
                    element_kind == E_TypeKind_UChar32 ||
                    element_kind == E_TypeKind_Char8 ||
                    element_kind == E_TypeKind_Char16 ||
                    element_kind == E_TypeKind_Char32);
          done = 1;
        }
      }break;
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

internal EV_ExpandRule *
ev_expand_rule_from_type_key(E_TypeKey type_key)
{
  EV_ExpandRule *rule = &ev_nil_expand_rule;
  {
    E_TypeKey k = e_type_key_unwrap(type_key, E_TypeUnwrapFlag_Meta);
    E_TypeKind kind = e_type_kind_from_key(k);
    for(;kind == E_TypeKind_Lens; k = e_type_key_direct(e_type_key_unwrap(k, E_TypeUnwrapFlag_Meta)), kind = e_type_kind_from_key(k))
    {
      E_Type *type = e_type_from_key(k);
      EV_ExpandRule *candidate = ev_expand_rule_from_string(type->name);
      if(candidate != &ev_nil_expand_rule)
      {
        rule = candidate;
        break;
      }
    }
  }
  return rule;
}

////////////////////////////////
//~ rjf: Block Building

internal EV_BlockTree
ev_block_tree_from_eval(Arena *arena, EV_View *view, String8 filter, E_Eval root_eval)
{
  ProfBeginFunction();
  EV_BlockTree tree = {&ev_nil_block};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: generate root expression
    EV_Key root_key = ev_key_root();
    EV_Key root_row_key = ev_key_make(ev_hash_from_key(root_key), 1);
    
    //- rjf: generate root block
    tree.root = push_array(arena, EV_Block, 1);
    MemoryCopyStruct(tree.root, &ev_nil_block);
    tree.root->key              = root_key;
    tree.root->string           = str8_zero();
    tree.root->eval             = root_eval;
    tree.root->type_expand_rule = &e_type_expand_rule__default;
    tree.root->viz_expand_rule  = &ev_nil_expand_rule;
    tree.root->row_count  = 1;
    tree.total_row_count += 1;
    tree.total_item_count += 1;
    
    //- rjf: generate initial task, for root's evaluation
    typedef struct BlockTreeBuildTask BlockTreeBuildTask;
    struct BlockTreeBuildTask
    {
      BlockTreeBuildTask *next;
      EV_Block *parent_block;
      E_Eval eval;
      E_Expr *next_expr;
      U64 child_id;
      U64 split_relative_idx;
      B32 default_expanded;
      B32 force_expanded;
      S32 depth;
    };
    BlockTreeBuildTask start_task = {0, tree.root, tree.root->eval, tree.root->eval.expr->next, 1, 0};
    BlockTreeBuildTask *first_task = &start_task;
    BlockTreeBuildTask *last_task = first_task;
    
    //- rjf: iterate all expansions & generate blocks for each
    for(BlockTreeBuildTask *t = first_task; t != 0; t = t->next)
    {
      // rjf: get task key
      EV_Key key = ev_key_make(ev_hash_from_key(t->parent_block->key), t->child_id);
      
      // rjf: obtain expansion node & expansion state
      EV_ExpandNode *expand_node = ev_expand_node_from_key(view, key);
      B32 is_expanded = (expand_node != 0 && expand_node->expanded);
      if(t->default_expanded || t->force_expanded)
      {
        is_expanded ^= 1;
      }
      
      // rjf: skip if not expanded
      if(!is_expanded)
      {
        continue;
      }
      
      // rjf: unpack eval
      E_Mode mode = t->eval.irtree.mode;
      E_Eval eval = t->eval;
      E_TypeKey expansion_type_key = ev_expansion_type_from_key(eval.irtree.type_key);
      if(!e_type_key_match(expansion_type_key, e_type_key_zero()))
      {
        eval.irtree.type_key = expansion_type_key;
      }
      
      // rjf: get expansion rules from type
      E_TypeExpandRule *type_expand_rule = e_expand_rule_from_type_key(eval.irtree.type_key);
      EV_ExpandRule *viz_expand_rule = ev_expand_rule_from_type_key(eval.irtree.type_key);
      
      // rjf: skip if no expansion rule, & type info disallows expansion
      if(viz_expand_rule == &ev_nil_expand_rule && !ev_type_key_and_mode_is_expandable(eval.irtree.type_key, mode))
      {
        continue;
      }
      
      // rjf: get filter for this task
      String8 task_filter = t->depth == 0 ? filter : str8_zero();
      
      // rjf: get top-level lookup/expansion info
      E_TypeExpandInfo type_expand_info = type_expand_rule->info(arena, eval, task_filter);
      EV_ExpandInfo viz_expand_info = viz_expand_rule->info(arena, view, task_filter, eval.expr);
      
      // rjf: determine expansion info
      U64 expansion_row_count = type_expand_info.expr_count;
      if(viz_expand_rule != &ev_nil_expand_rule)
      {
        expansion_row_count = viz_expand_info.row_count;
      }
      expansion_row_count = Min(0x0fffffffffffffffull, expansion_row_count);
      
      // rjf: determine if this expansion supports child expansions
      B32 allow_child_expansions = 1;
      if(viz_expand_info.single_item)
      {
        // NOTE(rjf): for now, just plugging in the heuristic of "is this a single row (a.k.a. visualizer)?"
        allow_child_expansions = 0;
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
        expansion_block->eval                     = eval;
        expansion_block->filter                   = task_filter;
        expansion_block->type_expand_info         = type_expand_info;
        expansion_block->type_expand_rule         = type_expand_rule;
        expansion_block->viz_expand_info          = viz_expand_info;
        expansion_block->viz_expand_rule          = viz_expand_rule;
        expansion_block->row_count                = expansion_row_count;
        tree.total_row_count += expansion_row_count;
        tree.total_item_count += viz_expand_info.single_item ? 1 : expansion_row_count;
      }
      
      // rjf: gather children expansions from expansion state
      U64 child_count = 0;
      EV_Key *child_keys = 0;
      U64 *child_nums = 0;
      if(allow_child_expansions && !child_count && !viz_expand_info.rows_default_expanded && expand_node != 0 && expansion_row_count != 0)
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
            child_nums[idx] = type_expand_rule->num_from_id(type_expand_info.user_data, child->key.child_id);
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
      if(allow_child_expansions && !child_count && (viz_expand_info.rows_default_expanded || (expand_node == 0 && !viz_expand_info.rows_default_expanded)))
      {
        child_count = viz_expand_info.row_count;
        child_keys  = push_array(scratch.arena, EV_Key, child_count);
        child_nums  = push_array(scratch.arena, U64,    child_count);
        for(U64 idx = 0; idx < child_count; idx += 1)
        {
          U64 child_id = type_expand_rule->id_from_num(type_expand_info.user_data, idx+1);
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
        if(viz_expand_info.rows_default_expanded || ev_expansion_from_key(view, child_keys[idx]))
        {
          Rng1U64 child_range = r1u64(split_relative_idx, split_relative_idx+1);
          E_Eval child_eval = {0};
          type_expand_rule->range(arena, type_expand_info.user_data, eval, task_filter, r1u64(split_relative_idx, split_relative_idx+1), &child_eval);
          EV_Key child_key = child_keys[idx];
          BlockTreeBuildTask *task = push_array(scratch.arena, BlockTreeBuildTask, 1);
          SLLQueuePush(first_task, last_task, task);
          task->parent_block       = expansion_block;
          task->eval               = child_eval;
          task->next_expr          = &e_expr_nil;
          task->child_id           = child_key.child_id;
          task->split_relative_idx = split_relative_idx;
          task->default_expanded   = viz_expand_info.rows_default_expanded;
          task->depth              = t->depth+1;
        }
      }
      
      // rjf: if this expr has a sibling, push another task to continue the chain
      if(t->next_expr != &e_expr_nil)
      {
        BlockTreeBuildTask *task = push_array(scratch.arena, BlockTreeBuildTask, 1);
        task->next = t->next;
        t->next = task;
        task->parent_block       = t->parent_block;
        task->eval               = e_eval_from_expr(t->next_expr);
        task->next_expr          = t->next_expr->next;
        task->child_id           = t->child_id + 1;
        task->split_relative_idx = 0;
        task->default_expanded   = t->default_expanded;
        task->force_expanded     = 1;
        task->depth              = t->depth;
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

internal U64
ev_block_id_from_num(EV_Block *block, U64 num)
{
  U64 result = block->type_expand_rule->id_from_num(block->type_expand_info.user_data, num);
  return result;
}

internal U64
ev_block_num_from_id(EV_Block *block, U64 id)
{
  U64 result = block->type_expand_rule->num_from_id(block->type_expand_info.user_data, id);
  return result;
}

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
        if(remainder_range.max >= remainder_range.min)
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
  U64 base_num = 1;
  for(EV_BlockRangeNode *n = block_ranges->first; n != 0; n = n->next)
  {
    U64 range_size = n->v.block->viz_expand_info.single_item ? 1 : dim_1u64(n->v.range);
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
    U64 range_size = n->v.block->viz_expand_info.single_item ? 1 : dim_1u64(n->v.range);
    Rng1U64 global_range = r1u64(base_num, base_num + range_size);
    if(contains_1u64(global_range, num))
    {
      U64 relative_num = (num - base_num) + n->v.range.min + 1;
      U64 child_id = ev_block_id_from_num(n->v.block, relative_num);
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
      U64 relative_num = ev_block_num_from_id(n->v.block, key.child_id);
      Rng1U64 num_range = r1u64(n->v.range.min, n->v.block->viz_expand_info.single_item ? (n->v.range.min+1) : n->v.range.max);
      if(contains_1u64(num_range, relative_num-1))
      {
        result = base_num + (relative_num - 1 - n->v.range.min);
        break;
      }
    }
    base_num += n->v.block->viz_expand_info.single_item ? 1 : dim_1u64(n->v.range);
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
      U64 next_base_num = base_num + (n->v.block->viz_expand_info.single_item ? 1 : dim_1u64(n->v.range));
      if(base_num <= num && num < next_base_num)
      {
        U64 relative_vnum = (n->v.block->viz_expand_info.single_item ? 0 : (num - base_num));
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
        U64 relative_num = (n->v.block->viz_expand_info.single_item ? 0 : (vnum - base_vnum));
        num = base_num + relative_num;
        break;
      }
      base_vnum = next_base_vnum;
      base_num += (n->v.block->viz_expand_info.single_item ? 1 : dim_1u64(n->v.range));
    }
  }
  return num;
}

////////////////////////////////
//~ rjf: Row Building

internal EV_WindowedRowList
ev_windowed_row_list_from_block_range_list(Arena *arena, EV_View *view, EV_BlockRangeList *block_ranges, Rng1U64 vnum_range)
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
      String8 block_filter = n->v.block->filter;
      
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
        if(n->v.block->viz_expand_info.single_item)
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
        B32 is_standalone_row = 0;
        U64 range_exprs_count = dim_1u64(block_relative_range__windowed);
        E_Eval *range_evals = push_array(arena, E_Eval, range_exprs_count);
        for EachIndex(idx, range_exprs_count)
        {
          range_evals[idx] = e_eval_nil;
        }
        if(n->v.block->viz_expand_info.single_item || n->v.block->parent == &ev_nil_block)
        {
          is_standalone_row = 1;
        }
        else
        {
          n->v.block->type_expand_rule->range(arena, n->v.block->type_expand_info.user_data, n->v.block->eval, block_filter, block_relative_range__windowed, range_evals);
        }
        
        // rjf: no expansion operator applied -> push row for block expression; pass through block info
        if(is_standalone_row)
        {
          EV_WindowedRowNode *row_node = push_array(arena, EV_WindowedRowNode, 1);
          SLLQueuePush(rows.first, rows.last, row_node);
          rows.count += 1;
          row_node->visual_size_skipped = num_skipped;
          row_node->visual_size_chopped = num_chopped;
          EV_Row *row = &row_node->row;
          row->block         = n->v.block;
          row->key           = ev_key_make(ev_hash_from_key(row->block->key), 1);
          row->visual_size   = n->v.block->viz_expand_info.single_item ? (n->v.block->row_count - (num_skipped + num_chopped)) : 1;
          row->edit_string   = n->v.block->string;
          row->eval          = n->v.block->eval;
        }
        
        // rjf: expansion operator applied -> call, and add rows for all expressions in the viewable range
        else for EachIndex(idx, range_exprs_count)
        {
          U64 child_num = block_relative_range.min + num_skipped + idx + 1;
          U64 child_id = ev_block_id_from_num(n->v.block, child_num);
          EV_Key row_key = ev_key_make(ev_hash_from_key(n->v.block->key), child_id);
          E_Eval row_eval = range_evals[idx];
          EV_WindowedRowNode *row_node = push_array(arena, EV_WindowedRowNode, 1);
          SLLQueuePush(rows.first, rows.last, row_node);
          rows.count += 1;
          EV_Row *row = &row_node->row;
          row->block                = n->v.block;
          row->key                  = row_key;
          row->visual_size          = 1;
          row->edit_string          = row_eval.string;
          row->eval                 = row_eval;
        }
      }
    }
  }
  return rows;
}

internal EV_Row *
ev_row_from_num(Arena *arena, EV_View *view, EV_BlockRangeList *block_ranges, U64 num)
{
  U64 vidx = ev_vnum_from_num(block_ranges, num);
  EV_WindowedRowList rows = ev_windowed_row_list_from_block_range_list(arena, view, block_ranges, r1u64(vidx, vidx+1));
  EV_Row *result = 0;
  if(rows.first != 0)
  {
    result = &rows.first->row;
  }
  else
  {
    result = push_array(arena, EV_Row, 1);
    result->block = &ev_nil_block;
    result->eval = e_eval_nil;
  }
  return result;
}

internal EV_WindowedRowList
ev_rows_from_num_range(Arena *arena, EV_View *view, EV_BlockRangeList *block_ranges, Rng1U64 num_range)
{
  Rng1U64 vnum_range = r1u64(ev_vnum_from_num(block_ranges, num_range.min), ev_vnum_from_num(block_ranges, num_range.max)+1);
  EV_WindowedRowList rows = ev_windowed_row_list_from_block_range_list(arena, view, block_ranges, vnum_range);
  return rows;
}

internal B32
ev_eval_is_expandable(E_Eval eval)
{
  B32 result = 0;
  E_IRTreeAndType irtree = eval.irtree;
  
  // rjf: determine if lenses force expandability
  if(!result)
  {
    EV_ExpandRule *expand_rule = ev_expand_rule_from_type_key(irtree.type_key);
    if(expand_rule != &ev_nil_expand_rule)
    {
      result = 1;
    }
  }
  
  // rjf: determine if type info force expandability
  if(!result)
  {
    result = ev_type_key_and_mode_is_expandable(irtree.type_key, irtree.mode);
  }
  return result;
}

internal B32
ev_row_is_expandable(EV_Row *row)
{
  B32 result = 0;
  if(!ev_key_match(ev_key_root(), row->block->key))
  {
    result = ev_eval_is_expandable(row->eval);
  }
  return result;
}

internal B32
ev_row_is_editable(EV_Row *row)
{
  B32 result = 0;
  E_IRTreeAndType irtree = row->eval.irtree;
  result = ev_type_key_is_editable(irtree.type_key);
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
ev_string_from_simple_typed_eval(Arena *arena, EV_StringParams *params, E_Eval eval)
{
  String8 result = {0};
  E_TypeKey type_key = e_type_key_unwrap(eval.irtree.type_key, E_TypeUnwrapFlag_AllDecorative & ~E_TypeUnwrapFlag_Enums);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  U64 type_byte_size = e_type_byte_size_from_key(type_key);
  U8 digit_group_separator = 0;
  if(!(params->flags & EV_StringFlag_ReadOnlyDisplayRules))
  {
    digit_group_separator = 0;
  }
  F64 f64 = 0;
  switch(type_kind)
  {
    default:{}break;
    
    case E_TypeKind_Handle:
    {
      result = str8_from_s64(arena, eval.value.s64, params->radix, params->min_digits, digit_group_separator);
    }break;
    
    case E_TypeKind_HResult:
    {
      if(params->flags & EV_StringFlag_ReadOnlyDisplayRules)
      {
        Temp scratch = scratch_begin(&arena, 1);
        U32 hresult_value = (U32)eval.value.u64;
        U32 is_error   = !!(hresult_value & (1ull<<31));
        U32 error_code = (hresult_value);
        U32 facility   = (hresult_value & 0x7ff0000) >> 16;
        String8 value_string = str8_from_s64(scratch.arena, eval.value.u64, params->radix, params->min_digits, digit_group_separator);
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
        result = str8_from_s64(arena, eval.value.u64, params->radix, params->min_digits, digit_group_separator);
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
      String8 char_str = {0};
      if(!(params->flags & EV_StringFlag_DisableChars))
      {
        char_str = ev_string_from_ascii_value(arena, eval.value.s64);
      }
      if(char_str.size != 0)
      {
        if(params->flags & EV_StringFlag_ReadOnlyDisplayRules)
        {
          String8 imm_string = (type_is_unsigned
                                ? str8_from_u64(arena, eval.value.u64, params->radix, params->min_digits, digit_group_separator)
                                : str8_from_s64(arena, eval.value.s64, params->radix, params->min_digits, digit_group_separator));
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
                  ? str8_from_u64(arena, eval.value.u64, params->radix, params->min_digits, digit_group_separator)
                  : str8_from_s64(arena, eval.value.s64, params->radix, params->min_digits, digit_group_separator));
      }
    }break;
    
    case E_TypeKind_S8:
    case E_TypeKind_S16:
    case E_TypeKind_S32:
    case E_TypeKind_S64:
    {
      result = str8_from_s64(arena, eval.value.s64, params->radix, params->min_digits, digit_group_separator);
    }break;
    
    case E_TypeKind_U8:
    case E_TypeKind_U16:
    case E_TypeKind_U32:
    case E_TypeKind_U64:
    {
      result = str8_from_u64(arena, eval.value.u64, params->radix, params->min_digits, digit_group_separator);
    }break;
    
    case E_TypeKind_U128:
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8 upper64 = str8_from_u64(scratch.arena, eval.value.u128.u64[0], params->radix, params->min_digits, digit_group_separator);
      String8 lower64 = str8_from_u64(scratch.arena, eval.value.u128.u64[1], params->radix, params->min_digits, digit_group_separator);
      result = push_str8f(arena, "%S:%S", upper64, lower64);
      scratch_end(scratch);
    }break;
    
    case E_TypeKind_F32:{f64 = (F64)eval.value.f32;}goto f64_path;
    case E_TypeKind_F64:{f64 = eval.value.f64;}goto f64_path;
    f64_path:;
    {
      result = push_str8f(arena, "%.*f", params->min_digits ? params->min_digits : 16, f64);
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
      E_Type *type = e_type_from_key(type_key);
      String8 constant_name = {0};
      for(U64 val_idx = 0; val_idx < type->count; val_idx += 1)
      {
        if(eval.value.u64 == type->enum_vals[val_idx].val)
        {
          constant_name = type->enum_vals[val_idx].name;
          break;
        }
      }
      String8 numeric_value_string = str8_from_u64(scratch.arena, eval.value.u64, params->radix, params->min_digits, digit_group_separator);
      if(params->flags & EV_StringFlag_ReadOnlyDisplayRules)
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

//- rjf: tree stringification iterator

internal EV_StringIter *
ev_string_iter_begin(Arena *arena, E_Eval eval, EV_StringParams *params)
{
  EV_StringIter *it = push_array(arena, EV_StringIter, 1);
  it->top_task = push_array(arena, EV_StringIterTask, 1);
  it->top_task->eval = eval;
  MemoryCopyStruct(&it->top_task->params, params);
  return it;
}

internal B32
ev_string_iter_next(Arena *arena, EV_StringIter *it, String8 *out_string)
{
  B32 result = 0;
  
  //- rjf: make progress on top task
  MemoryZeroStruct(out_string);
  B32 need_pop = 1;
  B32 need_new_task = 0;
  EV_StringIterTask new_task = {0};
  S32 top_task_depth = 0;
  if(it->top_task != 0)
  {
    result = 1;
    
    //- rjf: unpack task
    U64 task_idx = it->top_task->idx;
    S32 depth = top_task_depth = it->top_task->depth;
    EV_StringParams *params = &it->top_task->params;
    E_Eval eval = it->top_task->eval;
    E_TypeKey type_key = eval.irtree.type_key;
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
    String8 expansion_opener_symbol = str8_lit("{");
    String8 expansion_closer_symbol = str8_lit("}");
    
    //- rjf: type evaluations -> display type string
    if(eval.irtree.mode == E_Mode_Null && !e_type_key_match(e_type_key_zero(), eval.irtree.type_key))
    {
      *out_string = e_type_string_from_key(arena, type_key);
    }
    
    //- rjf: non-type evaluations
    else switch(type_kind)
    {
      //////////////////////////
      //- rjf: default - leaf cases
      //
      default:
      {
        E_Eval value_eval = e_value_eval_from_eval(eval);
        *out_string = ev_string_from_simple_typed_eval(arena, params, value_eval);
      }break;
      
      //////////////////////////
      //- rjf: lenses
      //
      case E_TypeKind_Lens:
      {
        if(it->top_task->redirect_to_sets_and_structs)
        {
          E_Type *type = e_type_from_key(type_key);
          if(type->flags & E_TypeFlag_ArrayLikeExpansion)
          {
            expansion_opener_symbol = str8_lit("[");
            expansion_closer_symbol = str8_lit("]");
          }
          goto arrays_and_sets_and_structs;
        }
        E_Type *type = e_type_from_key(type_key);
        E_TypeKind element_type_kind = e_type_kind_from_key(e_type_key_unwrap(type->direct_type_key, E_TypeUnwrapFlag_All));
        B32 lens_applied = 1;
        EV_StringParams lens_params = *params;
        if(0){}
        else if(str8_match(type->name, str8_lit("bin"), 0)) { lens_params.radix = 2; }
        else if(str8_match(type->name, str8_lit("oct"), 0)) { lens_params.radix = 8; }
        else if(str8_match(type->name, str8_lit("dec"), 0)) { lens_params.radix = 10; }
        else if(str8_match(type->name, str8_lit("hex"), 0)) { lens_params.radix = 16; }
        else if(str8_match(type->name, str8_lit("digits"), 0) && type->count >= 1)
        {
          E_ParentKey(eval.key)
          {
            E_Value value = e_value_from_expr(type->args[0]);
            lens_params.min_digits = value.u64;
          }
        }
        else if(str8_match(type->name, str8_lit("no_string"), 0))
        {
          lens_params.flags |= EV_StringFlag_DisableStrings;
        }
        else if(str8_match(type->name, str8_lit("no_char"), 0))
        {
          lens_params.flags |= EV_StringFlag_DisableChars;
        }
        else if(str8_match(type->name, str8_lit("no_addr"), 0))
        {
          lens_params.flags |= EV_StringFlag_DisableAddresses;
        }
        else if(str8_match(type->name, str8_lit("array"), 0) &&
                type->count >= 1 &&
                (((E_TypeKind_Char8 <= element_type_kind && element_type_kind <= E_TypeKind_UChar32) ||
                  element_type_kind == E_TypeKind_S8 ||
                  element_type_kind == E_TypeKind_U8)))
        {
          E_ParentKey(eval.key)
          {
            lens_params.limit_strings = 1;
            lens_params.limit_strings_size = e_value_from_expr(type->args[0]).u64;
          }
        }
        else
        {
          lens_applied = 0;
        }
        if(lens_applied)
        {
          need_new_task = 1;
          need_pop = 1;
          new_task.params = lens_params;
          new_task.eval = eval;
          new_task.eval.irtree.type_key = e_type_key_direct(eval.irtree.type_key);
        }
        else if(type->expand.info != 0)
        {
          need_new_task = 1;
          need_pop = 1;
          new_task.params = *params;
          new_task.eval = eval;
          new_task.redirect_to_sets_and_structs = 1;
        }
        else
        {
          need_new_task = 1;
          need_pop = 1;
          new_task.params = lens_params;
          new_task.eval = eval;
          new_task.eval.irtree.type_key = e_type_key_direct(eval.irtree.type_key);
#if 0 // NOTE(rjf): will explicitly visualize lenses in value strings. does not seem useful for now?
          switch(task_idx)
          {
            default:{}break;
            
            // rjf: step 0 -> generate lens description, then descend to same evaluation w/ direct type
            case 0:
            {
              Temp scratch = scratch_begin(&arena, 1);
              String8List strings = {0};
              {
                str8_list_pushf(scratch.arena, &strings, "%S(", type->name);
                for EachIndex(idx, type->count)
                {
                  String8 string = e_string_from_expr(scratch.arena, type->args[idx]);
                  str8_list_push(scratch.arena, &strings, string);
                  if(idx+1 < type->count)
                  {
                    str8_list_pushf(scratch.arena, &strings, ", ");
                  }
                }
                str8_list_pushf(scratch.arena, &strings, ") <- (");
              }
              *out_string = str8_list_join(arena, &strings, 0);
              need_new_task = 1;
              need_pop = 0;
              new_task.params = *params;
              new_task.eval = eval;
              new_task.eval.irtree.type_key = e_type_key_direct(eval.irtree.type_key);
              scratch_end(scratch);
            }break;
            
            // rjf: step 1 -> close
            case 1:
            {
              *out_string = str8_lit(")");
            }break;
          }
#endif
        }
      }break;
      
      //////////////////////////
      //- rjf: meta-expression tags
      //
      case E_TypeKind_MetaExpr:
      {
        if(params->flags & EV_StringFlag_ReadOnlyDisplayRules)
        {
          switch(task_idx)
          {
            default:{}break;
            case 0:
            {
              E_Type *type = e_type_from_key(type_key);
              *out_string = push_str8f(arena, "%S (", type->name);
              need_pop = 0;
              need_new_task = 1;
              new_task.params = *params;
              new_task.eval = eval;
              new_task.eval.irtree.type_key = e_type_key_direct(eval.irtree.type_key);
            }break;
            case 1:
            {
              *out_string = str8_lit(")");
            }break;
          }
        }
        else
        {
          E_Type *type = e_type_from_key(type_key);
          *out_string = type->name;
        }
      }break;
      
      //////////////////////////
      //- rjf: modifiers / no-ops
      //
      case E_TypeKind_Modifier:
      case E_TypeKind_MetaDescription:
      case E_TypeKind_MetaDisplayName:
      {
        need_pop = 1;
        need_new_task = 1;
        new_task.params = *params;
        new_task.eval = eval;
        new_task.eval.irtree.type_key = e_type_key_direct(eval.irtree.type_key);
      }break;
      
      //////////////////////////
      //- rjf: pointers
      //
      case E_TypeKind_Function:
      case E_TypeKind_Ptr:
      case E_TypeKind_LRef:
      case E_TypeKind_RRef:
      case E_TypeKind_Array:
      {
        if(type_kind == E_TypeKind_Array && it->top_task->redirect_to_sets_and_structs)
        {
          expansion_opener_symbol = str8_lit("[");
          expansion_closer_symbol = str8_lit("]");
          goto arrays_and_sets_and_structs;
        } 
        typedef struct EV_StringPtrData EV_StringPtrData;
        struct EV_StringPtrData
        {
          E_Eval value_eval;
          E_Type *type;
          E_Type *direct_type;
          B32 ptee_has_content;
          B32 ptee_has_string;
          B32 did_prefix_content;
          B32 did_redirect;
        };
        EV_StringPtrData *ptr_data = it->top_task->user_data;
        if(ptr_data == 0)
        {
          ptr_data = it->top_task->user_data = push_array(arena, EV_StringPtrData, 1);
          ptr_data->value_eval = e_value_eval_from_eval(eval);
          ptr_data->type = e_type_from_key(type_key);
          ptr_data->direct_type = e_type_from_key(e_type_key_unwrap(type_key, E_TypeUnwrapFlag_All));
          ptr_data->ptee_has_content = (ptr_data->value_eval.value.u64 != 0 && ptr_data->direct_type->kind != E_TypeKind_Null && ptr_data->direct_type->kind != E_TypeKind_Void);
          ptr_data->ptee_has_string  = ((E_TypeKind_Char8 <= ptr_data->direct_type->kind && ptr_data->direct_type->kind <= E_TypeKind_UChar32) ||
                                        ptr_data->direct_type->kind == E_TypeKind_S8 ||
                                        ptr_data->direct_type->kind == E_TypeKind_U8);
        }
        if(ptr_data->did_redirect)
        {
          need_pop = 1;
        }
        else switch(task_idx)
        {
          default:{}break;
          
          //- rjf: step 0 -> try "prefix content", which we want to print before the pointer value,
          // like strings or symbol names
          case 0:
          {
            // rjf: try strings
            if(!(ptr_data->type->flags & E_TypeFlag_IsNotText) &&
               !ptr_data->did_prefix_content && ptr_data->ptee_has_string &&
               !(params->flags & EV_StringFlag_DisableStrings) &&
               (type_kind == E_TypeKind_Array ||
                params->flags & EV_StringFlag_ReadOnlyDisplayRules))
            {
              Temp scratch = scratch_begin(&arena, 1);
              
              // rjf: read string data
#define EV_STRING_ITER_STRING_BUFFER_CAPACITY 4096
              U64 string_buffer_size = EV_STRING_ITER_STRING_BUFFER_CAPACITY;
              U8 *string_buffer = push_array(scratch.arena, U8, string_buffer_size);
              if(type_kind == E_TypeKind_Array && eval.irtree.mode == E_Mode_Value)
              {
                StaticAssert(sizeof(eval.value.u512.u8) <= EV_STRING_ITER_STRING_BUFFER_CAPACITY, ev_string_iter_value_string_buffer_size_check);
                MemoryCopy(string_buffer, eval.value.u512.u8, sizeof(eval.value.u512.u8));
              }
              else
              {
                U64 string_memory_addr = ptr_data->value_eval.value.u64;
                for(U64 try_size = string_buffer_size; try_size >= 16; try_size /= 2)
                {
                  B32 read_good = e_space_read(eval.space, string_buffer, r1u64(string_memory_addr, string_memory_addr+try_size));
                  if(read_good)
                  {
                    break;
                  }
                }
                string_buffer[string_buffer_size-1] = 0;
              }
              
              // rjf: check element size - if non-U8, assume UTF-16 or UTF-32 based on type, and convert
              U64 element_size = ptr_data->direct_type->byte_size;
              String8 string = {0};
              switch(element_size)
              {
                default:{string = str8_cstring((char *)string_buffer);}break;
                case 2: {string = str8_from_16(scratch.arena, str16_cstring((U16 *)string_buffer));}break;
                case 4: {string = str8_from_32(scratch.arena, str32_cstring((U32 *)string_buffer));}break;
              }
              
              // rjf: apply string size limitation
              if(params->limit_strings)
              {
                string = str8_prefix(string, params->limit_strings_size);
              }
              
              // rjf: escape and quote
              B32 string__is_escaped_and_quoted = (!(params->flags & EV_StringFlag_DisableStringQuotes) || depth > 0);
              String8 string__escaped_and_quoted = string;
              if(string__is_escaped_and_quoted)
              {
                String8 string_escaped = ev_escaped_from_raw_string(scratch.arena, string);
                string__escaped_and_quoted = push_str8f(scratch.arena, "\"%S\"", string_escaped);
              }
              
              // rjf: report
              *out_string = push_str8_copy(arena, string__escaped_and_quoted);
              ptr_data->did_prefix_content = 1;
              
              scratch_end(scratch);
            }
            
            // rjf: try symbols
            if(!ptr_data->did_prefix_content)
            {
              U64 vaddr = ptr_data->value_eval.value.u64;
              E_Module *module = &e_module_nil;
              U32 module_idx = 0;
              for EachIndex(idx, e_base_ctx->modules_count)
              {
                if(contains_1u64(e_base_ctx->modules[idx].vaddr_range, vaddr))
                {
                  module = &e_base_ctx->modules[idx];
                  module_idx = (U32)idx;
                  break;
                }
              }
              RDI_Parsed *rdi = module->rdi;
              U64 voff = vaddr - module->vaddr_range.min;
              B32 good_symbol_match = 0;
              
              // NOTE(rjf): read-only -> generate non-parseable things, like type-info / inlines
              if(params->flags & EV_StringFlag_ReadOnlyDisplayRules)
              {
                // rjf: voff -> scope
                U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
                
                // rjf: scope -> # of max possible inline depth
                U64 inline_site_count = 0;
                for(U64 s_idx = scope_idx, s_idx_next = 0; s_idx != 0; s_idx = s_idx_next)
                {
                  RDI_Scope *s = rdi_element_from_name_idx(rdi, Scopes, s_idx);
                  s_idx_next = s->parent_scope_idx;
                  if(s->inline_site_idx != 0)
                  {
                    inline_site_count += 1;
                  }
                  else
                  {
                    break;
                  }
                }
                
                // rjf: depth in [1, max]? -> form name from inline site
                if(0 < ptr_data->type->depth && ptr_data->type->depth <= inline_site_count)
                {
                  RDI_InlineSite *inline_site = 0;
                  U64 s_inline_depth = inline_site_count;
                  for(U64 s_idx = scope_idx, s_idx_next = 0; s_idx != 0; s_idx = s_idx_next)
                  {
                    RDI_Scope *s = rdi_element_from_name_idx(rdi, Scopes, s_idx);
                    s_idx_next = s->parent_scope_idx;
                    if(s_inline_depth == ptr_data->type->depth)
                    {
                      inline_site = rdi_element_from_name_idx(rdi, InlineSites, s->inline_site_idx);
                      break;
                    }
                    s_inline_depth -= 1;
                    if(s_inline_depth == 0)
                    {
                      break;
                    }
                  }
                  if(inline_site != 0)
                  {
                    E_TypeKey type = e_type_key_ext(E_TypeKind_Function, inline_site->type_idx, module_idx);
                    String8 name = {0};
                    name.str = rdi_string_from_idx(rdi, inline_site->name_string_idx, &name.size);
                    if(inline_site->type_idx != 0)
                    {
                      Temp scratch = scratch_begin(&arena, 1);
                      String8List list = {0};
                      str8_list_pushf(scratch.arena, &list, "[inlined] ");
                      e_type_lhs_string_from_key(scratch.arena, type, &list, 0, 0);
                      str8_list_push(scratch.arena, &list, name);
                      e_type_rhs_string_from_key(scratch.arena, type, &list, 0);
                      *out_string = str8_list_join(arena, &list, 0);
                      scratch_end(scratch);
                    }
                    else
                    {
                      *out_string = push_str8_copy(arena, name);
                    }
                    good_symbol_match = (name.size != 0);
                  }
                }
                
                // rjf: depth == 0 or depth >= max? -> form name from scope procedure
                else
                {
                  Temp scratch = scratch_begin(&arena, 1);
                  RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
                  U64 proc_idx = scope->proc_idx;
                  RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, proc_idx);
                  E_TypeKey type = e_type_key_ext(E_TypeKind_Function, procedure->type_idx, module_idx);
                  String8 name = {0};
                  name.str = rdi_string_from_idx(rdi, procedure->name_string_idx, &name.size);
                  if(procedure->type_idx != 0)
                  {
                    String8List list = {0};
                    e_type_lhs_string_from_key(scratch.arena, type, &list, 0, 0);
                    str8_list_push(scratch.arena, &list, name);
                    e_type_rhs_string_from_key(scratch.arena, type, &list, 0);
                    *out_string = str8_list_join(arena, &list, 0);
                  }
                  else
                  {
                    *out_string = push_str8_copy(arena, name);
                  }
                  
                  good_symbol_match = (out_string->size != 0);
                  scratch_end(scratch);
                }
                
                // rjf: if we have a function type, but we did not generate any name, then just put a ???
                if(out_string->size == 0 && e_type_kind_from_key(ptr_data->type->direct_type_key) == E_TypeKind_Function)
                {
                  *out_string = str8_lit("???");
                  good_symbol_match = 1;
                }
              }
              
              // NOTE(rjf): non-read-only -> only generate thing which can be parsed, so just procedure name
              else
              {
                // rjf: voff -> scope
                U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
                RDI_Scope *scope = rdi_scope_from_voff(rdi, voff);
                
                // rjf: scope -> procedure / string
                RDI_Procedure *procedure = rdi_procedure_from_scope(rdi, scope);
                String8 procedure_name = {0};
                procedure_name.str = rdi_name_from_procedure(rdi, procedure, &procedure_name.size);
                
                *out_string = procedure_name;
                good_symbol_match = (procedure_name.size != 0);
              }
              
              ptr_data->did_prefix_content = good_symbol_match;
            }
            
            // rjf: if this is an array, and we do not have a prefix, then we need to
            // generate a new task which redirects array types -> sets and structs.
            if(type_kind == E_TypeKind_Array && !ptr_data->did_prefix_content)
            {
              need_new_task = 1;
              need_pop = 0;
              new_task.params = *params;
              new_task.eval = eval;
              new_task.redirect_to_sets_and_structs = 1;
              ptr_data->did_redirect = 1;
            }
            
            // rjf: if this is an array, and we *did* prefix content, then we are
            // just done.
            else if(type_kind == E_TypeKind_Array && ptr_data->did_prefix_content)
            {
              // NOTE(rjf): no-op, task is done.
            }
            
            // rjf: otherwise, keep going on this task
            else
            {
              need_pop = 0;
            }
          }break;
          
          //- rjf: step 1 -> do pointer value + descend if needed
          case 1:
          {
            Temp scratch = scratch_begin(&arena, 1);
            String8 ptr_value_string = str8_from_u64(scratch.arena, ptr_data->value_eval.value.u64, 16, 0, 0);
            //
            // NOTE(rjf): currently, we are not using the string-generation radix parameter when
            // generating a pointer value - it is weird to want to change pointer value visualization
            // to anything other than hex, so it is just not supported right now...
            //
            
            // rjf: [read only] if we did prefix content, do a parenthesized pointer value
            if(!(params->flags & EV_StringFlag_DisableAddresses) && params->flags & EV_StringFlag_ReadOnlyDisplayRules && ptr_data->did_prefix_content)
            {
              *out_string = push_str8f(arena, " (%S)", ptr_value_string);
            }
            
            // rjf: [read only] if we did *not* do any prefix content, but we have content,
            // do "<pointer value> -> " then descend
            else if(params->flags & EV_StringFlag_ReadOnlyDisplayRules && !ptr_data->did_prefix_content && ptr_data->ptee_has_content)
            {
              if(!(params->flags & EV_StringFlag_DisableAddresses))
              {
                *out_string = push_str8f(arena, "%S -> ", ptr_value_string);
              }
              
              // rjf: single-length pointers -> just gen new task for deref'd expr
              if(ptr_data->type->count == 1)
              {
                E_Eval deref_eval = e_eval_wrapf(eval, "*$");
                need_new_task = 1;
                need_pop = 0;
                new_task.params = *params;
                new_task.eval = deref_eval;
              }
              
              // rjf: multi-length pointers -> expand like an array (try to dedup with array case)
              else
              {
                // TODO(rjf)
              }
            }
            
            // rjf: [writeable, catchall] if we did *not* do any prefix content, do "<pointer value>"
            else if(!ptr_data->did_prefix_content)
            {
              *out_string = push_str8_copy(arena, ptr_value_string);
            }
            
            scratch_end(scratch);
          }break;
        }
      }break;
      
      //////////////////////////
      //- rjf: non-string-arrays/structs, sets
      //
      case E_TypeKind_Struct:
      case E_TypeKind_Union:
      case E_TypeKind_Class:
      case E_TypeKind_IncompleteStruct:
      case E_TypeKind_IncompleteUnion:
      case E_TypeKind_IncompleteClass:
      case E_TypeKind_Set:
      arrays_and_sets_and_structs:
      {
        typedef struct EV_ExpandedTypeData EV_ExpandedTypeData;
        struct EV_ExpandedTypeData
        {
          E_Type *type;
          E_TypeExpandRule *expand_rule;
          E_TypeExpandInfo expand_info;
        };
        EV_ExpandedTypeData *expand_data = (EV_ExpandedTypeData *)it->top_task->user_data;
        if(expand_data == 0)
        {
          expand_data = it->top_task->user_data = push_array(arena, EV_ExpandedTypeData, 1);
          expand_data->type = e_type_from_key(type_key);
        }
        switch(task_idx)
        {
          //- rjf: step 0 -> generate opener symbol
          case 0:
          {
            if(expand_data->type->flags & E_TypeFlag_StubSingleLineExpansion)
            {
              *out_string = push_str8f(arena, "%S...%S", expansion_opener_symbol, expansion_closer_symbol);
            }
            else
            {
              need_pop = 0;
              expand_data->expand_rule = e_expand_rule_from_type_key(type_key);
              expand_data->expand_info = expand_data->expand_rule->info(arena, eval, params->filter);
              *out_string = expansion_opener_symbol;
            }
          }break;
          
          default:
          //- rjf: last step -> generate closer symbol
          if(task_idx == expand_data->expand_info.expr_count+1)
          {
            *out_string = expansion_closer_symbol;
          }
          
          //- rjf: middle step -> generate new task for next thing in expansion
          else
          {
            E_Eval next_eval = e_eval_nil;
            expand_data->expand_rule->range(arena, expand_data->expand_info.user_data, eval, params->filter, r1u64(task_idx-1, task_idx), &next_eval);
            if(next_eval.expr != &e_expr_nil)
            {
              need_new_task = 1;
              need_pop = 0;
              new_task.params = *params;
              new_task.eval = next_eval;
              if(task_idx > 1)
              {
                *out_string = str8_lit(", ");
              }
            }
            else
            {
              need_pop = 0;
            }
          }break;
        }
      }break;
    }
  }
  
  //- rjf: bump task counter
  if(it->top_task != 0)
  {
    it->top_task->idx += 1;
  }
  
  //- rjf: if result is good, and we want to pop? -> pop
  if(result && need_pop)
  {
    EV_StringIterTask *task = it->top_task;
    SLLStackPop(it->top_task);
    SLLStackPush(it->free_task, task);
  }
  
  //- rjf: if result is good, and we have a new task? -> push
  if(result && need_new_task)
  {
    EV_StringIterTask *new_t = it->free_task;
    if(new_t != 0)
    {
      SLLStackPop(it->free_task);
    }
    else
    {
      new_t = push_array(arena, EV_StringIterTask, 1);
    }
    MemoryCopyStruct(new_t, &new_task);
    new_t->depth = top_task_depth + 1*(!need_pop);
    SLLStackPush(it->top_task, new_t);
    new_t->idx = 0;
  }
  
  return result;
}
