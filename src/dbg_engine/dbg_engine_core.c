// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef MARKUP_LAYER_COLOR
#define MARKUP_LAYER_COLOR 0.70f, 0.50f, 0.25f

////////////////////////////////
//~ rjf: Generated Code

#include "dbg_engine/generated/dbg_engine.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
d_hash_from_seed_string(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
d_hash_from_string(String8 string)
{
  return d_hash_from_seed_string(5381, string);
}

internal U64
d_hash_from_seed_string__case_insensitive(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + char_to_lower(string.str[i]);
  }
  return result;
}

internal U64
d_hash_from_string__case_insensitive(String8 string)
{
  return d_hash_from_seed_string__case_insensitive(5381, string);
}

////////////////////////////////
//~ rjf: Handles

internal D_Handle
d_handle_zero(void)
{
  D_Handle result = {0};
  return result;
}

internal B32
d_handle_match(D_Handle a, D_Handle b)
{
  return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1]);
}

internal void
d_handle_list_push_node(D_HandleList *list, D_HandleNode *node)
{
  DLLPushBack(list->first, list->last, node);
  list->count += 1;
}

internal void
d_handle_list_push(Arena *arena, D_HandleList *list, D_Handle handle)
{
  D_HandleNode *n = push_array(arena, D_HandleNode, 1);
  n->handle = handle;
  d_handle_list_push_node(list, n);
}

internal D_HandleList
d_handle_list_copy(Arena *arena, D_HandleList list)
{
  D_HandleList result = {0};
  for(D_HandleNode *n = list.first; n != 0; n = n->next)
  {
    d_handle_list_push(arena, &result, n->handle);
  }
  return result;
}

////////////////////////////////
//~ rjf: Registers Type Pure Functions

internal void
d_regs_copy_contents(Arena *arena, D_Regs *dst, D_Regs *src)
{
  MemoryCopyStruct(dst, src);
  dst->entity_list = d_handle_list_copy(arena, src->entity_list);
  dst->file_path   = push_str8_copy(arena, src->file_path);
  dst->lines       = d_line_list_copy(arena, &src->lines);
  dst->dbgi_key    = di_key_copy(arena, &src->dbgi_key);
  dst->string      = push_str8_copy(arena, src->string);
  dst->params_tree = md_tree_copy(arena, src->params_tree);
  if(dst->entity_list.count == 0 && !d_handle_match(d_handle_zero(), dst->entity))
  {
    d_handle_list_push(arena, &dst->entity_list, dst->entity);
  }
}

internal D_Regs *
d_regs_copy(Arena *arena, D_Regs *src)
{
  D_Regs *dst = push_array(arena, D_Regs, 1);
  d_regs_copy_contents(arena, dst, src);
  return dst;
}

////////////////////////////////
//~ rjf: Config Type Functions

internal void
d_cfg_table_push_unparsed_string(Arena *arena, D_CfgTable *table, String8 string, D_CfgSrc source)
{
  if(table->slot_count == 0)
  {
    table->slot_count = 64;
    table->slots = push_array(arena, D_CfgSlot, table->slot_count);
  }
  MD_TokenizeResult tokenize = md_tokenize_from_text(arena, string);
  MD_ParseResult parse = md_parse_from_text_tokens(arena, str8_lit(""), string, tokenize.tokens);
  for(MD_EachNode(tln, parse.root->first)) if(tln->string.size != 0)
  {
    // rjf: map string -> hash*slot
    String8 string = str8(tln->string.str, tln->string.size);
    U64 hash = d_hash_from_string__case_insensitive(string);
    U64 slot_idx = hash % table->slot_count;
    D_CfgSlot *slot = &table->slots[slot_idx];
    
    // rjf: find existing value for this string
    D_CfgVal *val = 0;
    for(D_CfgVal *v = slot->first; v != 0; v = v->hash_next)
    {
      if(str8_match(v->string, string, StringMatchFlag_CaseInsensitive))
      {
        val = v;
        break;
      }
    }
    
    // rjf: create new value if needed
    if(val == 0)
    {
      val = push_array(arena, D_CfgVal, 1);
      val->string = push_str8_copy(arena, string);
      val->insertion_stamp = table->insertion_stamp_counter;
      SLLStackPush_N(slot->first, val, hash_next);
      SLLQueuePush_N(table->first_val, table->last_val, val, linear_next);
      table->insertion_stamp_counter += 1;
    }
    
    // rjf: create new node within this value
    D_CfgTree *tree = push_array(arena, D_CfgTree, 1);
    SLLQueuePush_NZ(&d_nil_cfg_tree, val->first, val->last, tree, next);
    tree->source = source;
    tree->root   = md_tree_copy(arena, tln);
  }
}

internal D_CfgTable
d_cfg_table_from_inheritance(Arena *arena, D_CfgTable *src)
{
  D_CfgTable dst_ = {0};
  D_CfgTable *dst = &dst_;
  {
    dst->slot_count = src->slot_count;
    dst->slots = push_array(arena, D_CfgSlot, dst->slot_count);
  }
  for(D_CfgVal *src_val = src->first_val; src_val != 0 && src_val != &d_nil_cfg_val; src_val = src_val->linear_next)
  {
    D_ViewRuleSpec *spec = d_view_rule_spec_from_string(src_val->string);
    if(spec->info.flags & D_ViewRuleSpecInfoFlag_Inherited)
    {
      U64 hash = d_hash_from_string(spec->info.string);
      U64 dst_slot_idx = hash%dst->slot_count;
      D_CfgSlot *dst_slot = &dst->slots[dst_slot_idx];
      D_CfgVal *dst_val = push_array(arena, D_CfgVal, 1);
      dst_val->first = src_val->first;
      dst_val->last = src_val->last;
      dst_val->string = src_val->string;
      dst_val->insertion_stamp = dst->insertion_stamp_counter;
      SLLStackPush_N(dst_slot->first, dst_val, hash_next);
      dst->insertion_stamp_counter += 1;
    }
  }
  return dst_;
}

internal D_CfgVal *
d_cfg_val_from_string(D_CfgTable *table, String8 string)
{
  D_CfgVal *result = &d_nil_cfg_val;
  if(table->slot_count != 0)
  {
    U64 hash = d_hash_from_string__case_insensitive(string);
    U64 slot_idx = hash % table->slot_count;
    D_CfgSlot *slot = &table->slots[slot_idx];
    for(D_CfgVal *val = slot->first; val != 0; val = val->hash_next)
    {
      if(str8_match(val->string, string, StringMatchFlag_CaseInsensitive))
      {
        result = val;
        break;
      }
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Debug Info Extraction Type Pure Functions

internal D_LineList
d_line_list_copy(Arena *arena, D_LineList *list)
{
  D_LineList dst = {0};
  for(D_LineNode *src_n = list->first; src_n != 0; src_n = src_n->next)
  {
    D_LineNode *dst_n = push_array(arena, D_LineNode, 1);
    MemoryCopyStruct(dst_n, src_n);
    dst_n->v.dbgi_key = di_key_copy(arena, &src_n->v.dbgi_key);
    SLLQueuePush(dst.first, dst.last, dst_n);
    dst.count += 1;
  }
  return dst;
}

////////////////////////////////
//~ rjf: Command Type Pure Functions

//- rjf: specs

internal B32
d_cmd_spec_is_nil(D_CmdSpec *spec)
{
  return (spec == 0 || spec == &d_nil_cmd_spec);
}

internal void
d_cmd_spec_list_push(Arena *arena, D_CmdSpecList *list, D_CmdSpec *spec)
{
  D_CmdSpecNode *n = push_array(arena, D_CmdSpecNode, 1);
  n->spec = spec;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

//- rjf: string -> command parsing

internal String8
d_cmd_name_part_from_string(String8 string)
{
  String8 result = string;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    if(idx == string.size || char_is_space(string.str[idx]))
    {
      result = str8_prefix(string, idx);
      break;
    }
  }
  return result;
}

internal String8
d_cmd_arg_part_from_string(String8 string)
{
  String8 result = str8_lit("");
  B32 found_space = 0;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    if(found_space && (idx == string.size || !char_is_space(string.str[idx])))
    {
      result = str8_skip(string, idx);
      break;
    }
    else if(!found_space && (idx == string.size || char_is_space(string.str[idx])))
    {
      found_space = 1;
    }
  }
  return result;
}

//- rjf: command parameter bundles

internal D_CmdParams
d_cmd_params_zero(void)
{
  D_CmdParams p = {0};
  return p;
}

internal String8
d_cmd_params_apply_spec_query(Arena *arena, D_CmdParams *params, D_CmdSpec *spec, String8 query)
{
  String8 error = {0};
  switch(spec->info.query.slot)
  {
    default:
    case D_CmdParamSlot_String:
    {
      params->string = push_str8_copy(arena, query);
    }break;
    case D_CmdParamSlot_FilePath:
    {
      String8TxtPtPair pair = str8_txt_pt_pair_from_string(query);
      params->file_path = push_str8_copy(arena, pair.string);
      params->text_point = pair.pt;
    }break;
    case D_CmdParamSlot_TextPoint:
    {
      U64 v = 0;
      if(try_u64_from_str8_c_rules(query, &v))
      {
        params->text_point.column = 1;
        params->text_point.line = v;
      }
      else
      {
        error = str8_lit("Couldn't interpret as a line number.");
      }
    }break;
    case D_CmdParamSlot_VirtualAddr: goto use_numeric_eval;
    case D_CmdParamSlot_VirtualOff: goto use_numeric_eval;
    case D_CmdParamSlot_Index: goto use_numeric_eval;
    case D_CmdParamSlot_ID: goto use_numeric_eval;
    use_numeric_eval:
    {
      Temp scratch = scratch_begin(&arena, 1);
      E_Eval eval = e_eval_from_string(scratch.arena, query);
      if(eval.msgs.max_kind == E_MsgKind_Null)
      {
        E_TypeKind eval_type_kind = e_type_kind_from_key(e_type_unwrap(eval.type_key));
        if(eval_type_kind == E_TypeKind_Ptr ||
           eval_type_kind == E_TypeKind_LRef ||
           eval_type_kind == E_TypeKind_RRef)
        {
          eval = e_value_eval_from_eval(eval);
        }
        U64 u64 = eval.value.u64;
        switch(spec->info.query.slot)
        {
          default:{}break;
          case D_CmdParamSlot_VirtualAddr:
          {
            params->vaddr = u64;
          }break;
          case D_CmdParamSlot_VirtualOff:
          {
            params->voff = u64;
          }break;
          case D_CmdParamSlot_Index:
          {
            params->index = u64;
          }break;
          case D_CmdParamSlot_UnwindIndex:
          {
            params->unwind_index = u64;
          }break;
          case D_CmdParamSlot_InlineDepth:
          {
            params->inline_depth = u64;
          }break;
          case D_CmdParamSlot_ID:
          {
            params->id = u64;
          }break;
        }
      }
      else
      {
        error = push_str8f(scratch.arena, "Couldn't evaluate \"%S\" as an address.", query);
      }
      scratch_end(scratch);
    }break;
  }
  return error;
}

//- rjf: command lists

internal void
d_cmd_list_push(Arena *arena, D_CmdList *cmds, D_CmdParams *params, D_CmdSpec *spec)
{
  D_CmdNode *n = push_array(arena, D_CmdNode, 1);
  n->cmd.spec = spec;
  n->cmd.params = df_cmd_params_copy(arena, params);
  DLLPushBack(cmds->first, cmds->last, n);
  cmds->count += 1;
}

//- rjf: string -> core layer command kind

internal D_CmdKind
d_cmd_kind_from_string(String8 string)
{
  D_CmdKind result = D_CmdKind_Null;
  for(U64 idx = 0; idx < ArrayCount(d_core_cmd_kind_spec_info_table); idx += 1)
  {
    if(str8_match(string, d_core_cmd_kind_spec_info_table[idx].string, StringMatchFlag_CaseInsensitive))
    {
      result = (D_CmdKind)idx;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Entity Functions

//- rjf: nil

internal B32
d_entity_is_nil(D_Entity *entity)
{
  return (entity == 0 || entity == &d_nil_entity);
}

//- rjf: handle <-> entity conversions

internal U64
d_index_from_entity(D_Entity *entity)
{
  return (U64)(entity - d_state->entities_base);
}

internal D_Handle
d_handle_from_entity(D_Entity *entity)
{
  D_Handle handle = d_handle_zero();
  if(!d_entity_is_nil(entity))
  {
    handle.u64[0] = d_index_from_entity(entity);
    handle.u64[1] = entity->gen;
  }
  return handle;
}

internal D_Entity *
d_entity_from_handle(D_Handle handle)
{
  D_Entity *result = d_state->entities_base + handle.u64[0];
  if(handle.u64[0] >= d_state->entities_count || result->gen != handle.u64[1])
  {
    result = &d_nil_entity;
  }
  return result;
}

internal D_EntityList
d_entity_list_from_handle_list(Arena *arena, D_HandleList handles)
{
  D_EntityList result = {0};
  for(D_HandleNode *n = handles.first; n != 0; n = n->next)
  {
    D_Entity *entity = d_entity_from_handle(n->handle);
    if(!d_entity_is_nil(entity))
    {
      d_entity_list_push(arena, &result, entity);
    }
  }
  return result;
}

internal D_HandleList
d_handle_list_from_entity_list(Arena *arena, D_EntityList entities)
{
  D_HandleList result = {0};
  for(D_EntityNode *n = entities.first; n != 0; n = n->next)
  {
    D_Handle handle = d_handle_from_entity(n->entity);
    d_handle_list_push(arena, &result, handle);
  }
  return result;
}

//- rjf: entity recursion iterators

internal D_EntityRec
d_entity_rec_depth_first(D_Entity *entity, D_Entity *subtree_root, U64 sib_off, U64 child_off)
{
  D_EntityRec result = {0};
  if(!d_entity_is_nil(*MemberFromOffset(D_Entity **, entity, child_off)))
  {
    result.next = *MemberFromOffset(D_Entity **, entity, child_off);
    result.push_count = 1;
  }
  else for(D_Entity *parent = entity; parent != subtree_root && !d_entity_is_nil(parent); parent = parent->parent)
  {
    if(parent != subtree_root && !d_entity_is_nil(*MemberFromOffset(D_Entity **, parent, sib_off)))
    {
      result.next = *MemberFromOffset(D_Entity **, parent, sib_off);
      break;
    }
    result.pop_count += 1;
  }
  return result;
}

//- rjf: ancestor/child introspection

internal D_Entity *
d_entity_child_from_kind(D_Entity *entity, D_EntityKind kind)
{
  D_Entity *result = &d_nil_entity;
  for(D_Entity *child = entity->first; !d_entity_is_nil(child); child = child->next)
  {
    if(child->kind == kind)
    {
      result = child;
      break;
    }
  }
  return result;
}

internal D_Entity *
d_entity_ancestor_from_kind(D_Entity *entity, D_EntityKind kind)
{
  D_Entity *result = &d_nil_entity;
  for(D_Entity *p = entity->parent; !d_entity_is_nil(p); p = p->parent)
  {
    if(p->kind == kind)
    {
      result = p;
      break;
    }
  }
  return result;
}

internal D_EntityList
d_push_entity_child_list_with_kind(Arena *arena, D_Entity *entity, D_EntityKind kind)
{
  D_EntityList result = {0};
  for(D_Entity *child = entity->first; !d_entity_is_nil(child); child = child->next)
  {
    if(child->kind == kind)
    {
      d_entity_list_push(arena, &result, child);
    }
  }
  return result;
}

internal D_Entity *
d_entity_child_from_string_and_kind(D_Entity *parent, String8 string, D_EntityKind kind)
{
  D_Entity *result = &d_nil_entity;
  for(D_Entity *child = parent->first; !d_entity_is_nil(child); child = child->next)
  {
    if(str8_match(child->string, string, 0) && child->kind == kind)
    {
      result = child;
      break;
    }
  }
  return result;
}

//- rjf: entity list building

internal void
d_entity_list_push(Arena *arena, D_EntityList *list, D_Entity *entity)
{
  D_EntityNode *n = push_array(arena, D_EntityNode, 1);
  n->entity = entity;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal D_EntityArray
d_entity_array_from_list(Arena *arena, D_EntityList *list)
{
  D_EntityArray result = {0};
  result.count = list->count;
  result.v = push_array(arena, D_Entity *, result.count);
  U64 idx = 0;
  for(D_EntityNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    result.v[idx] = n->entity;
  }
  return result;
}

//- rjf: entity fuzzy list building

internal D_EntityFuzzyItemArray
d_entity_fuzzy_item_array_from_entity_list_needle(Arena *arena, D_EntityList *list, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  D_EntityArray array = d_entity_array_from_list(scratch.arena, list);
  D_EntityFuzzyItemArray result = d_entity_fuzzy_item_array_from_entity_array_needle(arena, &array, needle);
  return result;
}

internal D_EntityFuzzyItemArray
d_entity_fuzzy_item_array_from_entity_array_needle(Arena *arena, D_EntityArray *array, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  D_EntityFuzzyItemArray result = {0};
  result.count = array->count;
  result.v = push_array(arena, D_EntityFuzzyItem, result.count);
  U64 result_idx = 0;
  for(U64 src_idx = 0; src_idx < array->count; src_idx += 1)
  {
    D_Entity *entity = array->v[src_idx];
    String8 display_string = d_display_string_from_entity(scratch.arena, entity);
    FuzzyMatchRangeList matches = fuzzy_match_find(arena, needle, display_string);
    if(matches.count >= matches.needle_part_count)
    {
      result.v[result_idx].entity = entity;
      result.v[result_idx].matches = matches;
      result_idx += 1;
    }
    else
    {
      String8 search_tags = d_search_tags_from_entity(scratch.arena, entity);
      if(search_tags.size != 0)
      {
        FuzzyMatchRangeList tag_matches = fuzzy_match_find(scratch.arena, needle, search_tags);
        if(tag_matches.count >= tag_matches.needle_part_count)
        {
          result.v[result_idx].entity = entity;
          result.v[result_idx].matches = matches;
          result_idx += 1;
        }
      }
    }
  }
  result.count = result_idx;
  scratch_end(scratch);
  return result;
}

//- rjf: full path building, from file/folder entities

internal String8
d_full_path_from_entity(Arena *arena, D_Entity *entity)
{
  String8 string = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List strs = {0};
    for(D_Entity *e = entity; !d_entity_is_nil(e); e = e->parent)
    {
      if(e->kind == D_EntityKind_File)
      {
        str8_list_push_front(scratch.arena, &strs, e->string);
      }
    }
    StringJoin join = {0};
    join.sep = str8_lit("/");
    string = str8_list_join(arena, &strs, &join);
    scratch_end(scratch);
  }
  return string;
}

//- rjf: display string entities, for referencing entities in ui

internal String8
d_display_string_from_entity(Arena *arena, D_Entity *entity)
{
  String8 result = {0};
  switch(entity->kind)
  {
    default:
    {
      if(entity->string.size != 0)
      {
        result = push_str8_copy(arena, entity->string);
      }
      else
      {
        String8 kind_string = d_entity_kind_display_string_table[entity->kind];
        result = push_str8f(arena, "%S $%I64u", kind_string, entity->id);
      }
    }break;
    
    case D_EntityKind_Target:
    {
      if(entity->string.size != 0)
      {
        result = push_str8_copy(arena, entity->string);
      }
      else
      {
        D_Entity *exe = d_entity_child_from_kind(entity, D_EntityKind_Executable);
        result = push_str8_copy(arena, exe->string);
      }
    }break;
    
    case D_EntityKind_Breakpoint:
    {
      if(entity->string.size != 0)
      {
        result = push_str8_copy(arena, entity->string);
      }
      else
      {
        D_Entity *loc = d_entity_child_from_kind(entity, D_EntityKind_Location);
        if(loc->flags & D_EntityFlag_HasTextPoint)
        {
          result = push_str8f(arena, "%S:%I64d:%I64d", str8_skip_last_slash(loc->string), loc->text_point.line, loc->text_point.column);
        }
        else if(loc->flags & D_EntityFlag_HasVAddr)
        {
          result = str8_from_u64(arena, loc->vaddr, 16, 16, 0);
        }
        else if(loc->string.size != 0)
        {
          result = push_str8_copy(arena, loc->string);
        }
      }
    }break;
    
    case D_EntityKind_Process:
    {
      D_Entity *main_mod_child = d_entity_child_from_kind(entity, D_EntityKind_Module);
      String8 main_mod_name = str8_skip_last_slash(main_mod_child->string);
      result = push_str8f(arena, "%S%s%sPID: %i%s",
                          main_mod_name,
                          main_mod_name.size != 0 ? " " : "",
                          main_mod_name.size != 0 ? "(" : "",
                          entity->ctrl_id,
                          main_mod_name.size != 0 ? ")" : "");
    }break;
    
    case D_EntityKind_Thread:
    {
      String8 name = entity->string;
      if(name.size == 0)
      {
        D_Entity *process = d_entity_ancestor_from_kind(entity, D_EntityKind_Process);
        D_Entity *first_thread = d_entity_child_from_kind(process, D_EntityKind_Thread);
        if(first_thread == entity)
        {
          name = str8_lit("Main Thread");
        }
      }
      result = push_str8f(arena, "%S%s%sTID: %i%s",
                          name,
                          name.size != 0 ? " " : "",
                          name.size != 0 ? "(" : "",
                          entity->ctrl_id,
                          name.size != 0 ? ")" : "");
    }break;
    
    case D_EntityKind_Module:
    {
      result = push_str8_copy(arena, str8_skip_last_slash(entity->string));
    }break;
    
    case D_EntityKind_RecentProject:
    {
      result = push_str8_copy(arena, str8_skip_last_slash(entity->string));
    }break;
  }
  return result;
}

//- rjf: extra search tag strings for fuzzy filtering entities

internal String8
d_search_tags_from_entity(Arena *arena, D_Entity *entity)
{
  String8 result = {0};
  if(entity->kind == D_EntityKind_Thread)
  {
    Temp scratch = scratch_begin(&arena, 1);
    D_Entity *process = d_entity_ancestor_from_kind(entity, D_EntityKind_Process);
    CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity);
    String8List strings = {0};
    for(U64 frame_num = unwind.frames.count; frame_num > 0; frame_num -= 1)
    {
      CTRL_UnwindFrame *f = &unwind.frames.v[frame_num-1];
      U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, f->regs);
      D_Entity *module = d_module_from_process_vaddr(process, rip_vaddr);
      U64 rip_voff = d_voff_from_vaddr(module, rip_vaddr);
      DI_Key dbgi_key = d_dbgi_key_from_module(module);
      String8 procedure_name = d_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff, 0);
      if(procedure_name.size != 0)
      {
        str8_list_push(scratch.arena, &strings, procedure_name);
      }
    }
    StringJoin join = {0};
    join.sep = str8_lit(",");
    result = str8_list_join(arena, &strings, &join);
    scratch_end(scratch);
  }
  return result;
}

//- rjf: entity -> color operations

internal Vec4F32
d_hsva_from_entity(D_Entity *entity)
{
  Vec4F32 result = {0};
  if(entity->flags & D_EntityFlag_HasColor)
  {
    result = entity->color_hsva;
  }
  return result;
}

internal Vec4F32
d_rgba_from_entity(D_Entity *entity)
{
  Vec4F32 result = {0};
  if(entity->flags & D_EntityFlag_HasColor)
  {
    Vec3F32 hsv = v3f32(entity->color_hsva.x, entity->color_hsva.y, entity->color_hsva.z);
    Vec3F32 rgb = rgb_from_hsv(hsv);
    result = v4f32(rgb.x, rgb.y, rgb.z, entity->color_hsva.w);
  }
  return result;
}

//- rjf: entity -> expansion tree keys

internal EV_Key
d_ev_key_from_entity(D_Entity *entity)
{
  EV_Key parent_key = d_parent_ev_key_from_entity(entity);
  EV_Key key = ev_key_make(ev_hash_from_key(parent_key), (U64)entity);
  return key;
}

internal EV_Key
d_parent_ev_key_from_entity(D_Entity *entity)
{
  EV_Key parent_key = ev_key_make(5381, (U64)entity);
  return parent_key;
}

//- rjf: entity -> evaluation

internal D_EntityEval *
d_eval_from_entity(Arena *arena, D_Entity *entity)
{
  D_EntityEval *eval = push_array(arena, D_EntityEval, 1);
  {
    D_Entity *loc = d_entity_child_from_kind(entity, D_EntityKind_Location);
    D_Entity *cnd = d_entity_child_from_kind(entity, D_EntityKind_Condition);
    String8 label_string = push_str8_copy(arena, entity->string);
    String8 loc_string = {0};
    if(loc->flags & D_EntityFlag_HasTextPoint)
    {
      loc_string = push_str8f(arena, "%S:%I64u:%I64u", loc->string, loc->text_point.line, loc->text_point.column);
    }
    else if(loc->flags & D_EntityFlag_HasVAddr)
    {
      loc_string = push_str8f(arena, "0x%I64x", loc->vaddr);
    }
    String8 cnd_string = push_str8_copy(arena, cnd->string);
    eval->enabled      = !entity->disabled;
    eval->hit_count    = entity->u64;
    eval->label_off    = (U64)((U8 *)label_string.str - (U8 *)eval);
    eval->location_off = (U64)((U8 *)loc_string.str - (U8 *)eval);
    eval->condition_off= (U64)((U8 *)cnd_string.str - (U8 *)eval);
  }
  return eval;
}

////////////////////////////////
//~ rjf: Name Allocation

internal U64
d_name_bucket_idx_from_string_size(U64 size)
{
  U64 size_rounded = u64_up_to_pow2(size+1);
  size_rounded = ClampBot((1<<4), size_rounded);
  U64 bucket_idx = 0;
  switch(size_rounded)
  {
    case 1<<4: {bucket_idx = 0;}break;
    case 1<<5: {bucket_idx = 1;}break;
    case 1<<6: {bucket_idx = 2;}break;
    case 1<<7: {bucket_idx = 3;}break;
    case 1<<8: {bucket_idx = 4;}break;
    case 1<<9: {bucket_idx = 5;}break;
    case 1<<10:{bucket_idx = 6;}break;
    default:{bucket_idx = ArrayCount(d_state->free_name_chunks)-1;}break;
  }
  return bucket_idx;
}

internal String8
d_name_alloc(String8 string)
{
  if(string.size == 0) {return str8_zero();}
  U64 bucket_idx = d_name_bucket_idx_from_string_size(string.size);
  
  // rjf: loop -> find node, allocate if not there
  //
  // (we do a loop here so that all allocation logic goes through
  // the same path, such that we *always* pull off a free list,
  // rather than just using what was pushed onto an arena directly,
  // which is not undoable; the free lists we control, and are thus
  // trivially undoable)
  //
  D_NameChunkNode *node = 0;
  for(;node == 0;)
  {
    node = d_state->free_name_chunks[bucket_idx];
    
    // rjf: pull from bucket free list
    if(node != 0)
    {
      if(bucket_idx == ArrayCount(d_state->free_name_chunks)-1)
      {
        node = 0;
        D_NameChunkNode *prev = 0;
        for(D_NameChunkNode *n = d_state->free_name_chunks[bucket_idx];
            n != 0;
            prev = n, n = n->next)
        {
          if(n->size >= string.size+1)
          {
            if(prev == 0)
            {
              d_state->free_name_chunks[bucket_idx] = n->next;
            }
            else
            {
              prev->next = n->next;
            }
            node = n;
            break;
          }
        }
      }
      else
      {
        SLLStackPop(d_state->free_name_chunks[bucket_idx]);
      }
    }
    
    // rjf: no found node -> allocate new, push onto associated free list
    if(node == 0)
    {
      U64 chunk_size = 0;
      if(bucket_idx < ArrayCount(d_state->free_name_chunks)-1)
      {
        chunk_size = 1<<(bucket_idx+4);
      }
      else
      {
        chunk_size = u64_up_to_pow2(string.size);
      }
      U8 *chunk_memory = push_array(d_state->arena, U8, chunk_size);
      D_NameChunkNode *chunk = (D_NameChunkNode *)chunk_memory;
      SLLStackPush(d_state->free_name_chunks[bucket_idx], chunk);
    }
  }
  
  // rjf: fill string & return
  String8 allocated_string = str8((U8 *)node, string.size);
  MemoryCopy((U8 *)node, string.str, string.size);
  return allocated_string;
}

internal void
d_name_release(String8 string)
{
  if(string.size == 0) {return;}
  U64 bucket_idx = d_name_bucket_idx_from_string_size(string.size);
  D_NameChunkNode *node = (D_NameChunkNode *)string.str;
  node->size = u64_up_to_pow2(string.size);
  SLLStackPush(d_state->free_name_chunks[bucket_idx], node);
}

////////////////////////////////
//~ rjf: Entity State Functions

//- rjf: entity allocation + tree forming

internal D_Entity *
d_entity_alloc(D_Entity *parent, D_EntityKind kind)
{
  B32 user_defined_lifetime = !!(d_entity_kind_flags_table[kind] & D_EntityKindFlag_UserDefinedLifetime);
  U64 free_list_idx = !!user_defined_lifetime;
  if(d_entity_is_nil(parent)) { parent = d_state->entities_root; }
  
  // rjf: empty free list -> push new
  if(!d_state->entities_free[free_list_idx])
  {
    D_Entity *entity = push_array(d_state->entities_arena, D_Entity, 1);
    d_state->entities_count += 1;
    d_state->entities_free_count += 1;
    SLLStackPush(d_state->entities_free[free_list_idx], entity);
  }
  
  // rjf: pop new entity off free-list
  D_Entity *entity = d_state->entities_free[free_list_idx];
  SLLStackPop(d_state->entities_free[free_list_idx]);
  d_state->entities_free_count -= 1;
  d_state->entities_active_count += 1;
  
  // rjf: zero entity
  {
    U64 gen = entity->gen;
    MemoryZeroStruct(entity);
    entity->gen = gen;
  }
  
  // rjf: set up alloc'd entity links
  entity->first = entity->last = entity->next = entity->prev = entity->parent = &d_nil_entity;
  entity->parent = parent;
  
  // rjf: stitch up parent links
  if(d_entity_is_nil(parent))
  {
    d_state->entities_root = entity;
  }
  else
  {
    DLLPushBack_NPZ(&d_nil_entity, parent->first, parent->last, entity, next, prev);
  }
  
  // rjf: fill out metadata
  entity->kind = kind;
  d_state->entities_id_gen += 1;
  entity->id = d_state->entities_id_gen;
  entity->gen += 1;
  entity->alloc_time_us = os_now_microseconds();
  entity->params_root = &md_nil_node;
  
  // rjf: initialize to deleted, record history, then "undelete" if this allocation can be undone
  if(user_defined_lifetime)
  {
    // TODO(rjf)
  }
  
  // rjf: dirtify caches
  d_state->kind_alloc_gens[kind] += 1;
  
  // rjf: log
  LogInfoNamedBlockF("new_entity")
  {
    log_infof("kind: \"%S\"\n", d_entity_kind_display_string_table[kind]);
    log_infof("id: $0x%I64x\n", entity->id);
  }
  
  return entity;
}

internal void
d_entity_mark_for_deletion(D_Entity *entity)
{
  if(!d_entity_is_nil(entity))
  {
    entity->flags |= D_EntityFlag_MarkedForDeletion;
  }
}

internal void
d_entity_release(D_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: unpack
  U64 free_list_idx = !!(d_entity_kind_flags_table[entity->kind] & D_EntityKindFlag_UserDefinedLifetime);
  
  // rjf: release whole tree
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    D_Entity *e;
  };
  Task start_task = {0, entity};
  Task *first_task = &start_task;
  Task *last_task = &start_task;
  for(Task *task = first_task; task != 0; task = task->next)
  {
    for(D_Entity *child = task->e->first; !d_entity_is_nil(child); child = child->next)
    {
      Task *t = push_array(scratch.arena, Task, 1);
      t->e = child;
      SLLQueuePush(first_task, last_task, t);
    }
    LogInfoNamedBlockF("end_entity")
    {
      String8 name = d_display_string_from_entity(scratch.arena, task->e);
      log_infof("kind: \"%S\"\n", d_entity_kind_display_string_table[task->e->kind]);
      log_infof("id: $0x%I64x\n", task->e->id);
      log_infof("display_string: \"%S\"\n", name);
    }
    SLLStackPush(d_state->entities_free[free_list_idx], task->e);
    d_state->entities_free_count += 1;
    d_state->entities_active_count -= 1;
    task->e->gen += 1;
    if(task->e->string.size != 0)
    {
      d_name_release(task->e->string);
    }
    if(task->e->params_arena != 0)
    {
      arena_release(task->e->params_arena);
    }
    d_state->kind_alloc_gens[task->e->kind] += 1;
  }
  
  scratch_end(scratch);
}

internal void
d_entity_change_parent(D_Entity *entity, D_Entity *old_parent, D_Entity *new_parent, D_Entity *prev_child)
{
  Assert(entity->parent == old_parent);
  Assert(prev_child->parent == old_parent || d_entity_is_nil(prev_child));
  
  // rjf: fix up links
  if(!d_entity_is_nil(old_parent))
  {
    DLLRemove_NPZ(&d_nil_entity, old_parent->first, old_parent->last, entity, next, prev);
  }
  if(!d_entity_is_nil(new_parent))
  {
    DLLInsert_NPZ(&d_nil_entity, new_parent->first, new_parent->last, prev_child, entity, next, prev);
  }
  entity->parent = new_parent;
  
  // rjf: notify
  d_state->kind_alloc_gens[entity->kind] += 1;
}

//- rjf: entity simple equipment

internal void
d_entity_equip_txt_pt(D_Entity *entity, TxtPt point)
{
  d_require_entity_nonnil(entity, return);
  entity->text_point = point;
  entity->flags |= D_EntityFlag_HasTextPoint;
}

internal void
d_entity_equip_entity_handle(D_Entity *entity, D_Handle handle)
{
  d_require_entity_nonnil(entity, return);
  entity->entity_handle = handle;
  entity->flags |= D_EntityFlag_HasEntityHandle;
}

internal void
d_entity_equip_disabled(D_Entity *entity, B32 value)
{
  d_require_entity_nonnil(entity, return);
  entity->disabled = value;
}

internal void
d_entity_equip_u64(D_Entity *entity, U64 u64)
{
  d_require_entity_nonnil(entity, return);
  entity->u64 = u64;
  entity->flags |= D_EntityFlag_HasU64;
}

internal void
d_entity_equip_color_rgba(D_Entity *entity, Vec4F32 rgba)
{
  d_require_entity_nonnil(entity, return);
  Vec3F32 rgb = v3f32(rgba.x, rgba.y, rgba.z);
  Vec3F32 hsv = hsv_from_rgb(rgb);
  Vec4F32 hsva = v4f32(hsv.x, hsv.y, hsv.z, rgba.w);
  d_entity_equip_color_hsva(entity, hsva);
}

internal void
d_entity_equip_color_hsva(D_Entity *entity, Vec4F32 hsva)
{
  d_require_entity_nonnil(entity, return);
  entity->color_hsva = hsva;
  entity->flags |= D_EntityFlag_HasColor;
}

internal void
d_entity_equip_cfg_src(D_Entity *entity, D_CfgSrc cfg_src)
{
  d_require_entity_nonnil(entity, return);
  entity->cfg_src = cfg_src;
}

internal void
d_entity_equip_timestamp(D_Entity *entity, U64 timestamp)
{
  d_require_entity_nonnil(entity, return);
  entity->timestamp = timestamp;
}

//- rjf: control layer correllation equipment

internal void
d_entity_equip_ctrl_handle(D_Entity *entity, CTRL_Handle handle)
{
  d_require_entity_nonnil(entity, return);
  entity->ctrl_handle = handle;
  entity->flags |= D_EntityFlag_HasCtrlHandle;
}

internal void
d_entity_equip_arch(D_Entity *entity, Arch arch)
{
  d_require_entity_nonnil(entity, return);
  entity->arch = arch;
  entity->flags |= D_EntityFlag_HasArch;
}

internal void
d_entity_equip_ctrl_id(D_Entity *entity, U32 id)
{
  d_require_entity_nonnil(entity, return);
  entity->ctrl_id = id;
  entity->flags |= D_EntityFlag_HasCtrlID;
}

internal void
d_entity_equip_stack_base(D_Entity *entity, U64 stack_base)
{
  d_require_entity_nonnil(entity, return);
  entity->stack_base = stack_base;
  entity->flags |= D_EntityFlag_HasStackBase;
}

internal void
d_entity_equip_vaddr_rng(D_Entity *entity, Rng1U64 range)
{
  d_require_entity_nonnil(entity, return);
  entity->vaddr_rng = range;
  entity->flags |= D_EntityFlag_HasVAddrRng;
}

internal void
d_entity_equip_vaddr(D_Entity *entity, U64 vaddr)
{
  d_require_entity_nonnil(entity, return);
  entity->vaddr = vaddr;
  entity->flags |= D_EntityFlag_HasVAddr;
}

//- rjf: name equipment

internal void
d_entity_equip_name(D_Entity *entity, String8 name)
{
  d_require_entity_nonnil(entity, return);
  if(entity->string.size != 0)
  {
    d_name_release(entity->string);
  }
  if(name.size != 0)
  {
    entity->string = d_name_alloc(name);
  }
  else
  {
    entity->string = str8_zero();
  }
}

internal void
d_entity_equip_namef(D_Entity *entity, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  d_entity_equip_name(entity, string);
  scratch_end(scratch);
}

//- rjf: params tree equipment

internal void
d_entity_equip_params(D_Entity *entity, MD_Node *params)
{
  if(entity->params_arena == 0)
  {
    entity->params_arena = arena_alloc();
  }
  arena_clear(entity->params_arena);
  entity->params_root = md_tree_copy(entity->params_arena, params);
}

internal void
d_entity_equip_param(D_Entity *entity, String8 key, String8 value)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: try to incrementally add to existing tree
  B32 incrementally_added = 0;
  if(!md_node_is_nil(entity->params_root))
  {
    MD_Node *params = entity->params_root;
    MD_Node *key_node = md_child_from_string(params, key, 0);
    if(md_node_is_nil(key_node))
    {
      String8 key_copy = push_str8_copy(entity->params_arena, key);
      key_node = md_push_node(entity->params_arena, MD_NodeKind_Main, MD_NodeFlag_Identifier, key_copy, key_copy, 0);
      md_node_push_child(params, key_node);
      String8 value_copy = push_str8_copy(entity->params_arena, value);
      MD_TokenizeResult value_tokenize = md_tokenize_from_text(scratch.arena, value_copy);
      MD_ParseResult value_parse = md_parse_from_text_tokens(scratch.arena, str8_zero(), value_copy, value_tokenize.tokens);
      for(MD_EachNode(child, value_parse.root->first))
      {
        child->parent = key_node;
      }
      key_node->first = value_parse.root->first;
      key_node->last = value_parse.root->last;
      incrementally_added = 1;
    }
  }
  
  //- rjf: not incrementally added -> fully rewrite parameter tree
  if(!incrementally_added)
  {
    MD_Node *params = md_tree_copy(scratch.arena, entity->params_root);
    MD_Node *key_node = md_child_from_string(params, key, 0);
    if(md_node_is_nil(key_node))
    {
      key_node = md_push_node(scratch.arena, MD_NodeKind_Main, MD_NodeFlag_Identifier, key, key, 0);
      md_node_push_child(params, key_node);
    }
    MD_TokenizeResult value_tokenize = md_tokenize_from_text(scratch.arena, value);
    MD_ParseResult value_parse = md_parse_from_text_tokens(scratch.arena, str8_zero(), value, value_tokenize.tokens);
    for(MD_EachNode(child, value_parse.root->first))
    {
      child->parent = key_node;
    }
    key_node->first = value_parse.root->first;
    key_node->last = value_parse.root->last;
    d_entity_equip_params(entity, params);
  }
  
  scratch_end(scratch);
}

//- rjf: opening folders/files & maintaining the entity model of the filesystem

internal D_Entity *
d_entity_from_path(String8 path, D_EntityFromPathFlags flags)
{
  Temp scratch = scratch_begin(0, 0);
  PathStyle path_style = PathStyle_Relative;
  String8List path_parts = path_normalized_list_from_string(scratch.arena, path, &path_style);
  StringMatchFlags path_match_flags = path_match_flags_from_os(operating_system_from_context());
  
  //- rjf: pass 1: open parts, ignore overrides
  D_Entity *file_no_override = &d_nil_entity;
  {
    D_Entity *parent = d_entity_root();
    for(String8Node *path_part_n = path_parts.first;
        path_part_n != 0;
        path_part_n = path_part_n->next)
    {
      // rjf: find next child
      D_Entity *next_parent = &d_nil_entity;
      for(D_Entity *child = parent->first; !d_entity_is_nil(child); child = child->next)
      {
        B32 name_matches = str8_match(child->string, path_part_n->string, path_match_flags);
        if(name_matches && child->kind == D_EntityKind_File)
        {
          next_parent = child;
          break;
        }
      }
      
      // rjf: no next -> allocate one
      if(d_entity_is_nil(next_parent))
      {
        if(flags & D_EntityFromPathFlag_OpenAsNeeded)
        {
          String8 parent_path = d_full_path_from_entity(scratch.arena, parent);
          String8 path = push_str8f(scratch.arena, "%S%s%S", parent_path, parent_path.size != 0 ? "/" : "", path_part_n->string);
          FileProperties file_properties = os_properties_from_file_path(path);
          if(file_properties.created != 0 || flags & D_EntityFromPathFlag_OpenMissing)
          {
            next_parent = d_entity_alloc(parent, D_EntityKind_File);
            d_entity_equip_name(next_parent, path_part_n->string);
            next_parent->timestamp = file_properties.modified;
            next_parent->flags |= D_EntityFlag_IsFolder * !!(file_properties.flags & FilePropertyFlag_IsFolder);
            next_parent->flags |= D_EntityFlag_IsMissing * !!(file_properties.created == 0);
            if(path_part_n->next != 0)
            {
              next_parent->flags |= D_EntityFlag_IsFolder;
            }
          }
        }
        else
        {
          parent = &d_nil_entity;
          break;
        }
      }
      
      // rjf: next parent -> follow it
      parent = next_parent;
    }
    file_no_override = (parent != d_entity_root() ? parent : &d_nil_entity);
  }
  
  //- rjf: pass 2: follow overrides
  D_Entity *file_overrides_applied = &d_nil_entity;
  if(flags & D_EntityFromPathFlag_AllowOverrides)
  {
    D_Entity *parent = d_entity_root();
    for(String8Node *path_part_n = path_parts.first;
        path_part_n != 0;
        path_part_n = path_part_n->next)
    {
      // rjf: find next child
      D_Entity *next_parent = &d_nil_entity;
      for(D_Entity *child = parent->first; !d_entity_is_nil(child); child = child->next)
      {
        B32 name_matches = str8_match(child->string, path_part_n->string, path_match_flags);
        if(name_matches && child->kind == D_EntityKind_File)
        {
          next_parent = child;
        }
      }
      
      // rjf: no next -> allocate one
      if(d_entity_is_nil(next_parent))
      {
        if(flags & D_EntityFromPathFlag_OpenAsNeeded)
        {
          String8 parent_path = d_full_path_from_entity(scratch.arena, parent);
          String8 path = push_str8f(scratch.arena, "%S%s%S", parent_path, parent_path.size != 0 ? "/" : "", path_part_n->string);
          FileProperties file_properties = os_properties_from_file_path(path);
          if(file_properties.created != 0 || flags & D_EntityFromPathFlag_OpenMissing)
          {
            next_parent = d_entity_alloc(parent, D_EntityKind_File);
            d_entity_equip_name(next_parent, path_part_n->string);
            next_parent->timestamp = file_properties.modified;
            next_parent->flags |= D_EntityFlag_IsFolder * !!(file_properties.flags & FilePropertyFlag_IsFolder);
            next_parent->flags |= D_EntityFlag_IsMissing * !!(file_properties.created == 0);
            if(path_part_n->next != 0)
            {
              next_parent->flags |= D_EntityFlag_IsFolder;
            }
          }
        }
        else
        {
          parent = &d_nil_entity;
          break;
        }
      }
      
      // rjf: next parent -> follow it
      parent = next_parent;
    }
    file_overrides_applied = (parent != d_entity_root() ? parent : &d_nil_entity);;
  }
  
  //- rjf: pick & return result
  D_Entity *result = (flags & D_EntityFromPathFlag_AllowOverrides) ? file_overrides_applied : file_no_override;
  if(flags & D_EntityFromPathFlag_AllowOverrides &&
     result == file_overrides_applied &&
     result->flags & D_EntityFlag_IsMissing)
  {
    result = file_no_override;
  }
  
  scratch_end(scratch);
  return result;
}

//- rjf: file path map override lookups

internal String8List
d_possible_overrides_from_file_path(Arena *arena, String8 file_path)
{
  // NOTE(rjf): This path, given some target file path, scans all file path map
  // overrides, and collects the set of file paths which could've redirected
  // to the target file path given the set of file path maps.
  //
  // For example, if I have a rule saying D:/devel/ maps to C:/devel/, and I
  // feed in C:/devel/foo/bar.txt, then this path will construct
  // D:/devel/foo/bar.txt, as a possible option.
  //
  // It will also preserve C:/devel/foo/bar.txt in the resultant list, so that
  // overrideless files still work through this path, and both redirected
  // files and non-redirected files can go through the same path.
  //
  String8List result = {0};
  str8_list_push(arena, &result, file_path);
  Temp scratch = scratch_begin(&arena, 1);
  PathStyle pth_style = PathStyle_Relative;
  String8List pth_parts = path_normalized_list_from_string(scratch.arena, file_path, &pth_style);
  {
    D_EntityList links = d_query_cached_entity_list_with_kind(D_EntityKind_FilePathMap);
    for(D_EntityNode *n = links.first; n != 0; n = n->next)
    {
      //- rjf: unpack link
      D_Entity *link = n->entity;
      D_Entity *src = d_entity_child_from_kind(link, D_EntityKind_Source);
      D_Entity *dst = d_entity_child_from_kind(link, D_EntityKind_Dest);
      PathStyle src_style = PathStyle_Relative;
      PathStyle dst_style = PathStyle_Relative;
      String8List src_parts = path_normalized_list_from_string(scratch.arena, src->string, &src_style);
      String8List dst_parts = path_normalized_list_from_string(scratch.arena, dst->string, &dst_style);
      
      //- rjf: determine if this link can possibly redirect to the target file path
      B32 dst_redirects_to_pth = 0;
      String8Node *non_redirected_pth_first = 0;
      if(dst_style == pth_style && dst_parts.first != 0 && pth_parts.first != 0)
      {
        dst_redirects_to_pth = 1;
        String8Node *dst_n = dst_parts.first;
        String8Node *pth_n = pth_parts.first;
        for(;dst_n != 0 && pth_n != 0; dst_n = dst_n->next, pth_n = pth_n->next)
        {
          if(!str8_match(dst_n->string, pth_n->string, StringMatchFlag_CaseInsensitive))
          {
            dst_redirects_to_pth = 0;
            break;
          }
          non_redirected_pth_first = pth_n->next;
        }
      }
      
      //- rjf: if this link can redirect to this path via `src` -> `dst`, compute
      // possible full source path, by taking `src` and appending non-redirected
      // suffix (which did not show up in `dst`)
      if(dst_redirects_to_pth)
      {
        String8List candidate_parts = src_parts;
        for(String8Node *p = non_redirected_pth_first; p != 0; p = p->next)
        {
          str8_list_push(scratch.arena, &candidate_parts, p->string);
        }
        StringJoin join = {0};
        join.sep = str8_lit("/");
        String8 candidate_path = str8_list_join(arena, &candidate_parts, 0);
        str8_list_push(arena, &result, candidate_path);
      }
    }
  }
  scratch_end(scratch);
  return result;
}

//- rjf: top-level state queries

internal D_Entity *
d_entity_root(void)
{
  return d_state->entities_root;
}

internal D_EntityList
d_push_entity_list_with_kind(Arena *arena, D_EntityKind kind)
{
  ProfBeginFunction();
  D_EntityList result = {0};
  for(D_Entity *entity = d_state->entities_root;
      !d_entity_is_nil(entity);
      entity = d_entity_rec_depth_first_pre(entity, &d_nil_entity).next)
  {
    if(entity->kind == kind)
    {
      d_entity_list_push(arena, &result, entity);
    }
  }
  ProfEnd();
  return result;
}

internal D_Entity *
d_entity_from_id(D_EntityID id)
{
  D_Entity *result = &d_nil_entity;
  for(D_Entity *e = d_entity_root();
      !d_entity_is_nil(e);
      e = d_entity_rec_depth_first_pre(e, &d_nil_entity).next)
  {
    if(e->id == id)
    {
      result = e;
      break;
    }
  }
  return result;
}

internal D_Entity *
d_machine_entity_from_machine_id(CTRL_MachineID machine_id)
{
  D_Entity *result = &d_nil_entity;
  for(D_Entity *e = d_entity_root();
      !d_entity_is_nil(e);
      e = d_entity_rec_depth_first_pre(e, &d_nil_entity).next)
  {
    if(e->kind == D_EntityKind_Machine && e->ctrl_handle.machine_id == machine_id)
    {
      result = e;
      break;
    }
  }
  if(d_entity_is_nil(result))
  {
    result = d_entity_alloc(d_entity_root(), D_EntityKind_Machine);
    d_entity_equip_ctrl_handle(result, ctrl_handle_make(machine_id, dmn_handle_zero()));
  }
  return result;
}

internal D_Entity *
d_entity_from_ctrl_handle(CTRL_Handle handle)
{
  D_Entity *result = &d_nil_entity;
  if(handle.machine_id != 0 || handle.dmn_handle.u64[0] != 0)
  {
    for(D_Entity *e = d_entity_root();
        !d_entity_is_nil(e);
        e = d_entity_rec_depth_first_pre(e, &d_nil_entity).next)
    {
      if(e->flags & D_EntityFlag_HasCtrlHandle &&
         ctrl_handle_match(e->ctrl_handle, handle))
      {
        result = e;
        break;
      }
    }
  }
  return result;
}

internal D_Entity *
d_entity_from_ctrl_id(CTRL_MachineID machine_id, U32 id)
{
  D_Entity *result = &d_nil_entity;
  if(id != 0)
  {
    for(D_Entity *e = d_entity_root();
        !d_entity_is_nil(e);
        e = d_entity_rec_depth_first_pre(e, &d_nil_entity).next)
    {
      if(e->flags & D_EntityFlag_HasCtrlHandle &&
         e->flags & D_EntityFlag_HasCtrlID &&
         e->ctrl_handle.machine_id == machine_id &&
         e->ctrl_id == id)
      {
        result = e;
        break;
      }
    }
  }
  return result;
}

internal D_Entity *
d_entity_from_name_and_kind(String8 string, D_EntityKind kind)
{
  D_Entity *result = &d_nil_entity;
  D_EntityList all_of_this_kind = d_query_cached_entity_list_with_kind(kind);
  for(D_EntityNode *n = all_of_this_kind.first; n != 0; n = n->next)
  {
    if(str8_match(n->entity->string, string, 0))
    {
      result = n->entity;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Command Stateful Functions

internal void
d_register_cmd_specs(D_CmdSpecInfoArray specs)
{
  U64 registrar_idx = d_state->total_registrar_count;
  d_state->total_registrar_count += 1;
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    // rjf: extract info from array slot
    D_CmdSpecInfo *info = &specs.v[idx];
    
    // rjf: skip empties
    if(info->string.size == 0)
    {
      continue;
    }
    
    // rjf: determine hash/slot
    U64 hash = d_hash_from_string(info->string);
    U64 slot = hash % d_state->cmd_spec_table_size;
    
    // rjf: allocate node & push
    D_CmdSpec *spec = push_array(d_state->arena, D_CmdSpec, 1);
    SLLStackPush_N(d_state->cmd_spec_table[slot], spec, hash_next);
    
    // rjf: fill node
    D_CmdSpecInfo *info_copy = &spec->info;
    info_copy->string                 = push_str8_copy(d_state->arena, info->string);
    info_copy->description            = push_str8_copy(d_state->arena, info->description);
    info_copy->search_tags            = push_str8_copy(d_state->arena, info->search_tags);
    info_copy->display_name           = push_str8_copy(d_state->arena, info->display_name);
    info_copy->flags                  = info->flags;
    info_copy->query                  = info->query;
    spec->registrar_index = registrar_idx;
    spec->ordering_index = idx;
  }
}

internal D_CmdSpec *
d_cmd_spec_from_string(String8 string)
{
  D_CmdSpec *result = &d_nil_cmd_spec;
  {
    U64 hash = d_hash_from_string(string);
    U64 slot = hash%d_state->cmd_spec_table_size;
    for(D_CmdSpec *n = d_state->cmd_spec_table[slot]; n != 0; n = n->hash_next)
    {
      if(str8_match(n->info.string, string, 0))
      {
        result = n;
        break;
      }
    }
  }
  return result;
}

internal D_CmdSpec *
d_cmd_spec_from_kind(D_CmdKind kind)
{
  String8 string = d_core_cmd_kind_spec_info_table[kind].string;
  D_CmdSpec *result = d_cmd_spec_from_string(string);
  return result;
}

internal void
d_cmd_spec_counter_inc(D_CmdSpec *spec)
{
  if(!d_cmd_spec_is_nil(spec))
  {
    spec->run_count += 1;
  }
}

internal D_CmdSpecList
d_push_cmd_spec_list(Arena *arena)
{
  D_CmdSpecList list = {0};
  for(U64 idx = 0; idx < d_state->cmd_spec_table_size; idx += 1)
  {
    for(D_CmdSpec *spec = d_state->cmd_spec_table[idx]; spec != 0; spec = spec->hash_next)
    {
      d_cmd_spec_list_push(arena, &list, spec);
    }
  }
  return list;
}

////////////////////////////////
//~ rjf: View Rule Spec Stateful Functions

internal void
d_register_view_rule_specs(D_ViewRuleSpecInfoArray specs)
{
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    // rjf: extract info from array slot
    D_ViewRuleSpecInfo *info = &specs.v[idx];
    
    // rjf: skip empties
    if(info->string.size == 0)
    {
      continue;
    }
    
    // rjf: determine hash/slot
    U64 hash = d_hash_from_string(info->string);
    U64 slot_idx = hash%d_state->view_rule_spec_table_size;
    
    // rjf: allocate node & push
    D_ViewRuleSpec *spec = push_array(d_state->arena, D_ViewRuleSpec, 1);
    SLLStackPush_N(d_state->view_rule_spec_table[slot_idx], spec, hash_next);
    
    // rjf: fill node
    D_ViewRuleSpecInfo *info_copy = &spec->info;
    MemoryCopyStruct(info_copy, info);
    info_copy->string         = push_str8_copy(d_state->arena, info->string);
    info_copy->display_string = push_str8_copy(d_state->arena, info->display_string);
    info_copy->description    = push_str8_copy(d_state->arena, info->description);
  }
}

internal D_ViewRuleSpec *
d_view_rule_spec_from_string(String8 string)
{
  D_ViewRuleSpec *spec = &d_nil_core_view_rule_spec;
  {
    U64 hash = d_hash_from_string(string);
    U64 slot_idx = hash%d_state->view_rule_spec_table_size;
    for(D_ViewRuleSpec *s = d_state->view_rule_spec_table[slot_idx]; s != 0; s = s->hash_next)
    {
      if(str8_match(string, s->info.string, 0))
      {
        spec = s;
        break;
      }
    }
  }
  return spec;
}

////////////////////////////////
//~ rjf: Stepping "Trap Net" Builders

// NOTE(rjf): Stepping Algorithm Overview (2024/01/17)
//
// The basic idea behind all stepping algorithms in the debugger are setting up
// a "trap net". A "trap net" is just a collection of high-level traps that are
// meant to "catch" a thread after letting it run. This trap net is submitted
// when the debugger frontend sends a "run" command (it is just empty if doing
// an actual 'run' or 'continue'). The debugger control thread then uses this
// trap net to program a state machine, to appropriately respond to a variety
// of debug events which it is passed from the OS.
//
// These are "high-level traps" because they can have specific behavioral info
// attached to them. These are encoded via the `CTRL_TrapFlags` type, which
// allow expression of the following behaviors:
//
//  - end-stepping: when this trap is hit, it will end the stepping operation,
//      and the target will not continue.
//  - ignore-stack-pointer-check: when a trap in the trap net is hit, it will
//      by-default be ignored if the thread's stack pointer has changed. this
//      flag disables that behavior, for when the stack pointer is expected to
//      change (e.g. step-out).
//  - single-step-after-hit: when a trap with this flag is hit, the debugger
//      will immediately single-step the thread which hit it.
//  - save-stack-pointer: when a trap with this flag is hit, it will rewrite
//      the stack pointer which is used to compare against, when deciding
//      whether or not to filter a trap (based on stack pointer changes).
//  - begin-spoof-mode: this enables "spoof mode". "spoof mode" is a special
//      mode that disables the trap net entirely, and lets the thread run
//      freely - but it catches the thread not with a trap, but a false return
//      address. the debugger will overwrite a specific return address on the
//      stack. this address will be overwritten with an address which does NOT
//      point to a valid page, such that when the thread returns out of a
//      particular call frame, the debugger will receive a debug event, at
//      which point it can move the thread back to the correct return address,
//      and resume with the trap net enabled. this is used in "step over"
//      operations, because it avoids target <-> debugger "roundtrips" (e.g.
//      target being stopped, debugger being called with debug events, then
//      target resumes when debugger's control thread is done running) for
//      recursions. (it doesn't make a difference with non-recursive calls,
//      but the debugger can't detect the difference).
//
// Each stepping command prepares its trap net differently.
//
// --- Instruction Step Into --------------------------------------------------
// In this case, no trap net is prepared, and only a low-level single-step is
// performed.
//
// --- Instruction Step Over --------------------------------------------------
// To build a trap net for an instruction-level step-over, the next instruction
// at the thread's current instruction pointer is decoded. If it is a call
// instruction, or if it is a repeating instruction, then a trap with the
// 'end-stepping' behavior is placed at the instruction immediately following
// the 'call' instruction.
//
// --- Line Step Into ---------------------------------------------------------
// For a source-line step-into, the thread's instruction pointer is first used
// to look up into the debug info's line info, to find the machine code in the
// thread's current source line. Every instruction in this range is decoded.
// Traps are then built in the following way:
//
// - 'call' instruction -> if can decode call destination address, place
//     "end-stepping | ignore-stack-pointer-check" trap at destination. if
//     can't, "end-stepping | single-step-after | ignore-stack-pointer-check"
//     trap at call.
// - 'jmp' (both unconditional & conditional) -> if can decode jump destination
//     address, AND if jump leaves the line, place "end-stepping | ignore-
//     stack-pointer-check" trap at destination. if can't, "end-stepping |
//     single-step-after | ignore-stack-pointer-check" trap at jmp. if jump
//     stays within the line, do nothing.
// - 'return' -> place "end-stepping | single-step-after" trap at return inst.
// - "end-stepping" trap is placed at the first address after the line, to
//     catch all steps which simply proceed linearly through the instruction
//     stream.
//
// --- Line Step Over ---------------------------------------------------------
// For a source-line step-over, the thread's instruction pointer is first used
// to look up into the debug info's line info, to find the machine code in the
// thread's current source line. Every instruction in this range is decoded.
// Traps are then built in the following way:
//
// - 'call' instruction -> place "single-step-after | begin-spoof-mode" trap at
//     call instruction.
// - 'jmp' (both unconditional & conditional) -> if can decode jump destination
//     address, AND if jump leaves the line, place "end-stepping" trap at
//     destination. if can't, "end-stepping | single-step-after" trap at jmp.
//     if jump stays within the line, do nothing.
// - 'return' -> place "end-stepping | single-step-after" trap at return inst.
// - "end-stepping" trap is placed at the first address after the line, to
//     catch all steps which simply proceed linearly through the instruction
//     stream.
// - for any instructions which may change the stack pointer, traps are placed
//     at them with the "save-stack-pointer | single-step-after" behaviors.

internal CTRL_TrapList
d_trap_net_from_thread__step_over_inst(Arena *arena, CTRL_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_TrapList result = {0};
  
  // rjf: thread => unpacked info
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  Arch arch = thread->arch;
  U64 ip_vaddr = ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, thread->handle);
  
  // rjf: ip => machine code
  String8 machine_code = {0};
  {
    Rng1U64 rng = r1u64(ip_vaddr, ip_vaddr+max_instruction_size_from_arch(arch));
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->handle, rng, os_now_microseconds()+5000);
    machine_code = machine_code_slice.data;
  }
  
  // rjf: build traps if machine code was read successfully
  if(machine_code.size != 0)
  {
    // rjf: decode instruction
    DASM_Inst inst = dasm_inst_from_code(scratch.arena, arch, ip_vaddr, machine_code, DASM_Syntax_Intel);
    
    // rjf: call => run until call returns
    if(inst.flags & DASM_InstFlag_Call || inst.flags & DASM_InstFlag_Repeats)
    {
      CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, ip_vaddr+inst.size};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal CTRL_TrapList
d_trap_net_from_thread__step_over_line(Arena *arena, CTRL_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  log_infof("step_over_line:\n{\n");
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  Arch arch = thread->arch;
  U64 ip_vaddr = ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, thread->handle);
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  CTRL_Entity *module = ctrl_module_from_process_vaddr(process, ip_vaddr);
  DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
  log_infof("ip_vaddr: 0x%I64x\n", ip_vaddr);
  log_infof("dbgi_key: {%S, 0x%I64x}\n", dbgi_key.path, dbgi_key.min_timestamp);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = ctrl_voff_from_vaddr(module, ip_vaddr);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, ip_voff);
    Rng1U64 line_voff_rng = {0};
    if(lines.first != 0)
    {
      line_voff_rng = lines.first->v.voff_range;
      line_vaddr_rng = ctrl_vaddr_range_from_voff_range(module, line_voff_rng);
      log_infof("line: {%S:%I64i}\n", lines.first->v.file_path, lines.first->v.pt.line);
    }
    log_infof("voff_range: {0x%I64x, 0x%I64x}\n", line_voff_rng.min, line_voff_rng.max);
    log_infof("vaddr_range: {0x%I64x, 0x%I64x}\n", line_vaddr_rng.min, line_vaddr_rng.max);
  }
  
  // rjf: opl line_vaddr_rng -> 0xf00f00 or 0xfeefee? => include in line vaddr range
  //
  // MSVC exports line info at these line numbers when /JMC (Just My Code) debugging
  // is enabled. This is enabled by default normally.
  {
    U64 opl_line_voff_rng = ctrl_voff_from_vaddr(module, line_vaddr_rng.max);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, opl_line_voff_rng);
    if(lines.first != 0 && (lines.first->v.pt.line == 0xf00f00 || lines.first->v.pt.line == 0xfeefee))
    {
      line_vaddr_rng.max = ctrl_vaddr_from_voff(module, lines.first->v.voff_range.max);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  if(good_line_info)
  {
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->handle, line_vaddr_rng, os_now_microseconds()+50000);
    machine_code = machine_code_slice.data;
    LogInfoNamedBlockF("machine_code_slice")
    {
      log_infof("stale: %i\n", machine_code_slice.stale);
      log_infof("any_byte_bad: %i\n", machine_code_slice.any_byte_bad);
      log_infof("any_byte_changed: %i\n", machine_code_slice.any_byte_changed);
      log_infof("bytes:\n[\n");
      for(U64 idx = 0; idx < machine_code_slice.data.size; idx += 1)
      {
        log_infof("0x%x,", machine_code_slice.data.str[idx]);
        if(idx%16 == 15 || idx+1 == machine_code_slice.data.size)
        {
          log_infof("\n");
        }
      }
      log_infof("]\n");
    }
  }
  
  // rjf: machine code => ctrl flow analysis
  DASM_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_line_info)
  {
    ctrl_flow_info = dasm_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                              DASM_InstFlag_Call|
                                                              DASM_InstFlag_Branch|
                                                              DASM_InstFlag_UnconditionalJump|
                                                              DASM_InstFlag_ChangesStackPointer|
                                                              DASM_InstFlag_Return,
                                                              arch,
                                                              line_vaddr_rng.min,
                                                              machine_code);
    LogInfoNamedBlockF("ctrl_flow_info")
    {
      LogInfoNamedBlockF("exit_points") for(DASM_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
      {
        log_infof("{vaddr:0x%I64x, jump_dest_vaddr:0x%I64x, inst_flags:%x}\n", n->v.vaddr, n->v.jump_dest_vaddr, n->v.inst_flags);
      }
    }
  }
  
  // rjf: push traps for all exit points
  if(good_line_info) for(DASM_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DASM_CtrlFlowPoint *point = &n->v;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DASM_InstFlag_Branch|
                            DASM_InstFlag_UnconditionalJump|
                            DASM_InstFlag_Return))
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_EndStepping);
      
      // rjf: omit if this jump stays inside of this line
      if(contains_1u64(line_vaddr_rng, point->jump_dest_vaddr))
      {
        add = 0;
      }
      
      // rjf: trap @ destination, if we can - we can avoid a single-step this way.
      if(point->jump_dest_vaddr != 0)
      {
        trap_addr = point->jump_dest_vaddr;
        flags &= ~CTRL_TrapFlag_SingleStepAfterHit;
      }
      
    }
    
    // rjf: call => place spoof at return spot in stack, single-step after hitting
    else if(point->inst_flags & DASM_InstFlag_Call)
    {
      flags |= (CTRL_TrapFlag_BeginSpoofMode|CTRL_TrapFlag_SingleStepAfterHit);
    }
    
    // rjf: instruction changes stack pointer => save off the stack pointer, single-step over, keep stepping
    else if(point->inst_flags & DASM_InstFlag_ChangesStackPointer)
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_SaveStackPointer);
    }
    
    // rjf: add if appropriate
    if(add)
    {
      CTRL_Trap trap = {flags, trap_addr};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  // rjf: push trap for natural linear flow
  if(good_line_info)
  {
    CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, line_vaddr_rng.max};
    ctrl_trap_list_push(arena, &result, &trap);
  }
  
  // rjf: log
  LogInfoNamedBlockF("traps") for(CTRL_TrapNode *n = result.first; n != 0; n = n->next)
  {
    log_infof("{flags:0x%x, vaddr:0x%I64x}\n", n->v.flags, n->v.vaddr);
  }
  
  scratch_end(scratch);
  log_infof("}\n\n");
  return result;
}

internal CTRL_TrapList
d_trap_net_from_thread__step_into_line(Arena *arena, CTRL_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  Arch arch = thread->arch;
  U64 ip_vaddr = ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, thread->handle);
  CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
  CTRL_Entity *module = ctrl_module_from_process_vaddr(process, ip_vaddr);
  DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = ctrl_voff_from_vaddr(module, ip_vaddr);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, ip_voff);
    Rng1U64 line_voff_rng = {0};
    if(lines.first != 0)
    {
      line_voff_rng = lines.first->v.voff_range;
      line_vaddr_rng = ctrl_vaddr_range_from_voff_range(module, line_voff_rng);
    }
  }
  
  // rjf: opl line_vaddr_rng -> 0xf00f00 or 0xfeefee? => include in line vaddr range
  //
  // MSVC exports line info at these line numbers when /JMC (Just My Code) debugging
  // is enabled. This is enabled by default normally.
  {
    U64 opl_line_voff_rng = ctrl_voff_from_vaddr(module, line_vaddr_rng.max);
    D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, opl_line_voff_rng);
    if(lines.first != 0 && (lines.first->v.pt.line == 0xf00f00 || lines.first->v.pt.line == 0xfeefee))
    {
      line_vaddr_rng.max = ctrl_vaddr_from_voff(module, lines.first->v.voff_range.max);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  if(good_line_info)
  {
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->handle, line_vaddr_rng, os_now_microseconds()+5000);
    machine_code = machine_code_slice.data;
  }
  
  // rjf: machine code => ctrl flow analysis
  DASM_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_line_info)
  {
    ctrl_flow_info = dasm_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                              DASM_InstFlag_Call|
                                                              DASM_InstFlag_Branch|
                                                              DASM_InstFlag_UnconditionalJump|
                                                              DASM_InstFlag_ChangesStackPointer|
                                                              DASM_InstFlag_Return,
                                                              arch,
                                                              line_vaddr_rng.min,
                                                              machine_code);
  }
  
  // rjf: push traps for all exit points
  if(good_line_info) for(DASM_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DASM_CtrlFlowPoint *point = &n->v;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DASM_InstFlag_Call|
                            DASM_InstFlag_Branch|
                            DASM_InstFlag_UnconditionalJump|
                            DASM_InstFlag_Return))
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_EndStepping|CTRL_TrapFlag_IgnoreStackPointerCheck);
      
      // rjf: omit if this jump stays inside of this line
      if(contains_1u64(line_vaddr_rng, point->jump_dest_vaddr))
      {
        add = 0;
      }
      
      // rjf: trap @ destination, if we can - we can avoid a single-step this way.
      if(point->jump_dest_vaddr != 0)
      {
        trap_addr = point->jump_dest_vaddr;
        flags &= ~CTRL_TrapFlag_SingleStepAfterHit;
      }
    }
    
    // rjf: instruction changes stack pointer => save off the stack pointer, single-step over, keep stepping
    else if(point->inst_flags & DASM_InstFlag_ChangesStackPointer)
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_SaveStackPointer);
    }
    
    // rjf: add if appropriate
    if(add)
    {
      CTRL_Trap trap = {flags, trap_addr};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  // rjf: push trap for natural linear flow
  if(good_line_info)
  {
    CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, line_vaddr_rng.max};
    ctrl_trap_list_push(arena, &result, &trap);
  }
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Modules & Debug Info Mappings

//- rjf: module <=> debug info keys

internal DI_Key
d_dbgi_key_from_module(D_Entity *module)
{
  D_Entity *debug_info_path = d_entity_child_from_kind(module, D_EntityKind_DebugInfoPath);
  DI_Key key = {debug_info_path->string, debug_info_path->timestamp};
  return key;
}

internal D_EntityList
d_modules_from_dbgi_key(Arena *arena, DI_Key *dbgi_key)
{
  D_EntityList list = {0};
  D_EntityList all_modules = d_query_cached_entity_list_with_kind(D_EntityKind_Module);
  for(D_EntityNode *n = all_modules.first; n != 0; n = n->next)
  {
    D_Entity *module = n->entity;
    DI_Key module_dbgi_key = d_dbgi_key_from_module(module);
    if(di_key_match(&module_dbgi_key, dbgi_key))
    {
      d_entity_list_push(arena, &list, module);
    }
  }
  return list;
}

//- rjf: voff <=> vaddr

internal U64
d_base_vaddr_from_module(D_Entity *module)
{
  U64 module_base_vaddr = module->vaddr;
  return module_base_vaddr;
}

internal U64
d_voff_from_vaddr(D_Entity *module, U64 vaddr)
{
  U64 module_base_vaddr = d_base_vaddr_from_module(module);
  U64 voff = vaddr - module_base_vaddr;
  return voff;
}

internal U64
d_vaddr_from_voff(D_Entity *module, U64 voff)
{
  U64 module_base_vaddr = d_base_vaddr_from_module(module);
  U64 vaddr = voff + module_base_vaddr;
  return vaddr;
}

internal Rng1U64
d_voff_range_from_vaddr_range(D_Entity *module, Rng1U64 vaddr_rng)
{
  U64 rng_size = dim_1u64(vaddr_rng);
  Rng1U64 voff_rng = {0};
  voff_rng.min = d_voff_from_vaddr(module, vaddr_rng.min);
  voff_rng.max = voff_rng.min + rng_size;
  return voff_rng;
}

internal Rng1U64
d_vaddr_range_from_voff_range(D_Entity *module, Rng1U64 voff_rng)
{
  U64 rng_size = dim_1u64(voff_rng);
  Rng1U64 vaddr_rng = {0};
  vaddr_rng.min = d_vaddr_from_voff(module, voff_rng.min);
  vaddr_rng.max = vaddr_rng.min + rng_size;
  return vaddr_rng;
}

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: symbol lookups

internal String8
d_symbol_name_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff, B32 decorated)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    DI_Scope *scope = di_scope_open();
    RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
    if(result.size == 0)
    {
      U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
      RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
      U64 proc_idx = scope->proc_idx;
      RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, proc_idx);
      E_TypeKey type = e_type_key_ext(E_TypeKind_Function, procedure->type_idx, e_parse_ctx_module_idx_from_rdi(rdi));
      String8 name = {0};
      name.str = rdi_string_from_idx(rdi, procedure->name_string_idx, &name.size);
      if(decorated && procedure->type_idx != 0)
      {
        String8List list = {0};
        e_type_lhs_string_from_key(scratch.arena, type, &list, 0, 0);
        str8_list_push(scratch.arena, &list, name);
        e_type_rhs_string_from_key(scratch.arena, type, &list, 0);
        result = str8_list_join(arena, &list, 0);
      }
      else
      {
        result = push_str8_copy(arena, name);
      }
    }
    if(result.size == 0)
    {
      U64 global_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_GlobalVMap, voff);
      RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, global_idx);
      U64 name_size = 0;
      U8 *name_ptr = rdi_string_from_idx(rdi, global_var->name_string_idx, &name_size);
      result = push_str8_copy(arena, str8(name_ptr, name_size));
    }
    di_scope_close(scope);
    scratch_end(scratch);
  }
  return result;
}

internal String8
d_symbol_name_from_process_vaddr(Arena *arena, D_Entity *process, U64 vaddr, B32 decorated)
{
  ProfBeginFunction();
  String8 result = {0};
  {
    D_Entity *module = d_module_from_process_vaddr(process, vaddr);
    DI_Key dbgi_key = d_dbgi_key_from_module(module);
    U64 voff = d_voff_from_vaddr(module, vaddr);
    result = d_symbol_name_from_dbgi_key_voff(arena, &dbgi_key, voff, decorated);
  }
  ProfEnd();
  return result;
}

//- rjf: symbol -> voff lookups

internal U64
d_voff_from_dbgi_key_symbol_name(DI_Key *dbgi_key, String8 symbol_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DI_Scope *scope = di_scope_open();
  U64 result = 0;
  {
    RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
    RDI_NameMapKind name_map_kinds[] =
    {
      RDI_NameMapKind_GlobalVariables,
      RDI_NameMapKind_Procedures,
    };
    if(rdi != &di_rdi_parsed_nil)
    {
      for(U64 name_map_kind_idx = 0;
          name_map_kind_idx < ArrayCount(name_map_kinds);
          name_map_kind_idx += 1)
      {
        RDI_NameMapKind name_map_kind = name_map_kinds[name_map_kind_idx];
        RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, name_map_kind);
        RDI_ParsedNameMap parsed_name_map = {0};
        rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, symbol_name.str, symbol_name.size);
        
        // rjf: node -> num
        U64 entity_num = 0;
        if(node != 0)
        {
          switch(node->match_count)
          {
            case 1:
            {
              entity_num = node->match_idx_or_idx_run_first + 1;
            }break;
            default:
            {
              U32 num = 0;
              U32 *run = rdi_matches_from_map_node(rdi, node, &num);
              if(num != 0)
              {
                entity_num = run[0]+1;
              }
            }break;
          }
        }
        
        // rjf: num -> voff
        U64 voff = 0;
        if(entity_num != 0) switch(name_map_kind)
        {
          default:{}break;
          case RDI_NameMapKind_GlobalVariables:
          {
            RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, entity_num-1);
            voff = global_var->voff;
          }break;
          case RDI_NameMapKind_Procedures:
          {
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, entity_num-1);
            RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, procedure->root_scope_idx);
            voff = *rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_first);
          }break;
        }
        
        // rjf: nonzero voff -> break
        if(voff != 0)
        {
          result = voff;
          break;
        }
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal U64
d_type_num_from_dbgi_key_name(DI_Key *dbgi_key, String8 name)
{
  ProfBeginFunction();
  DI_Scope *scope = di_scope_open();
  U64 result = 0;
  {
    RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
    RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Types);
    RDI_ParsedNameMap parsed_name_map = {0};
    rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
    RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, name.str, name.size);
    U64 entity_num = 0;
    if(node != 0)
    {
      switch(node->match_count)
      {
        case 1:
        {
          entity_num = node->match_idx_or_idx_run_first + 1;
        }break;
        default:
        {
          U32 num = 0;
          U32 *run = rdi_matches_from_map_node(rdi, node, &num);
          if(num != 0)
          {
            entity_num = run[0]+1;
          }
        }break;
      }
    }
    result = entity_num;
  }
  di_scope_close(scope);
  ProfEnd();
  return result;
}

//- rjf: voff -> line info

internal D_LineList
d_lines_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *scope = di_scope_open();
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  D_LineList result = {0};
  {
    //- rjf: gather line tables
    typedef struct LineTableNode LineTableNode;
    struct LineTableNode
    {
      LineTableNode *next;
      RDI_ParsedLineTable parsed_line_table;
    };
    LineTableNode start_line_table = {0};
    RDI_Unit *unit = rdi_unit_from_voff(rdi, voff);
    RDI_LineTable *unit_line_table = rdi_line_table_from_unit(rdi, unit);
    rdi_parsed_from_line_table(rdi, unit_line_table, &start_line_table.parsed_line_table);
    LineTableNode *top_line_table = 0;
    RDI_Scope *scope = rdi_scope_from_voff(rdi, voff);
    {
      for(RDI_Scope *s = scope;
          s->inline_site_idx != 0;
          s = rdi_element_from_name_idx(rdi, Scopes, s->parent_scope_idx))
      {
        RDI_InlineSite *inline_site = rdi_element_from_name_idx(rdi, InlineSites, s->inline_site_idx);
        if(inline_site->line_table_idx != 0)
        {
          LineTableNode *n = push_array(scratch.arena, LineTableNode, 1);
          SLLStackPush(top_line_table, n);
          RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, inline_site->line_table_idx);
          rdi_parsed_from_line_table(rdi, line_table, &n->parsed_line_table);
        }
      }
    }
    SLLStackPush(top_line_table, &start_line_table);
    
    //- rjf: gather lines in each line table
    Rng1U64 shallowest_voff_range = {0};
    for(LineTableNode *line_table_n = top_line_table; line_table_n != 0; line_table_n = line_table_n->next)
    {
      RDI_ParsedLineTable parsed_line_table = line_table_n->parsed_line_table;
      U64 line_info_idx = rdi_line_info_idx_from_voff(&parsed_line_table, voff);
      if(line_info_idx < parsed_line_table.count)
      {
        RDI_Line *line = &parsed_line_table.lines[line_info_idx];
        RDI_Column *column = (line_info_idx < parsed_line_table.col_count) ? &parsed_line_table.cols[line_info_idx] : 0;
        RDI_SourceFile *file = rdi_element_from_name_idx(rdi, SourceFiles, line->file_idx);
        String8List path_parts = {0};
        for(RDI_FilePathNode *fpn = rdi_element_from_name_idx(rdi, FilePathNodes, file->file_path_node_idx);
            fpn != rdi_element_from_name_idx(rdi, FilePathNodes, 0);
            fpn = rdi_element_from_name_idx(rdi, FilePathNodes, fpn->parent_path_node))
        {
          String8 path_part = {0};
          path_part.str = rdi_string_from_idx(rdi, fpn->name_string_idx, &path_part.size);
          str8_list_push_front(scratch.arena, &path_parts, path_part);
        }
        StringJoin join = {0};
        join.sep = str8_lit("/");
        String8 file_normalized_full_path = str8_list_join(arena, &path_parts, &join);
        D_LineNode *n = push_array(arena, D_LineNode, 1);
        SLLQueuePush(result.first, result.last, n);
        result.count += 1;
        if(line->file_idx != 0 && file_normalized_full_path.size != 0)
        {
          n->v.file_path = file_normalized_full_path;
        }
        n->v.pt = txt_pt(line->line_num, column ? column->col_first : 1);
        n->v.voff_range = r1u64(parsed_line_table.voffs[line_info_idx], parsed_line_table.voffs[line_info_idx+1]);
        n->v.dbgi_key = *dbgi_key;
        if(line_table_n == top_line_table)
        {
          shallowest_voff_range = n->v.voff_range;
        }
      }
    }
    
    //- rjf: clamp all lines from all tables by shallowest (most unwound) range
    for(D_LineNode *n = result.first; n != 0; n = n->next)
    {
      n->v.voff_range = intersect_1u64(n->v.voff_range, shallowest_voff_range);
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return result;
}

//- rjf: file:line -> line info

internal D_LineListArray
d_lines_array_from_file_path_line_range(Arena *arena, String8 file_path, Rng1S64 line_num_range)
{
  D_LineListArray array = {0};
  {
    array.count = dim_1s64(line_num_range)+1;
    array.v = push_array(arena, D_LineList, array.count);
  }
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *scope = di_scope_open();
  DI_KeyList dbgi_keys = d_push_active_dbgi_key_list(scratch.arena);
  String8List overrides = d_possible_overrides_from_file_path(scratch.arena, file_path);
  for(String8Node *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    String8 file_path = override_n->string;
    String8 file_path_normalized = lower_from_str8(scratch.arena, file_path);
    for(DI_KeyNode *dbgi_key_n = dbgi_keys.first;
        dbgi_key_n != 0;
        dbgi_key_n = dbgi_key_n->next)
    {
      // rjf: binary -> rdi
      DI_Key key = dbgi_key_n->v;
      RDI_Parsed *rdi = di_rdi_from_key(scope, &key, 0);
      
      // rjf: file_path_normalized * rdi -> src_id
      B32 good_src_id = 0;
      U32 src_id = 0;
      if(rdi != &di_rdi_parsed_nil)
      {
        RDI_NameMap *mapptr = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_NormalSourcePaths);
        RDI_ParsedNameMap map = {0};
        rdi_parsed_from_name_map(rdi, mapptr, &map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, file_path_normalized.str, file_path_normalized.size);
        if(node != 0)
        {
          U32 id_count = 0;
          U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
          if(id_count > 0)
          {
            good_src_id = 1;
            src_id = ids[0];
          }
        }
      }
      
      // rjf: good src-id -> look up line info for visible range
      if(good_src_id)
      {
        RDI_SourceFile *src = rdi_element_from_name_idx(rdi, SourceFiles, src_id);
        RDI_SourceLineMap *src_line_map = rdi_element_from_name_idx(rdi, SourceLineMaps, src->source_line_map_idx);
        RDI_ParsedSourceLineMap line_map = {0};
        rdi_parsed_from_source_line_map(rdi, src_line_map, &line_map);
        U64 line_idx = 0;
        for(S64 line_num = line_num_range.min;
            line_num <= line_num_range.max;
            line_num += 1, line_idx += 1)
        {
          D_LineList *list = &array.v[line_idx];
          U32 voff_count = 0;
          U64 *voffs = rdi_line_voffs_from_num(&line_map, u32_from_u64_saturate((U64)line_num), &voff_count);
          for(U64 idx = 0; idx < voff_count; idx += 1)
          {
            U64 base_voff = voffs[idx];
            U64 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, base_voff);
            RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
            RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
            RDI_ParsedLineTable unit_line_info = {0};
            rdi_parsed_from_line_table(rdi, line_table, &unit_line_info);
            U64 line_info_idx = rdi_line_info_idx_from_voff(&unit_line_info, base_voff);
            if(unit_line_info.voffs != 0)
            {
              Rng1U64 range = r1u64(base_voff, unit_line_info.voffs[line_info_idx+1]);
              S64 actual_line = (S64)unit_line_info.lines[line_info_idx].line_num;
              D_LineNode *n = push_array(arena, D_LineNode, 1);
              n->v.voff_range = range;
              n->v.pt.line = (S64)actual_line;
              n->v.pt.column = 1;
              n->v.dbgi_key = key;
              SLLQueuePush(list->first, list->last, n);
              list->count += 1;
            }
          }
        }
      }
      
      // rjf: good src id -> push to relevant dbgi keys
      if(good_src_id)
      {
        di_key_list_push(arena, &array.dbgi_keys, &key);
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return array;
}

internal D_LineList
d_lines_from_file_path_line_num(Arena *arena, String8 file_path, S64 line_num)
{
  D_LineListArray array = d_lines_array_from_file_path_line_range(arena, file_path, r1s64(line_num, line_num+1));
  D_LineList list = {0};
  if(array.count != 0)
  {
    list = array.v[0];
  }
  return list;
}

////////////////////////////////
//~ rjf: Process/Thread/Module Info Lookups

internal D_Entity *
d_module_from_process_vaddr(D_Entity *process, U64 vaddr)
{
  D_Entity *module = &d_nil_entity;
  for(D_Entity *child = process->first; !d_entity_is_nil(child); child = child->next)
  {
    if(child->kind == D_EntityKind_Module && contains_1u64(child->vaddr_rng, vaddr))
    {
      module = child;
      break;
    }
  }
  return module;
}

internal D_Entity *
d_module_from_thread(D_Entity *thread)
{
  D_Entity *process = thread->parent;
  U64 rip = d_query_cached_rip_from_thread(thread);
  return d_module_from_process_vaddr(process, rip);
}

internal U64
d_tls_base_vaddr_from_process_root_rip(D_Entity *process, U64 root_vaddr, U64 rip_vaddr)
{
  ProfBeginFunction();
  U64 base_vaddr = 0;
  Temp scratch = scratch_begin(0, 0);
  if(!d_ctrl_targets_running())
  {
    //- rjf: unpack module info
    D_Entity *module = d_module_from_process_vaddr(process, rip_vaddr);
    Rng1U64 tls_vaddr_range = ctrl_tls_vaddr_range_from_module(module->ctrl_handle);
    U64 addr_size = bit_size_from_arch(process->arch)/8;
    
    //- rjf: read module's TLS index
    U64 tls_index = 0;
    if(addr_size != 0)
    {
      CTRL_ProcessMemorySlice tls_index_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_handle, tls_vaddr_range, 0);
      if(tls_index_slice.data.size >= addr_size)
      {
        tls_index = *(U64 *)tls_index_slice.data.str;
      }
    }
    
    //- rjf: PE path
    if(addr_size != 0)
    {
      U64 thread_info_addr = root_vaddr;
      U64 tls_addr_off = tls_index*addr_size;
      U64 tls_addr_array = 0;
      CTRL_ProcessMemorySlice tls_addr_array_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_handle, r1u64(thread_info_addr, thread_info_addr+addr_size), 0);
      String8 tls_addr_array_data = tls_addr_array_slice.data;
      if(tls_addr_array_data.size >= 8)
      {
        MemoryCopy(&tls_addr_array, tls_addr_array_data.str, sizeof(U64));
      }
      CTRL_ProcessMemorySlice result_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_handle, r1u64(tls_addr_array + tls_addr_off, tls_addr_array + tls_addr_off + addr_size), 0);
      String8 result_data = result_slice.data;
      if(result_data.size >= 8)
      {
        MemoryCopy(&base_vaddr, result_data.str, sizeof(U64));
      }
    }
    
    //- rjf: non-PE path (not implemented)
#if 0
    if(!bin_is_pe)
    {
      // TODO(rjf): not supported. old code from the prototype that Nick had sketched out:
      // TODO(nick): This code works only if the linked c runtime library is glibc.
      // Implement CRT detection here.
      
      U64 dtv_addr = UINT64_MAX;
      demon_read_memory(process->demon_handle, &dtv_addr, thread_info_addr, addr_size);
      
      /*
        union delta_thread_vector
        {
          size_t counter;
          struct
          {
            void *value;
            void *to_free;
          } pointer;
        };
      */
      
      U64 dtv_size = 16;
      U64 dtv_count = 0;
      demon_read_memory(process->demon_handle, &dtv_count, dtv_addr - dtv_size, addr_size);
      
      if (tls_index > 0 && tls_index < dtv_count)
      {
        demon_read_memory(process->demon_handle, &result, dtv_addr + dtv_size*tls_index, addr_size);
      }
    }
#endif
  }
  scratch_end(scratch);
  ProfEnd();
  return base_vaddr;
}

internal Arch
d_arch_from_entity(D_Entity *entity)
{
  return entity->arch;
}

internal E_String2NumMap *
d_push_locals_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff)
{
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  E_String2NumMap *result = e_push_locals_map_from_rdi_voff(arena, rdi, voff);
  return result;
}

internal E_String2NumMap *
d_push_member_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff)
{
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  E_String2NumMap *result = e_push_member_map_from_rdi_voff(arena, rdi, voff);
  return result;
}

internal D_Entity *
d_module_from_thread_candidates(D_Entity *thread, D_EntityList *candidates)
{
  D_Entity *src_module = d_module_from_thread(thread);
  D_Entity *module = &d_nil_entity;
  D_Entity *process = d_entity_ancestor_from_kind(thread, D_EntityKind_Process);
  for(D_EntityNode *n = candidates->first; n != 0; n = n->next)
  {
    D_Entity *candidate_module = n->entity;
    D_Entity *candidate_process = d_entity_ancestor_from_kind(candidate_module, D_EntityKind_Process);
    if(candidate_process == process)
    {
      module = candidate_module;
    }
    if(candidate_module == src_module)
    {
      break;
    }
  }
  return module;
}

internal D_Unwind
d_unwind_from_ctrl_unwind(Arena *arena, DI_Scope *di_scope, D_Entity *process, CTRL_Unwind *base_unwind)
{
  Arch arch = d_arch_from_entity(process);
  D_Unwind result = {0};
  result.frames.concrete_frame_count = base_unwind->frames.count;
  result.frames.total_frame_count = result.frames.concrete_frame_count;
  result.frames.v = push_array(arena, D_UnwindFrame, result.frames.concrete_frame_count);
  for(U64 idx = 0; idx < result.frames.concrete_frame_count; idx += 1)
  {
    CTRL_UnwindFrame *src = &base_unwind->frames.v[idx];
    D_UnwindFrame *dst = &result.frames.v[idx];
    U64 rip_vaddr = regs_rip_from_arch_block(arch, src->regs);
    D_Entity *module = d_module_from_process_vaddr(process, rip_vaddr);
    U64 rip_voff = d_voff_from_vaddr(module, rip_vaddr);
    DI_Key dbgi_key = d_dbgi_key_from_module(module);
    RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, 0);
    RDI_Scope *scope = rdi_scope_from_voff(rdi, rip_voff);
    
    // rjf: fill concrete frame info
    dst->regs = src->regs;
    dst->rdi = rdi;
    dst->procedure = rdi_element_from_name_idx(rdi, Procedures, scope->proc_idx);
    
    // rjf: push inline frames
    for(RDI_Scope *s = scope;
        s->inline_site_idx != 0;
        s = rdi_element_from_name_idx(rdi, Scopes, s->parent_scope_idx))
    {
      RDI_InlineSite *site = rdi_element_from_name_idx(rdi, InlineSites, s->inline_site_idx);
      D_UnwindInlineFrame *inline_frame = push_array(arena, D_UnwindInlineFrame, 1);
      DLLPushFront(dst->first_inline_frame, dst->last_inline_frame, inline_frame);
      inline_frame->inline_site = site;
      dst->inline_frame_count += 1;
      result.frames.inline_frame_count += 1;
      result.frames.total_frame_count += 1;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Target Controls

//- rjf: stopped info from the control thread

internal CTRL_Event
d_ctrl_last_stop_event(void)
{
  return d_state->ctrl_last_stop_event;
}

////////////////////////////////
//~ rjf: Evaluation Context

//- rjf: entity <-> eval space

internal D_Entity *
d_entity_from_eval_space(E_Space space)
{
  D_Entity *entity = &d_nil_entity;
  if(space.u64[0] == 0 && space.u64[1] != 0)
  {
    entity = (D_Entity *)space.u64[1];
  }
  return entity;
}

internal E_Space
d_eval_space_from_entity(D_Entity *entity)
{
  E_Space space = {0};
  space.u64[1] = (U64)entity;
  return space;
}

//- rjf: eval space reads/writes

internal B32
d_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range)
{
  B32 result = 0;
  D_Entity *entity = d_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: nil-space -> fall back to file system
    case D_EntityKind_Nil:
    {
      U128 key = space;
      U128 hash = hs_hash_from_key(key, 0);
      HS_Scope *scope = hs_scope_open();
      {
        String8 data = hs_data_from_hash(scope, hash);
        Rng1U64 legal_range = r1u64(0, data.size);
        Rng1U64 read_range = intersect_1u64(range, legal_range);
        if(read_range.min < read_range.max)
        {
          result = 1;
          MemoryCopy(out, data.str + read_range.min, dim_1u64(read_range));
        }
      }
      hs_scope_close(scope);
    }break;
    
    //- rjf: default -> evaluating a debugger entity; read from entity POD evaluation
    default:
    {
      Temp scratch = scratch_begin(0, 0);
      arena_push(scratch.arena, 0, 64);
      U64 pos_min = arena_pos(scratch.arena);
      D_EntityEval *eval = d_eval_from_entity(scratch.arena, entity);
      U64 pos_opl = arena_pos(scratch.arena);
      Rng1U64 legal_range = r1u64(0, pos_opl-pos_min);
      if(contains_1u64(legal_range, range.min))
      {
        result = 1;
        U64 range_dim = dim_1u64(range);
        U64 bytes_to_read = Min(range_dim, (legal_range.max - range.min));
        MemoryCopy(out, ((U8 *)eval) + range.min, bytes_to_read);
        if(bytes_to_read < range_dim)
        {
          MemoryZero((U8 *)out + bytes_to_read, range_dim - bytes_to_read);
        }
      }
      scratch_end(scratch);
    }break;
    
    //- rjf: process -> reading process memory
    case D_EntityKind_Process:
    {
      Temp scratch = scratch_begin(0, 0);
      CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, entity->ctrl_handle, range, d_state->frame_eval_memread_endt_us);
      String8 data = slice.data;
      if(data.size == dim_1u64(range))
      {
        result = 1;
        MemoryCopy(out, data.str, data.size);
      }
      scratch_end(scratch);
    }break;
    
    //- rjf: thread -> reading from thread register block
    case D_EntityKind_Thread:
    {
      Temp scratch = scratch_begin(0, 0);
      CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity);
      U64 frame_idx = e_interpret_ctx->reg_unwind_count;
      if(frame_idx < unwind.frames.count)
      {
        CTRL_UnwindFrame *f = &unwind.frames.v[frame_idx];
        U64 regs_size = regs_block_size_from_arch(e_interpret_ctx->reg_arch);
        Rng1U64 legal_range = r1u64(0, regs_size);
        Rng1U64 read_range = intersect_1u64(legal_range, range);
        U64 read_size = dim_1u64(read_range);
        MemoryCopy(out, (U8 *)f->regs + read_range.min, read_size);
        result = (read_size == dim_1u64(range));
      }
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal B32
d_eval_space_write(void *u, E_Space space, void *in, Rng1U64 range)
{
  B32 result = 0;
  D_Entity *entity = d_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: default -> making commits to entity evaluation
    default:
    {
      Temp scratch = scratch_begin(0, 0);
      D_EntityEval *eval = d_eval_from_entity(scratch.arena, entity);
      U64 range_dim = dim_1u64(range);
      if(range.min == OffsetOf(D_EntityEval, enabled) &&
         range_dim >= 1)
      {
        result = 1;
        B32 new_enabled = !!((U8 *)in)[0];
        d_entity_equip_disabled(entity, !new_enabled);
      }
      else if(range.min == eval->label_off &&
              range_dim >= 1)
      {
        result = 1;
        String8 new_name = str8_cstring_capped((U8 *)in, (U8 *)in+range_dim);
        d_entity_equip_name(entity, new_name);
      }
      else if(range.min == eval->condition_off &&
              range_dim >= 1)
      {
        result = 1;
        D_Entity *condition = d_entity_child_from_kind(entity, D_EntityKind_Condition);
        if(d_entity_is_nil(condition))
        {
          condition = d_entity_alloc(entity, D_EntityKind_Condition);
        }
        String8 new_name = str8_cstring_capped((U8 *)in, (U8 *)in+range_dim);
        d_entity_equip_name(condition, new_name);
      }
      scratch_end(scratch);
    }break;
    
    //- rjf: process -> commit to process memory
    case D_EntityKind_Process:
    {
      result = ctrl_process_write(entity->ctrl_handle, range, in);
    }break;
    
    //- rjf: thread -> commit to thread's register block
    case D_EntityKind_Thread:
    {
      CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity);
      U64 frame_idx = 0;
      if(frame_idx < unwind.frames.count)
      {
        Temp scratch = scratch_begin(0, 0);
        U64 regs_size = regs_block_size_from_arch(d_arch_from_entity(entity));
        Rng1U64 legal_range = r1u64(0, regs_size);
        Rng1U64 write_range = intersect_1u64(legal_range, range);
        U64 write_size = dim_1u64(write_range);
        CTRL_UnwindFrame *f = &unwind.frames.v[frame_idx];
        void *new_regs = push_array(scratch.arena, U8, regs_size);
        MemoryCopy(new_regs, f->regs, regs_size);
        MemoryCopy((U8 *)new_regs + write_range.min, in, write_size);
        result = ctrl_thread_write_reg_block(entity->ctrl_handle, new_regs);
        scratch_end(scratch);
      }
    }break;
  }
  return result;
}

//- rjf: asynchronous streamed reads -> hashes from spaces

internal U128
d_key_from_eval_space_range(E_Space space, Rng1U64 range, B32 zero_terminated)
{
  U128 result = {0};
  D_Entity *entity = d_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: nil space -> filesystem key encoded inside of `space`
    case D_EntityKind_Nil:
    {
      result = space;
    }break;
    
    //- rjf: process space -> query 
    case D_EntityKind_Process:
    {
      result = ctrl_hash_store_key_from_process_vaddr_range(entity->ctrl_handle, range, zero_terminated);
    }break;
  }
  return result;
}

//- rjf: space -> entire range

internal Rng1U64
d_whole_range_from_eval_space(E_Space space)
{
  Rng1U64 result = r1u64(0, 0);
  D_Entity *entity = d_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: nil space -> filesystem key encoded inside of `space`
    case D_EntityKind_Nil:
    {
      HS_Scope *scope = hs_scope_open();
      U128 hash = {0};
      for(U64 idx = 0; idx < 2; idx += 1)
      {
        hash = hs_hash_from_key(space, idx);
        if(!u128_match(hash, u128_zero()))
        {
          break;
        }
      }
      String8 data = hs_data_from_hash(scope, hash);
      result = r1u64(0, data.size);
      hs_scope_close(scope);
    }break;
    case D_EntityKind_Process:
    {
      result = r1u64(0, 0x7FFFFFFFFFFFull);
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Evaluation View Visualization & Interaction

//- rjf: writing values back to child processes

internal B32
d_commit_eval_value_string(E_Eval dst_eval, String8 string)
{
  B32 result = 0;
  if(dst_eval.mode == E_Mode_Offset)
  {
    Temp scratch = scratch_begin(0, 0);
    E_TypeKey type_key = e_type_unwrap(dst_eval.type_key);
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
    E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(dst_eval.type_key)));
    E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
    String8 commit_data = {0};
    B32 commit_at_ptr_dest = 0;
    if(E_TypeKind_FirstBasic <= type_kind && type_kind <= E_TypeKind_LastBasic)
    {
      E_Eval src_eval = e_eval_from_string(scratch.arena, string);
      commit_data = push_str8_copy(scratch.arena, str8_struct(&src_eval.value));
      commit_data.size = Min(commit_data.size, e_type_byte_size_from_key(type_key));
    }
    else if(type_kind == E_TypeKind_Ptr || type_kind == E_TypeKind_Array)
    {
      E_Eval src_eval = e_eval_from_string(scratch.arena, string);
      E_Eval src_eval_value = e_value_eval_from_eval(src_eval);
      E_TypeKind src_eval_value_type_kind = e_type_kind_from_key(src_eval_value.type_key);
      if(type_kind == E_TypeKind_Ptr &&
         (e_type_kind_is_pointer_or_ref(src_eval_value_type_kind) ||
          e_type_kind_is_integer(src_eval_value_type_kind)) &&
         src_eval_value.value.u64 != 0 && src_eval_value.mode == E_Mode_Value)
      {
        commit_data = push_str8_copy(scratch.arena, str8_struct(&src_eval.value));
        commit_data.size = Min(commit_data.size, e_type_byte_size_from_key(type_key));
      }
      else if(direct_type_kind == E_TypeKind_Char8 ||
              direct_type_kind == E_TypeKind_UChar8 ||
              e_type_kind_is_integer(direct_type_kind))
      {
        if(string.size >= 1 && string.str[0] == '"')
        {
          string = str8_skip(string, 1);
        }
        if(string.size >= 1 && string.str[string.size-1] == '"')
        {
          string = str8_chop(string, 1);
        }
        commit_data = e_raw_from_escaped_string(scratch.arena, string);
        commit_data.size += 1;
        if(type_kind == E_TypeKind_Ptr)
        {
          commit_at_ptr_dest = 1;
        }
      }
    }
    if(commit_data.size != 0 && e_type_byte_size_from_key(type_key) != 0)
    {
      U64 dst_offset = dst_eval.value.u64;
      if(dst_eval.mode == E_Mode_Offset && commit_at_ptr_dest)
      {
        E_Eval dst_value_eval = e_value_eval_from_eval(dst_eval);
        dst_offset = dst_value_eval.value.u64;
      }
      result = e_space_write(dst_eval.space, commit_data.str, r1u64(dst_offset, dst_offset + commit_data.size));
    }
    scratch_end(scratch);
  }
  return result;
}

//- rjf: view rule config tree info extraction

internal U64
d_base_offset_from_eval(E_Eval eval)
{
  if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(eval.type_key)))
  {
    eval = e_value_eval_from_eval(eval);
  }
  return eval.value.u64;
}

internal E_Value
d_value_from_params_key(MD_Node *params, String8 key)
{
  Temp scratch = scratch_begin(0, 0);
  MD_Node *key_node = md_child_from_string(params, key, 0);
  String8 expr = md_string_from_children(scratch.arena, key_node);
  E_Eval eval = e_eval_from_string(scratch.arena, expr);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  scratch_end(scratch);
  return value_eval.value;
}

internal Rng1U64
d_range_from_eval_params(E_Eval eval, MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  U64 size = d_value_from_params_key(params, str8_lit("size")).u64;
  E_TypeKey type_key = e_type_unwrap(eval.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(eval.type_key));
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  if(size == 0 && e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                               direct_type_kind == E_TypeKind_Union ||
                                                               direct_type_kind == E_TypeKind_Class ||
                                                               direct_type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(e_type_direct_from_key(e_type_unwrap(eval.type_key)));
  }
  if(size == 0 && eval.mode == E_Mode_Offset && (type_kind == E_TypeKind_Struct ||
                                                 type_kind == E_TypeKind_Union ||
                                                 type_kind == E_TypeKind_Class ||
                                                 type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(e_type_unwrap(eval.type_key));
  }
  if(size == 0)
  {
    size = 16384;
  }
  Rng1U64 result = {0};
  result.min = d_base_offset_from_eval(eval);
  result.max = result.min + size;
  scratch_end(scratch);
  return result;
}

internal TXT_LangKind
d_lang_kind_from_eval_params(E_Eval eval, MD_Node *params)
{
  TXT_LangKind lang_kind = TXT_LangKind_Null;
  if(eval.expr->kind == E_ExprKind_LeafFilePath)
  {
    lang_kind = txt_lang_kind_from_extension(str8_skip_last_dot(eval.expr->string));
  }
  else
  {
    MD_Node *lang_node = md_child_from_string(params, str8_lit("lang"), 0);
    String8 lang_kind_string = lang_node->first->string;
    lang_kind = txt_lang_kind_from_extension(lang_kind_string);
  }
  return lang_kind;
}

internal Arch
d_arch_from_eval_params(E_Eval eval, MD_Node *params)
{
  Arch arch = Arch_Null;
  MD_Node *arch_node = md_child_from_string(params, str8_lit("arch"), 0);
  String8 arch_kind_string = arch_node->first->string;
  if(str8_match(arch_kind_string, str8_lit("x64"), StringMatchFlag_CaseInsensitive))
  {
    arch = Arch_x64;
  }
  return arch;
}

internal Vec2S32
d_dim2s32_from_eval_params(E_Eval eval, MD_Node *params)
{
  Vec2S32 dim = v2s32(1, 1);
  {
    dim.x = d_value_from_params_key(params, str8_lit("w")).s32;
    dim.y = d_value_from_params_key(params, str8_lit("h")).s32;
  }
  return dim;
}

internal R_Tex2DFormat
d_tex2dformat_from_eval_params(E_Eval eval, MD_Node *params)
{
  R_Tex2DFormat result = R_Tex2DFormat_RGBA8;
  {
    MD_Node *fmt_node = md_child_from_string(params, str8_lit("fmt"), 0);
    for(EachNonZeroEnumVal(R_Tex2DFormat, fmt))
    {
      if(str8_match(r_tex2d_format_display_string_table[fmt], fmt_node->first->string, StringMatchFlag_CaseInsensitive))
      {
        result = fmt;
        break;
      }
    }
  }
  return result;
}

//- rjf: eval -> entity

internal D_Entity *
d_entity_from_eval_string(String8 string)
{
  D_Entity *entity = &d_nil_entity;
  {
    Temp scratch = scratch_begin(0, 0);
    E_Eval eval = e_eval_from_string(scratch.arena, string);
    entity = d_entity_from_eval_space(eval.space);
    scratch_end(scratch);
  }
  return entity;
}

internal String8
d_eval_string_from_entity(Arena *arena, D_Entity *entity)
{
  String8 eval_string = push_str8f(arena, "macro:`$%I64u`", entity->id);
  return eval_string;
}

//- rjf: eval <-> file path

internal String8
d_file_path_from_eval_string(Arena *arena, String8 string)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_Eval eval = e_eval_from_string(scratch.arena, string);
    if(eval.expr->kind == E_ExprKind_LeafFilePath)
    {
      result = d_cfg_raw_from_escaped_string(arena, eval.expr->string);
    }
    scratch_end(scratch);
  }
  return result;
}

internal String8
d_eval_string_from_file_path(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 string_escaped = d_cfg_escaped_from_raw_string(scratch.arena, string);
  String8 result = push_str8f(arena, "file:\"%S\"", string_escaped);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame data

internal U64
d_frame_index(void)
{
  return d_state->frame_index;
}

internal Arena *
d_frame_arena(void)
{
  return d_state->frame_arenas[d_state->frame_index%ArrayCount(d_state->frame_arenas)];
}

//- rjf: interaction registers

internal D_Regs *
d_regs(void)
{
  D_Regs *regs = &d_state->top_regs->v;
  return regs;
}

internal D_Regs *
d_base_regs(void)
{
  D_Regs *regs = &d_state->base_regs.v;
  return regs;
}

internal D_Regs *
d_push_regs(void)
{
  D_Regs *top = d_regs();
  D_RegsNode *n = push_array(d_frame_arena(), D_RegsNode, 1);
  MemoryCopyStruct(&n->v, top);
  SLLStackPush(d_state->top_regs, n);
  return &n->v;
}

internal D_Regs *
d_pop_regs(void)
{
  D_Regs *regs = &d_state->top_regs->v;
  SLLStackPop(d_state->top_regs);
  if(d_state->top_regs == 0)
  {
    d_state->top_regs = &d_state->base_regs;
  }
  return regs;
}

//- rjf: control state

internal D_RunKind
d_ctrl_last_run_kind(void)
{
  return d_state->ctrl_last_run_kind;
}

internal U64
d_ctrl_last_run_frame_idx(void)
{
  return d_state->ctrl_last_run_frame_idx;
}

internal B32
d_ctrl_targets_running(void)
{
  return d_state->ctrl_is_running;
}

//- rjf: config serialization

internal String8
d_cfg_escaped_from_raw_string(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List parts = {0};
  U64 split_start_idx = 0;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    U8 byte = (idx < string.size ? string.str[idx] : 0);
    if(byte == 0 || byte == '\"' || byte == '\\')
    {
      String8 part = str8_substr(string, r1u64(split_start_idx, idx));
      str8_list_push(scratch.arena, &parts, part);
      switch(byte)
      {
        default:{}break;
        case '\"':{str8_list_push(scratch.arena, &parts, str8_lit("\\\""));}break;
        case '\\':{str8_list_push(scratch.arena, &parts, str8_lit("\\\\"));}break;
      }
      split_start_idx = idx+1;
    }
  }
  StringJoin join = {0};
  String8 result = str8_list_join(arena, &parts, &join);
  scratch_end(scratch);
  return result;
}

internal String8
d_cfg_raw_from_escaped_string(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List parts = {0};
  U64 split_start_idx = 0;
  U64 extra_advance = 0;
  for(U64 idx = 0; idx <= string.size; ((idx += 1+extra_advance), extra_advance=0))
  {
    U8 byte = (idx < string.size ? string.str[idx] : 0);
    if(byte == 0 || byte == '\\')
    {
      String8 part = str8_substr(string, r1u64(split_start_idx, idx));
      str8_list_push(scratch.arena, &parts, part);
      if(byte == '\\' && idx+1 < string.size)
      {
        switch(string.str[idx+1])
        {
          default:{}break;
          case '"': {extra_advance = 1; str8_list_push(scratch.arena, &parts, str8_lit("\""));}break;
          case '\\':{extra_advance = 1; str8_list_push(scratch.arena, &parts, str8_lit("\\"));}break;
        }
      }
      split_start_idx = idx+1+extra_advance;
    }
  }
  StringJoin join = {0};
  String8 result = str8_list_join(arena, &parts, &join);
  scratch_end(scratch);
  return result;
}

internal String8List
d_cfg_strings_from_core(Arena *arena, String8 root_path, D_CfgSrc source)
{
  ProfBeginFunction();
  local_persist char *spaces = "                                                                                ";
  local_persist char *slashes= "////////////////////////////////////////////////////////////////////////////////";
  String8List strs = {0};
  
  //- rjf: write all entities
  {
    for(EachEnumVal(D_EntityKind, k))
    {
      D_EntityKindFlags k_flags = d_entity_kind_flags_table[k];
      if(!(k_flags & D_EntityKindFlag_IsSerializedToConfig))
      {
        continue;
      }
      B32 first = 1;
      D_EntityList entities = d_query_cached_entity_list_with_kind(k);
      for(D_EntityNode *n = entities.first; n != 0; n = n->next)
      {
        D_Entity *entity = n->entity;
        if(entity->cfg_src != source)
        {
          continue;
        }
        if(first)
        {
          first = 0;
          String8 title_name = d_entity_kind_name_lower_plural_table[k];
          str8_list_pushf(arena, &strs, "/// %S %.*s\n\n",
                          title_name,
                          (int)Max(0, 79 - (title_name.size + 5)),
                          slashes);
        }
        D_EntityRec rec = {0};
        S64 depth = 0;
        for(D_Entity *e = entity; !d_entity_is_nil(e); e = rec.next)
        {
          //- rjf: get next iteration
          rec = d_entity_rec_depth_first_pre(e, entity);
          
          //- rjf: unpack entity info
          typedef U32 EntityInfoFlags;
          enum
          {
            EntityInfoFlag_HasName     = (1<<0),
            EntityInfoFlag_HasDisabled = (1<<1),
            EntityInfoFlag_HasTxtPt    = (1<<2),
            EntityInfoFlag_HasVAddr    = (1<<3),
            EntityInfoFlag_HasColor    = (1<<4),
            EntityInfoFlag_HasChildren = (1<<5),
          };
          String8 entity_name_escaped = e->string;
          if(d_entity_kind_flags_table[e->kind] & D_EntityKindFlag_NameIsPath)
          {
            Temp scratch = scratch_begin(&arena, 1);
            String8 path_normalized = path_normalized_from_string(scratch.arena, e->string);
            entity_name_escaped = path_relative_dst_from_absolute_dst_src(arena, path_normalized, root_path);
            scratch_end(scratch);
          }
          else
          {
            entity_name_escaped = d_cfg_escaped_from_raw_string(arena, e->string);
          }
          EntityInfoFlags info_flags = 0;
          if(entity_name_escaped.size != 0)        { info_flags |= EntityInfoFlag_HasName; }
          if(!!e->disabled)                        { info_flags |= EntityInfoFlag_HasDisabled; }
          if(e->flags & D_EntityFlag_HasTextPoint) { info_flags |= EntityInfoFlag_HasTxtPt; }
          if(e->flags & D_EntityFlag_HasVAddr)     { info_flags |= EntityInfoFlag_HasVAddr; }
          if(e->flags & D_EntityFlag_HasColor)     { info_flags |= EntityInfoFlag_HasColor; }
          if(!d_entity_is_nil(e->first))           { info_flags |= EntityInfoFlag_HasChildren; }
          
          //- rjf: write entity info
          B32 opened_brace = 0;
          switch(info_flags)
          {
            //- rjf: default path -> entity has lots of stuff, so write all info generically
            default:
            {
              opened_brace = 1;
              
              // rjf: write entity title
              str8_list_pushf(arena, &strs, "%S:\n{\n", d_entity_kind_name_lower_table[e->kind]);
              
              // rjf: write this entity's info
              if(entity_name_escaped.size != 0)
              {
                str8_list_pushf(arena, &strs, "name: \"%S\"\n", entity_name_escaped);
              }
              if(e->disabled)
              {
                str8_list_pushf(arena, &strs, "disabled: 1\n");
              }
              if(e->flags & D_EntityFlag_HasColor)
              {
                Vec4F32 hsva = d_hsva_from_entity(e);
                Vec4F32 rgba = rgba_from_hsva(hsva);
                U32 rgba_hex = u32_from_rgba(rgba);
                str8_list_pushf(arena, &strs, "color: 0x%x\n", rgba_hex);
              }
              if(e->flags & D_EntityFlag_HasTextPoint)
              {
                str8_list_pushf(arena, &strs, "line: %I64d\n", e->text_point.line);
              }
              if(e->flags & D_EntityFlag_HasVAddr)
              {
                str8_list_pushf(arena, &strs, "vaddr: (0x%I64x)\n", e->vaddr);
              }
            }break;
            
            //- rjf: single-line fast-paths
            case EntityInfoFlag_HasName:
            {str8_list_pushf(arena, &strs, "%S: \"%S\"\n", d_entity_kind_name_lower_table[e->kind], entity_name_escaped);}break;
            case EntityInfoFlag_HasName|EntityInfoFlag_HasTxtPt:
            {str8_list_pushf(arena, &strs, "%S: (\"%S\":%I64d)\n", d_entity_kind_name_lower_table[e->kind], entity_name_escaped, e->text_point.line);}break;
            case EntityInfoFlag_HasVAddr:
            {str8_list_pushf(arena, &strs, "%S: (0x%I64x)\n", d_entity_kind_name_lower_table[e->kind], e->vaddr);}break;
            
            //- rjf: empty
            case 0:
            {}break;
          }
          
          // rjf: push
          depth += rec.push_count;
          
          // rjf: pop
          if(rec.push_count == 0)
          {
            for(S64 pop_idx = 0; pop_idx < rec.pop_count + opened_brace; pop_idx += 1)
            {
              if(depth > 0)
              {
                depth -= 1;
              }
              str8_list_pushf(arena, &strs, "}\n");
            }
          }
          
          // rjf: separate top-level entities with extra newline
          if(d_entity_is_nil(rec.next) && (rec.pop_count != 0 || n->next == 0))
          {
            str8_list_pushf(arena, &strs, "\n");
          }
        }
      }
    }
  }
  
  //- rjf: write exception code filters
  if(source == D_CfgSrc_Project)
  {
    str8_list_push(arena, &strs, str8_lit("/// exception code filters ////////////////////////////////////////////////////\n"));
    str8_list_push(arena, &strs, str8_lit("\n"));
    str8_list_push(arena, &strs, str8_lit("exception_code_filters:\n"));
    str8_list_push(arena, &strs, str8_lit("{\n"));
    for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)(CTRL_ExceptionCodeKind_Null+1);
        k < CTRL_ExceptionCodeKind_COUNT;
        k = (CTRL_ExceptionCodeKind)(k+1))
    {
      String8 name = ctrl_exception_code_kind_lowercase_code_string_table[k];
      B32 value = !!(d_state->ctrl_exception_code_filters[k/64] & (1ull<<(k%64)));
      str8_list_pushf(arena, &strs, "  %S: %i\n", name, value);
    }
    str8_list_push(arena, &strs, str8_lit("}\n\n"));
  }
  
  ProfEnd();
  return strs;
}

//- rjf: entity kind cache

internal D_EntityList
d_query_cached_entity_list_with_kind(D_EntityKind kind)
{
  ProfBeginFunction();
  D_EntityListCache *cache = &d_state->kind_caches[kind];
  
  // rjf: build cached list if we're out-of-date
  if(cache->alloc_gen != d_state->kind_alloc_gens[kind])
  {
    cache->alloc_gen = d_state->kind_alloc_gens[kind];
    if(cache->arena == 0)
    {
      cache->arena = arena_alloc();
    }
    arena_clear(cache->arena);
    cache->list = d_push_entity_list_with_kind(cache->arena, kind);
  }
  
  // rjf: grab & return cached list
  D_EntityList result = cache->list;
  ProfEnd();
  return result;
}

//- rjf: active entity based queries

internal DI_KeyList
d_push_active_dbgi_key_list(Arena *arena)
{
  DI_KeyList dbgis = {0};
  D_EntityList modules = d_query_cached_entity_list_with_kind(D_EntityKind_Module);
  for(D_EntityNode *n = modules.first; n != 0; n = n->next)
  {
    D_Entity *module = n->entity;
    DI_Key key = d_dbgi_key_from_module(module);
    di_key_list_push(arena, &dbgis, &key);
  }
  return dbgis;
}

internal D_EntityList
d_push_active_target_list(Arena *arena)
{
  D_EntityList active_targets = {0};
  D_EntityList all_targets = d_query_cached_entity_list_with_kind(D_EntityKind_Target);
  for(D_EntityNode *n = all_targets.first; n != 0; n = n->next)
  {
    if(!n->entity->disabled)
    {
      d_entity_list_push(arena, &active_targets, n->entity);
    }
  }
  return active_targets;
}

//- rjf: expand key based entity queries

internal D_Entity *
d_entity_from_ev_key_and_kind(EV_Key key, D_EntityKind kind)
{
  D_Entity *result = &d_nil_entity;
  D_EntityList list = d_query_cached_entity_list_with_kind(kind);
  for(D_EntityNode *n = list.first; n != 0; n = n->next)
  {
    D_Entity *entity = n->entity;
    if(ev_key_match(d_ev_key_from_entity(entity), key))
    {
      result = entity;
      break;
    }
  }
  return result;
}

//- rjf: per-run caches

internal CTRL_Unwind
d_query_cached_unwind_from_thread(D_Entity *thread)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_Unwind result = {0};
  if(thread->kind == D_EntityKind_Thread)
  {
    U64 reg_gen = ctrl_reg_gen();
    U64 mem_gen = ctrl_mem_gen();
    D_UnwindCache *cache = &d_state->unwind_cache;
    D_Handle handle = d_handle_from_entity(thread);
    U64 hash = d_hash_from_string(str8_struct(&handle));
    U64 slot_idx = hash%cache->slots_count;
    D_UnwindCacheSlot *slot = &cache->slots[slot_idx];
    D_UnwindCacheNode *node = 0;
    for(D_UnwindCacheNode *n = slot->first; n != 0; n = n->next)
    {
      if(d_handle_match(handle, n->thread))
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = cache->free_node;
      if(node != 0)
      {
        SLLStackPop(cache->free_node);
      }
      else
      {
        node = push_array_no_zero(d_state->arena, D_UnwindCacheNode, 1);
      }
      MemoryZeroStruct(node);
      DLLPushBack(slot->first, slot->last, node);
      node->arena = arena_alloc();
      node->thread = handle;
    }
    if(node->reggen != reg_gen ||
       node->memgen != mem_gen)
    {
      CTRL_Unwind new_unwind = ctrl_unwind_from_thread(scratch.arena, d_state->ctrl_entity_store, thread->ctrl_handle, os_now_microseconds()+100);
      if(!(new_unwind.flags & (CTRL_UnwindFlag_Error|CTRL_UnwindFlag_Stale)) && new_unwind.frames.count != 0)
      {
        node->unwind = ctrl_unwind_deep_copy(node->arena, thread->arch, &new_unwind);
        node->reggen = reg_gen;
        node->memgen = mem_gen;
      }
    }
    result = node->unwind;
  }
  scratch_end(scratch);
  return result;
}

internal U64
d_query_cached_rip_from_thread(D_Entity *thread)
{
  U64 result = d_query_cached_rip_from_thread_unwind(thread, 0);
  return result;
}

internal U64
d_query_cached_rip_from_thread_unwind(D_Entity *thread, U64 unwind_count)
{
  U64 result = 0;
  if(unwind_count == 0)
  {
    result = ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, thread->ctrl_handle);
  }
  else
  {
    CTRL_Unwind unwind = d_query_cached_unwind_from_thread(thread);
    if(unwind.frames.count != 0)
    {
      result = regs_rip_from_arch_block(thread->arch, unwind.frames.v[unwind_count%unwind.frames.count].regs);
    }
  }
  return result;
}

internal U64
d_query_cached_tls_base_vaddr_from_process_root_rip(D_Entity *process, U64 root_vaddr, U64 rip_vaddr)
{
  U64 result = 0;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(d_state->tls_base_caches); cache_idx += 1)
  {
    D_RunTLSBaseCache *cache = &d_state->tls_base_caches[(d_state->tls_base_cache_gen+cache_idx)%ArrayCount(d_state->tls_base_caches)];
    if(cache_idx == 0 && cache->slots_count == 0)
    {
      cache->slots_count = 256;
      cache->slots = push_array(cache->arena, D_RunTLSBaseCacheSlot, cache->slots_count);
    }
    else if(cache->slots_count == 0)
    {
      break;
    }
    D_Handle handle = d_handle_from_entity(process);
    U64 hash = d_hash_from_seed_string(d_hash_from_string(str8_struct(&handle)), str8_struct(&rip_vaddr));
    U64 slot_idx = hash%cache->slots_count;
    D_RunTLSBaseCacheSlot *slot = &cache->slots[slot_idx];
    D_RunTLSBaseCacheNode *node = 0;
    for(D_RunTLSBaseCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(d_handle_match(n->process, handle) && n->root_vaddr == root_vaddr && n->rip_vaddr == rip_vaddr)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      U64 tls_base_vaddr = d_tls_base_vaddr_from_process_root_rip(process, root_vaddr, rip_vaddr);
      if(tls_base_vaddr != 0)
      {
        node = push_array(cache->arena, D_RunTLSBaseCacheNode, 1);
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
        node->process = handle;
        node->root_vaddr = root_vaddr;
        node->rip_vaddr = rip_vaddr;
        node->tls_base_vaddr = tls_base_vaddr;
      }
    }
    if(node != 0 && node->tls_base_vaddr != 0)
    {
      result = node->tls_base_vaddr;
      break;
    }
  }
  return result;
}

internal E_String2NumMap *
d_query_cached_locals_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  E_String2NumMap *map = &e_string2num_map_nil;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(d_state->locals_caches); cache_idx += 1)
  {
    D_RunLocalsCache *cache = &d_state->locals_caches[(d_state->locals_cache_gen+cache_idx)%ArrayCount(d_state->locals_caches)];
    if(cache_idx == 0 && cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, D_RunLocalsCacheSlot, cache->table_size);
    }
    else if(cache->table_size == 0)
    {
      break;
    }
    U64 hash = di_hash_from_key(dbgi_key);
    U64 slot_idx = hash % cache->table_size;
    D_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    D_RunLocalsCacheNode *node = 0;
    for(D_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(di_key_match(&n->dbgi_key, dbgi_key) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DI_Scope *scope = di_scope_open();
      E_String2NumMap *map = d_push_locals_map_from_dbgi_key_voff(cache->arena, scope, dbgi_key, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, D_RunLocalsCacheNode, 1);
        node->dbgi_key = di_key_copy(cache->arena, dbgi_key);
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      di_scope_close(scope);
    }
    if(node != 0 && node->locals_map->slots_count != 0)
    {
      map = node->locals_map;
      break;
    }
  }
  ProfEnd();
  return map;
}

internal E_String2NumMap *
d_query_cached_member_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  E_String2NumMap *map = &e_string2num_map_nil;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(d_state->member_caches); cache_idx += 1)
  {
    D_RunLocalsCache *cache = &d_state->member_caches[(d_state->member_cache_gen+cache_idx)%ArrayCount(d_state->member_caches)];
    if(cache_idx == 0 && cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, D_RunLocalsCacheSlot, cache->table_size);
    }
    else if(cache->table_size == 0)
    {
      break;
    }
    U64 hash = di_hash_from_key(dbgi_key);
    U64 slot_idx = hash % cache->table_size;
    D_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    D_RunLocalsCacheNode *node = 0;
    for(D_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(di_key_match(&n->dbgi_key, dbgi_key) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DI_Scope *scope = di_scope_open();
      E_String2NumMap *map = d_push_member_map_from_dbgi_key_voff(cache->arena, scope, dbgi_key, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, D_RunLocalsCacheNode, 1);
        node->dbgi_key = di_key_copy(cache->arena, dbgi_key);
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      di_scope_close(scope);
    }
    if(node != 0 && node->locals_map->slots_count != 0)
    {
      map = node->locals_map;
      break;
    }
  }
  ProfEnd();
  return map;
}

//- rjf: top-level command dispatch

internal void
d_push_cmd(D_CmdSpec *spec, D_CmdParams *params)
{
  d_cmd_list_push(d_state->cmds_arena, &d_state->cmds, params, spec);
}

//- rjf: command iteration

internal B32
d_next_cmd(D_Cmd **cmd)
{
  D_CmdNode *start_node = d_state->cmds.first;
  if(cmd[0] != 0)
  {
    start_node = CastFromMember(D_CmdNode, cmd, cmd[0]);
    start_node = start_node->next;
  }
  cmd[0] = 0;
  if(start_node != 0)
  {
    cmd[0] = &start_node->cmd;
  }
  return !!cmd[0];
}

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

#if !defined(BLAKE2_H)
#define HAVE_SSE2
#include "third_party/blake2/blake2.h"
#include "third_party/blake2/blake2b.c"
#endif

internal void
d_init(void)
{
  Arena *arena = arena_alloc();
  d_state = push_array(arena, D_State, 1);
  d_state->arena = arena;
  for(U64 idx = 0; idx < ArrayCount(d_state->frame_arenas); idx += 1)
  {
    d_state->frame_arenas[idx] = arena_alloc();
  }
  d_state->cmds_arena = arena_alloc();
  d_state->output_log_key = hs_hash_from_data(str8_lit("df_output_log_key"));
  d_state->entities_arena = arena_alloc(.reserve_size = GB(64), .commit_size = KB(64));
  d_state->entities_root = &d_nil_entity;
  d_state->entities_base = push_array(d_state->entities_arena, D_Entity, 0);
  d_state->entities_count = 0;
  d_state->ctrl_entity_store = ctrl_entity_store_alloc();
  d_state->ctrl_stop_arena = arena_alloc();
  d_state->entities_root = d_entity_alloc(&d_nil_entity, D_EntityKind_Root);
  d_state->cmd_spec_table_size = 1024;
  d_state->cmd_spec_table = push_array(arena, D_CmdSpec *, d_state->cmd_spec_table_size);
  d_state->view_rule_spec_table_size = 1024;
  d_state->view_rule_spec_table = push_array(arena, D_ViewRuleSpec *, d_state->view_rule_spec_table_size);
  d_state->top_regs = &d_state->base_regs;
  d_state->ctrl_msg_arena = arena_alloc();
  
  // rjf: set up initial exception filtering rules
  for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)0; k < CTRL_ExceptionCodeKind_COUNT; k = (CTRL_ExceptionCodeKind)(k+1))
  {
    if(ctrl_exception_code_kind_default_enable_table[k])
    {
      d_state->ctrl_exception_code_filters[k/64] |= 1ull<<(k%64);
    }
  }
  
  // rjf: set up initial entities
  {
    D_Entity *local_machine = d_entity_alloc(d_state->entities_root, D_EntityKind_Machine);
    d_entity_equip_ctrl_handle(local_machine, ctrl_handle_make(CTRL_MachineID_Local, dmn_handle_zero()));
    d_entity_equip_name(local_machine, str8_lit("This PC"));
  }
  
  // rjf: register core commands
  {
    D_CmdSpecInfoArray array = {d_core_cmd_kind_spec_info_table, ArrayCount(d_core_cmd_kind_spec_info_table)};
    d_register_cmd_specs(array);
  }
  
  // rjf: register core view rules
  {
    D_ViewRuleSpecInfoArray array = {d_core_view_rule_spec_info_table, ArrayCount(d_core_view_rule_spec_info_table)};
    d_register_view_rule_specs(array);
  }
  
  // rjf: set up caches
  d_state->unwind_cache.slots_count = 1024;
  d_state->unwind_cache.slots = push_array(arena, D_UnwindCacheSlot, d_state->unwind_cache.slots_count);
  for(U64 idx = 0; idx < ArrayCount(d_state->tls_base_caches); idx += 1)
  {
    d_state->tls_base_caches[idx].arena = arena_alloc();
  }
  for(U64 idx = 0; idx < ArrayCount(d_state->locals_caches); idx += 1)
  {
    d_state->locals_caches[idx].arena = arena_alloc();
  }
  for(U64 idx = 0; idx < ArrayCount(d_state->member_caches); idx += 1)
  {
    d_state->member_caches[idx].arena = arena_alloc();
  }
  
  // rjf: set up run state
  d_state->ctrl_last_run_arena = arena_alloc();
}

internal CTRL_EventList
d_tick(Arena *arena, D_TargetArray *targets, D_BreakpointArray *breakpoints)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_EventList result = {0};
  d_state->frame_index += 1;
  arena_clear(d_frame_arena());
  d_state->frame_eval_memread_endt_us = os_now_microseconds() + 5000;
  d_state->top_regs = &d_state->base_regs;
  d_regs_copy_contents(d_frame_arena(), &d_state->top_regs->v, &d_state->top_regs->v);
  
  //////////////////////////////
  //- rjf: sync with ctrl thread
  //
  ProfScope("sync with ctrl thread")
  {
    //- rjf: grab next reggen/memgen
    U64 new_mem_gen = ctrl_mem_gen();
    U64 new_reg_gen = ctrl_reg_gen();
    
    //- rjf: consume & process events
    result = ctrl_c2u_pop_events(arena);
    ctrl_entity_store_apply_events(d_state->ctrl_entity_store, &result);
    for(CTRL_EventNode *event_n = result.first;
        event_n != 0;
        event_n = event_n->next)
    {
      CTRL_Event *event = &event_n->v;
      log_infof("ctrl_event:\n{\n");
      log_infof("kind: \"%S\"\n", ctrl_string_from_event_kind(event->kind));
      log_infof("entity_id: %u\n", event->entity_id);
      switch(event->kind)
      {
        default:{}break;
        
        //- rjf: errors
        
        case CTRL_EventKind_Error:
        {
          log_user_error(event->string);
        }break;
        
        //- rjf: starts/stops
        
        case CTRL_EventKind_Started:
        {
          d_state->ctrl_is_running = 1;
        }break;
        
        case CTRL_EventKind_Stopped:
        {
          B32 should_snap = !(d_state->ctrl_soft_halt_issued);
          d_state->ctrl_is_running = 0;
          d_state->ctrl_soft_halt_issued = 0;
          D_Entity *stop_thread = d_entity_from_ctrl_handle(event->entity);
          
          // rjf: gather stop info
          {
            arena_clear(d_state->ctrl_stop_arena);
            MemoryCopyStruct(&d_state->ctrl_last_stop_event, event);
            d_state->ctrl_last_stop_event.string = push_str8_copy(d_state->ctrl_stop_arena, d_state->ctrl_last_stop_event.string);
          }
          
          // rjf: select & snap to thread causing stop
          if(should_snap && stop_thread->kind == D_EntityKind_Thread)
          {
            log_infof("stop_thread: \"%S\"\n", d_display_string_from_entity(scratch.arena, stop_thread));
            D_CmdParams params = d_cmd_params_zero();
            params.entity = d_handle_from_entity(stop_thread);
            d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_SelectThread), &params);
          }
          
          // rjf: if no stop-causing thread, and if selected thread, snap to selected
          if(should_snap && d_entity_is_nil(stop_thread))
          {
            D_Entity *selected_thread = d_entity_from_handle(d_regs()->thread);
            if(!d_entity_is_nil(selected_thread))
            {
              D_CmdParams params = d_cmd_params_zero();
              params.entity = d_handle_from_entity(selected_thread);
              df_push_cmd(d_cmd_spec_from_kind(DF_CmdKind_FindThread), &params);
            }
          }
          
          // rjf: thread hit user breakpoint -> increment breakpoint hit count
          if(should_snap && event->cause == CTRL_EventCause_UserBreakpoint)
          {
            U64 stop_thread_vaddr = ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, stop_thread->ctrl_handle);
            D_Entity *process = d_entity_ancestor_from_kind(stop_thread, D_EntityKind_Process);
            D_Entity *module = d_module_from_process_vaddr(process, stop_thread_vaddr);
            DI_Key dbgi_key = d_dbgi_key_from_module(module);
            U64 stop_thread_voff = d_voff_from_vaddr(module, stop_thread_vaddr);
            D_EntityList user_bps = d_query_cached_entity_list_with_kind(D_EntityKind_Breakpoint);
            for(D_EntityNode *n = user_bps.first; n != 0; n = n->next)
            {
              D_Entity *bp = n->entity;
              D_Entity *loc = d_entity_child_from_kind(bp, D_EntityKind_Location);
              D_LineList loc_lines = d_lines_from_file_path_line_num(scratch.arena, loc->string, loc->text_point.line);
              if(loc_lines.first != 0)
              {
                for(D_LineNode *n = loc_lines.first; n != 0; n = n->next)
                {
                  if(contains_1u64(n->v.voff_range, stop_thread_voff))
                  {
                    bp->u64 += 1;
                    break;
                  }
                }
              }
              else if(loc->flags & D_EntityFlag_HasVAddr && stop_thread_vaddr == loc->vaddr)
              {
                bp->u64 += 1;
              }
              else if(loc->string.size != 0)
              {
                U64 symb_voff = d_voff_from_dbgi_key_symbol_name(&dbgi_key, loc->string);
                if(symb_voff == stop_thread_voff)
                {
                  bp->u64 += 1;
                }
              }
            }
          }
          
          // rjf: exception or unexpected trap -> push error
          if(event->cause == CTRL_EventCause_InterruptedByException ||
             event->cause == CTRL_EventCause_InterruptedByTrap)
          {
            log_user_error(str8_zero());
          }
          
          // rjf: kill all entities which are marked to die on stop
          {
            D_Entity *request = d_entity_from_id(event->msg_id);
            if(d_entity_is_nil(request))
            {
              for(D_Entity *entity = d_entity_root();
                  !d_entity_is_nil(entity);
                  entity = d_entity_rec_depth_first_pre(entity, d_entity_root()).next)
              {
                if(entity->flags & D_EntityFlag_DiesOnRunStop)
                {
                  d_entity_mark_for_deletion(entity);
                }
              }
            }
          }
        }break;
        
        //- rjf: entity creation/deletion
        
        case CTRL_EventKind_NewProc:
        {
          // rjf: the first process? -> clear session output & reset all bp hit counts
          D_EntityList existing_processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
          if(existing_processes.count == 0)
          {
            MTX_Op op = {r1u64(0, 0xffffffffffffffffull), str8_lit("[new session]\n")};
            mtx_push_op(d_state->output_log_key, op);
            D_EntityList bps = d_query_cached_entity_list_with_kind(D_EntityKind_Breakpoint);
            for(D_EntityNode *n = bps.first; n != 0; n = n->next)
            {
              n->entity->u64 = 0;
            }
          }
          
          // rjf: create entity
          D_Entity *machine = d_machine_entity_from_machine_id(event->entity.machine_id);
          D_Entity *entity = d_entity_alloc(machine, D_EntityKind_Process);
          d_entity_equip_u64(entity, event->msg_id);
          d_entity_equip_ctrl_handle(entity, event->entity);
          d_entity_equip_ctrl_id(entity, event->entity_id);
          d_entity_equip_arch(entity, event->arch);
        }break;
        
        case CTRL_EventKind_NewThread:
        {
          // rjf: create entity
          D_Entity *parent = d_entity_from_ctrl_handle(event->parent);
          D_Entity *entity = d_entity_alloc(parent, D_EntityKind_Thread);
          d_entity_equip_ctrl_handle(entity, event->entity);
          d_entity_equip_arch(entity, event->arch);
          d_entity_equip_ctrl_id(entity, event->entity_id);
          d_entity_equip_stack_base(entity, event->stack_base);
          d_entity_equip_vaddr(entity, event->rip_vaddr);
          if(event->string.size != 0)
          {
            d_entity_equip_name(entity, event->string);
          }
          
          // rjf: find any pending thread names correllating with this TID -> equip name if found match
          {
            D_EntityList pending_thread_names = d_query_cached_entity_list_with_kind(D_EntityKind_PendingThreadName);
            for(D_EntityNode *n = pending_thread_names.first; n != 0; n = n->next)
            {
              D_Entity *pending_thread_name = n->entity;
              if(event->entity.machine_id == pending_thread_name->ctrl_handle.machine_id && event->entity_id == pending_thread_name->ctrl_id)
              {
                d_entity_mark_for_deletion(pending_thread_name);
                d_entity_equip_name(entity, pending_thread_name->string);
                break;
              }
            }
          }
          
          // rjf: determine index in process
          U64 thread_idx_in_process = 0;
          for(D_Entity *child = parent->first; !d_entity_is_nil(child); child = child->next)
          {
            if(child == entity)
            {
              break;
            }
            if(child->kind == D_EntityKind_Thread)
            {
              thread_idx_in_process += 1;
            }
          }
          
          // rjf: build default thread color table
          Vec4F32 thread_colors[] =
          {
            df_rgba_from_theme_color(DF_ThemeColor_Thread0),
            df_rgba_from_theme_color(DF_ThemeColor_Thread1),
            df_rgba_from_theme_color(DF_ThemeColor_Thread2),
            df_rgba_from_theme_color(DF_ThemeColor_Thread3),
            df_rgba_from_theme_color(DF_ThemeColor_Thread4),
            df_rgba_from_theme_color(DF_ThemeColor_Thread5),
            df_rgba_from_theme_color(DF_ThemeColor_Thread6),
            df_rgba_from_theme_color(DF_ThemeColor_Thread7),
          };
          
          // rjf: pick color
          Vec4F32 thread_color = thread_colors[thread_idx_in_process % ArrayCount(thread_colors)];
          
          // rjf: equip color
          d_entity_equip_color_rgba(entity, thread_color);
          
          // rjf: automatically select if we don't have a selected thread
          D_Entity *selected_thread = d_entity_from_handle(d_state->base_regs.v.thread);
          if(d_entity_is_nil(selected_thread))
          {
            d_state->base_regs.v.thread = d_handle_from_entity(entity);
          }
          
          // rjf: do initial snap
          D_EntityList already_existing_processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
          B32 do_initial_snap = (already_existing_processes.count == 1 && thread_idx_in_process == 0);
          if(do_initial_snap)
          {
            D_CmdParams params = d_cmd_params_zero();
            params.entity = d_handle_from_entity(entity);
            d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_SelectThread), &params);
          }
        }break;
        
        case CTRL_EventKind_NewModule:
        {
          // rjf: grab process
          D_Entity *parent = d_entity_from_ctrl_handle(event->parent);
          
          // rjf: determine if this is the first module
          B32 is_first = 0;
          if(d_entity_is_nil(d_entity_child_from_kind(parent, D_EntityKind_Module)))
          {
            is_first = 1;
          }
          
          // rjf: create module entity
          D_Entity *module = d_entity_alloc(parent, D_EntityKind_Module);
          d_entity_equip_ctrl_handle(module, event->entity);
          d_entity_equip_arch(module, event->arch);
          d_entity_equip_name(module, event->string);
          d_entity_equip_vaddr_rng(module, event->vaddr_rng);
          d_entity_equip_vaddr(module, event->rip_vaddr);
          d_entity_equip_timestamp(module, event->timestamp);
          
          // rjf: is first -> find target, equip process & module & first thread with target color
          if(is_first)
          {
            D_EntityList targets = d_query_cached_entity_list_with_kind(D_EntityKind_Target);
            for(D_EntityNode *n = targets.first; n != 0; n = n->next)
            {
              D_Entity *target = n->entity;
              D_Entity *exe = d_entity_child_from_kind(target, D_EntityKind_Executable);
              String8 exe_name = exe->string;
              String8 exe_name_normalized = path_normalized_from_string(scratch.arena, exe_name);
              String8 module_name_normalized = path_normalized_from_string(scratch.arena, module->string);
              if(str8_match(exe_name_normalized, module_name_normalized, StringMatchFlag_CaseInsensitive) &&
                 target->flags & D_EntityFlag_HasColor)
              {
                D_Entity *first_thread = d_entity_child_from_kind(parent, D_EntityKind_Thread);
                Vec4F32 rgba = d_rgba_from_entity(target);
                d_entity_equip_color_rgba(parent, rgba);
                d_entity_equip_color_rgba(first_thread, rgba);
                d_entity_equip_color_rgba(module, rgba);
                break;
              }
            }
          }
        }break;
        
        case CTRL_EventKind_EndProc:
        {
          U32 pid = event->entity_id;
          D_Entity *process = d_entity_from_ctrl_handle(event->entity);
          d_entity_mark_for_deletion(process);
        }break;
        
        case CTRL_EventKind_EndThread:
        {
          D_Entity *thread = d_entity_from_ctrl_handle(event->entity);
          d_entity_mark_for_deletion(thread);
        }break;
        
        case CTRL_EventKind_EndModule:
        {
          D_Entity *module = d_entity_from_ctrl_handle(event->entity);
          d_entity_mark_for_deletion(module);
        }break;
        
        //- rjf: debug info changes
        
        case CTRL_EventKind_ModuleDebugInfoPathChange:
        {
          D_Entity *module = d_entity_from_ctrl_handle(event->entity);
          D_Entity *debug_info = d_entity_child_from_kind(module, D_EntityKind_DebugInfoPath);
          if(d_entity_is_nil(debug_info))
          {
            debug_info = d_entity_alloc(module, D_EntityKind_DebugInfoPath);
          }
          d_entity_equip_name(debug_info, event->string);
          d_entity_equip_timestamp(debug_info, event->timestamp);
        }break;
        
        //- rjf: debug strings
        
        case CTRL_EventKind_DebugString:
        {
          MTX_Op op = {r1u64(max_U64, max_U64), event->string};
          mtx_push_op(d_state->output_log_key, op);
        }break;
        
        case CTRL_EventKind_ThreadName:
        {
          String8 string = event->string;
          D_Entity *entity = d_entity_from_ctrl_handle(event->entity);
          if(event->entity_id != 0)
          {
            entity = d_entity_from_ctrl_id(event->entity.machine_id, event->entity_id);
          }
          if(d_entity_is_nil(entity))
          {
            D_Entity *process = d_entity_from_ctrl_handle(event->parent);
            if(!d_entity_is_nil(process))
            {
              entity = d_entity_alloc(process, D_EntityKind_PendingThreadName);
              d_entity_equip_name(entity, string);
              d_entity_equip_ctrl_handle(entity, ctrl_handle_make(event->entity.machine_id, dmn_handle_zero()));
              d_entity_equip_ctrl_id(entity, event->entity_id);
            }
          }
          if(!d_entity_is_nil(entity))
          {
            d_entity_equip_name(entity, string);
          }
        }break;
        
        //- rjf: memory
        
        case CTRL_EventKind_MemReserve:{}break;
        case CTRL_EventKind_MemCommit:{}break;
        case CTRL_EventKind_MemDecommit:{}break;
        case CTRL_EventKind_MemRelease:{}break;
      }
      log_infof("}\n\n");
    }
    
    //- rjf: clear tls base cache
    if((d_state->tls_base_cache_reggen_idx != new_reg_gen ||
        d_state->tls_base_cache_memgen_idx != new_mem_gen) &&
       !d_ctrl_targets_running())
    {
      d_state->tls_base_cache_gen += 1;
      D_RunTLSBaseCache *cache = &d_state->tls_base_caches[d_state->tls_base_cache_gen%ArrayCount(d_state->tls_base_caches)];
      arena_clear(cache->arena);
      cache->slots_count = 0;
      cache->slots = 0;
      d_state->tls_base_cache_reggen_idx = new_reg_gen;
      d_state->tls_base_cache_memgen_idx = new_mem_gen;
    }
    
    //- rjf: clear locals cache
    if(d_state->locals_cache_reggen_idx != new_reg_gen &&
       !d_ctrl_targets_running())
    {
      d_state->locals_cache_gen += 1;
      D_RunLocalsCache *cache = &d_state->locals_caches[d_state->locals_cache_gen%ArrayCount(d_state->locals_caches)];
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      d_state->locals_cache_reggen_idx = new_reg_gen;
    }
    
    //- rjf: clear members cache
    if(d_state->member_cache_reggen_idx != new_reg_gen &&
       !d_ctrl_targets_running())
    {
      d_state->member_cache_gen += 1;
      D_RunLocalsCache *cache = &d_state->member_caches[d_state->member_cache_gen%ArrayCount(d_state->member_caches)];
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      d_state->member_cache_reggen_idx = new_reg_gen;
    }
  }
  
  //////////////////////////////
  //- rjf: hash ctrl parameterization state
  //
  U128 ctrl_param_state_hash = {0};
  {
    // rjf: build data strings of all param data
    String8List strings = {0};
    {
      CTRL_EntityList threads = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Thread);
      for(CTRL_EntityNode *n = threads.first; n != 0; n = n->next)
      {
        CTRL_Entity *thread = n->v;
        str8_list_push(scratch.arena, &strings, str8_struct(&thread->is_frozen));
      }
      for(U64 idx = 0; idx < breakpoints->count; idx += 1)
      {
        D_Breakpoint *bp = &breakpoints->v[idx];
        str8_list_push(scratch.arena, &strings, bp->file_path);
        str8_list_push(scratch.arena, &strings, str8_struct(&bp->pt));
        str8_list_push(scratch.arena, &strings, bp->symbol_name);
        str8_list_push(scratch.arena, &strings, str8_struct(&bp->vaddr));
        str8_list_push(scratch.arena, &strings, bp->condition);
      }
    }
    
    // rjf: join & hash to produce result
    String8 string = str8_list_join(scratch.arena, &strings, 0);
    blake2b((U8 *)&ctrl_param_state_hash.u64[0], sizeof(ctrl_param_state_hash), string.str, string.size, 0, 0);
  }
  
  //////////////////////////////
  //- rjf: if ctrl thread is running, and our ctrl parameterization
  // state hash has changed since the last run, we should soft-
  // halt-refresh to inform the ctrl thread about the updated
  // state
  //
  if(d_ctrl_targets_running() && !u128_match(ctrl_param_state_hash, d_state->ctrl_last_run_param_state_hash))
  {
    d_cmd(D_CmdKind_SoftHaltRefresh);
  }
  
  //////////////////////////////
  //- rjf: eliminate entities that are marked for deletion
  //
  ProfScope("eliminate deleted entities")
  {
    for(D_Entity *entity = d_entity_root(), *next = 0; !d_entity_is_nil(entity); entity = next)
    {
      next = d_entity_rec_depth_first_pre(entity, &d_nil_entity).next;
      if(entity->flags & D_EntityFlag_MarkedForDeletion)
      {
        B32 undoable = (d_entity_kind_flags_table[entity->kind] & D_EntityKindFlag_UserDefinedLifetime);
        
        // rjf: fixup next entity to iterate to
        next = d_entity_rec_depth_first(entity, &d_nil_entity, OffsetOf(D_Entity, next), OffsetOf(D_Entity, next)).next;
        
        // rjf: eliminate root entity if we're freeing it
        if(entity == d_state->entities_root)
        {
          d_state->entities_root = &d_nil_entity;
        }
        
        // rjf: unhook & release this entity tree
        d_entity_change_parent(entity, entity->parent, &d_nil_entity, &d_nil_entity);
        d_entity_release(entity);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: garbage collect eliminated thread unwinds
  //
  for(U64 slot_idx = 0; slot_idx < d_state->unwind_cache.slots_count; slot_idx += 1)
  {
    D_UnwindCacheSlot *slot = &d_state->unwind_cache.slots[slot_idx];
    for(D_UnwindCacheNode *n = slot->first, *next = 0; n != 0; n = next)
    {
      next = n->next;
      if(d_entity_is_nil(d_entity_from_handle(n->thread)))
      {
        DLLRemove(slot->first, slot->last, n);
        arena_release(n->arena);
        SLLStackPush(d_state->unwind_cache.free_node, n);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: sync with di parsers
  //
  ProfScope("sync with di parsers")
  {
    DI_EventList events = di_p2u_pop_events(scratch.arena, 0);
    for(DI_EventNode *n = events.first; n != 0; n = n->next)
    {
      DI_Event *event = &n->v;
      switch(event->kind)
      {
        default:{}break;
        case DI_EventKind_ConversionStarted:
        {
          D_Entity *task = d_entity_alloc(d_entity_root(), D_EntityKind_ConversionTask);
          d_entity_equip_name(task, event->string);
        }break;
        case DI_EventKind_ConversionEnded:
        {
          D_Entity *task = d_entity_from_name_and_kind(event->string, D_EntityKind_ConversionTask);
          if(!d_entity_is_nil(task))
          {
            d_entity_mark_for_deletion(task);
          }
        }break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: start/stop telemetry captures
  //
  ProfScope("start/stop telemetry captures")
  {
    if(!ProfIsCapturing() && DEV_telemetry_capture)
    {
      ProfBeginCapture("raddbg");
    }
    if(ProfIsCapturing() && !DEV_telemetry_capture)
    {
      ProfEndCapture();
    }
  }
  
  //////////////////////////////
  //- rjf: process top-level commands
  //
  CTRL_MsgList ctrl_msgs = {0};
  ProfScope("process top-level commands")
  {
    for(D_Cmd *cmd = 0; d_next_cmd(&cmd);)
    {
      // rjf: unpack command
      D_CmdParams params = cmd->params;
      D_CmdKind core_cmd_kind = d_cmd_kind_from_string(cmd->spec->info.string);
      d_cmd_spec_counter_inc(cmd->spec);
      
      // rjf: prep ctrl running arguments
      B32 need_run = 0;
      D_RunKind run_kind = D_RunKind_Run;
      CTRL_Entity *run_thread = &ctrl_entity_nil;
      CTRL_RunFlags run_flags = 0;
      CTRL_TrapList run_traps = {0};
      
      // rjf: process command
      switch(core_cmd_kind)
      {
        default:{}break;
        
        //- rjf: low-level target control operations
        case D_CmdKind_LaunchAndRun:
        case D_CmdKind_LaunchAndInit:
        {
          // rjf: get list of targets to launch
          D_EntityList targets = d_entity_list_from_handle_list(scratch.arena, params.entity_list);
          
          // rjf: no targets => assume all active targets
          if(targets.count == 0)
          {
            targets = d_push_active_target_list(scratch.arena);
          }
          
          // rjf: launch
          if(targets.count != 0)
          {
            for(D_EntityNode *n = targets.first; n != 0; n = n->next)
            {
              // rjf: extract data from target
              D_Entity *target = n->entity;
              String8 name = d_entity_child_from_kind(target, D_EntityKind_Executable)->string;
              String8 args = d_entity_child_from_kind(target, D_EntityKind_Arguments)->string;
              String8 path = d_entity_child_from_kind(target, D_EntityKind_WorkingDirectory)->string;
              String8 entry= d_entity_child_from_kind(target, D_EntityKind_EntryPoint)->string;
              name  = str8_skip_chop_whitespace(name);
              args  = str8_skip_chop_whitespace(args);
              path  = str8_skip_chop_whitespace(path);
              entry = str8_skip_chop_whitespace(entry);
              if(path.size == 0)
              {
                path = os_get_current_path(scratch.arena);
              }
              
              // rjf: build launch options
              String8List cmdln_strings = {0};
              {
                str8_list_push(scratch.arena, &cmdln_strings, name);
                {
                  U64 start_split_idx = 0;
                  B32 quoted = 0;
                  for(U64 idx = 0; idx <= args.size; idx += 1)
                  {
                    U8 byte = idx < args.size ? args.str[idx] : 0;
                    if(byte == '"')
                    {
                      quoted ^= 1;
                    }
                    B32 splitter_found = (!quoted && (byte == 0 || char_is_space(byte)));
                    if(splitter_found)
                    {
                      String8 string = str8_substr(args, r1u64(start_split_idx, idx));
                      if(string.size > 0)
                      {
                        str8_list_push(scratch.arena, &cmdln_strings, string);
                      }
                      start_split_idx = idx+1;
                    }
                  }
                }
              }
              
              // rjf: push message to launch
              {
                CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
                msg->kind = CTRL_MsgKind_Launch;
                msg->path = path;
                msg->cmd_line_string_list = cmdln_strings;
                msg->env_inherit = 1;
                MemoryCopyArray(msg->exception_code_filters, d_state->ctrl_exception_code_filters);
                str8_list_push(scratch.arena, &msg->entry_points, entry);
              }
            }
            
            // rjf: run
            need_run = 1;
            run_kind = D_RunKind_Run;
            run_thread = &ctrl_entity_nil;
            run_flags = (core_cmd_kind == D_CmdKind_LaunchAndInit) ? CTRL_RunFlag_StopOnEntryPoint : 0;
          }
          
          // rjf: no targets -> error
          if(targets.count == 0)
          {
            log_user_error(str8_lit("No active targets exist; cannot launch."));
          }
        }break;
        case D_CmdKind_Kill:
        {
          D_EntityList processes = d_entity_list_from_handle_list(scratch.arena, params.entity_list);
          
          // rjf: no processes => kill everything
          if(processes.count == 0)
          {
            processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
          }
          
          // rjf: kill processes
          if(processes.count != 0)
          {
            for(D_EntityNode *n = processes.first; n != 0; n = n->next)
            {
              D_Entity *process = n->entity;
              CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
              msg->kind = CTRL_MsgKind_Kill;
              msg->exit_code = 1;
              msg->entity = process->ctrl_handle;
              MemoryCopyArray(msg->exception_code_filters, d_state->ctrl_exception_code_filters);
            }
          }
          
          // rjf: no processes -> error
          if(processes.count == 0)
          {
            log_user_error(str8_lit("No attached running processes exist; cannot kill."));
          }
        }break;
        case D_CmdKind_KillAll:
        {
          D_EntityList processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
          if(processes.count != 0)
          {
            for(D_EntityNode *n = processes.first; n != 0; n = n->next)
            {
              D_Entity *process = n->entity;
              CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
              msg->kind = CTRL_MsgKind_Kill;
              msg->exit_code = 1;
              msg->entity = process->ctrl_handle;
              MemoryCopyArray(msg->exception_code_filters, d_state->ctrl_exception_code_filters);
            }
          }
          if(processes.count == 0)
          {
            log_user_error(str8_lit("No attached running processes exist; cannot kill."));
          }
        }break;
        case D_CmdKind_Detach:
        {
          for(D_HandleNode *n = params.entity_list.first; n != 0; n = n->next)
          {
            D_Entity *entity = d_entity_from_handle(n->handle);
            if(entity->kind == D_EntityKind_Process)
            {
              CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
              msg->kind   = CTRL_MsgKind_Detach;
              msg->entity = entity->ctrl_handle;
              MemoryCopyArray(msg->exception_code_filters, d_state->ctrl_exception_code_filters);
            }
          }
        }break;
        case D_CmdKind_Continue:
        {
          B32 good_to_run = 0;
          CTRL_EntityList threads = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Thread);
          for(CTRL_EntityNode *n = threads.first; n != 0; n = n->next)
          {
            if(!n->v->is_frozen)
            {
              good_to_run = 1;
              break;
            }
          }
          if(good_to_run)
          {
            need_run = 1;
            run_kind = D_RunKind_Run;
            run_thread = &ctrl_entity_nil;
          }
          else
          {
            log_user_error(str8_lit("Cannot run with all threads frozen."));
          }
        }break;
        case D_CmdKind_StepIntoInst:
        case D_CmdKind_StepOverInst:
        case D_CmdKind_StepIntoLine:
        case D_CmdKind_StepOverLine:
        case D_CmdKind_StepOut:
        {
          D_Entity *d_thread = d_entity_from_handle(params.entity);
          CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, d_thread->ctrl_handle);
          if(d_ctrl_targets_running())
          {
            if(d_ctrl_last_run_kind() == D_RunKind_Run)
            {
              log_user_error(str8_lit("Must halt before stepping."));
            }
          }
          else if(thread->is_frozen)
          {
            log_user_error(str8_lit("Must thaw selected thread before stepping."));
          }
          else
          {
            B32 good = 1;
            CTRL_TrapList traps = {0};
            switch(core_cmd_kind)
            {
              default: break;
              case D_CmdKind_StepIntoInst: {}break;
              case D_CmdKind_StepOverInst: {traps = d_trap_net_from_thread__step_over_inst(scratch.arena, thread);}break;
              case D_CmdKind_StepIntoLine: {traps = d_trap_net_from_thread__step_into_line(scratch.arena, thread);}break;
              case D_CmdKind_StepOverLine: {traps = d_trap_net_from_thread__step_over_line(scratch.arena, thread);}break;
              case D_CmdKind_StepOut:
              {
                // rjf: thread => full unwind
                CTRL_Unwind unwind = ctrl_unwind_from_thread(scratch.arena, d_state->ctrl_entity_store, thread->handle, os_now_microseconds()+10000);
                
                // rjf: use first unwind frame to generate trap
                if(unwind.flags == 0 && unwind.frames.count > 1)
                {
                  U64 vaddr = regs_rip_from_arch_block(thread->arch, unwind.frames.v[1].regs);
                  CTRL_Trap trap = {CTRL_TrapFlag_EndStepping|CTRL_TrapFlag_IgnoreStackPointerCheck, vaddr};
                  ctrl_trap_list_push(scratch.arena, &traps, &trap);
                }
                else
                {
                  log_user_error(str8_lit("Could not find the return address of the current callstack frame successfully."));
                  good = 0;
                }
              }break;
            }
            if(good && traps.count != 0)
            {
              need_run   = 1;
              run_kind   = D_RunKind_Step;
              run_thread = thread;
              run_flags  = 0;
              run_traps  = traps;
            }
            if(good && traps.count == 0)
            {
              need_run   = 1;
              run_kind   = D_RunKind_SingleStep;
              run_thread = thread;
              run_flags  = 0;
              run_traps  = traps;
            }
          }
        }break;
        case D_CmdKind_Halt:
        if(d_ctrl_targets_running())
        {
          ctrl_halt();
        }break;
        case D_CmdKind_SoftHaltRefresh:
        {
          if(d_ctrl_targets_running())
          {
            need_run   = 1;
            run_kind   = d_state->ctrl_last_run_kind;
            run_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, d_state->ctrl_last_run_thread_handle);
            run_flags  = d_state->ctrl_last_run_flags;
            run_traps  = d_state->ctrl_last_run_traps;
          }
        }break;
        case D_CmdKind_SetThreadIP:
        {
          D_Entity *thread = d_entity_from_handle(params.entity);
          U64 vaddr = params.vaddr;
          if(thread->kind == D_EntityKind_Thread && vaddr != 0)
          {
            void *block = ctrl_query_cached_reg_block_from_thread(scratch.arena, d_state->ctrl_entity_store, thread->ctrl_handle);
            regs_arch_block_write_rip(thread->arch, block, vaddr);
            B32 result = ctrl_thread_write_reg_block(thread->ctrl_handle, block);
            
            // rjf: early mutation of unwind cache for immediate frontend effect
            if(result)
            {
              D_UnwindCache *cache = &d_state->unwind_cache;
              if(cache->slots_count != 0)
              {
                D_Handle thread_handle = d_handle_from_entity(thread);
                U64 hash = d_hash_from_string(str8_struct(&thread_handle));
                U64 slot_idx = hash%cache->slots_count;
                D_UnwindCacheSlot *slot = &cache->slots[slot_idx];
                for(D_UnwindCacheNode *n = slot->first; n != 0; n = n->next)
                {
                  if(d_handle_match(n->thread, thread_handle) && n->unwind.frames.count != 0)
                  {
                    regs_arch_block_write_rip(thread->arch, n->unwind.frames.v[0].regs, vaddr);
                    break;
                  }
                }
              }
            }
          }
        }break;
        
        //- rjf: high-level composite target control operations
        case D_CmdKind_RunToLine:
        {
          String8 file_path = params.file_path;
          TxtPt point = params.text_point;
          D_Entity *bp = d_entity_alloc(d_entity_root(), D_EntityKind_Breakpoint);
          d_entity_equip_cfg_src(bp, D_CfgSrc_Transient);
          bp->flags |= D_EntityFlag_DiesOnRunStop;
          D_Entity *loc = d_entity_alloc(bp, D_EntityKind_Location);
          d_entity_equip_name(loc, file_path);
          d_entity_equip_txt_pt(loc, point);
          D_CmdParams p = d_cmd_params_zero();
          d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_Run), &p);
        }break;
        case D_CmdKind_RunToAddress:
        {
          D_Entity *bp = d_entity_alloc(d_entity_root(), D_EntityKind_Breakpoint);
          d_entity_equip_cfg_src(bp, D_CfgSrc_Transient);
          bp->flags |= D_EntityFlag_DiesOnRunStop;
          D_Entity *loc = d_entity_alloc(bp, D_EntityKind_Location);
          d_entity_equip_vaddr(loc, params.vaddr);
          D_CmdParams p = d_cmd_params_zero();
          d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_Run), &p);
        }break;
        case D_CmdKind_Run:
        {
          D_CmdParams params = d_cmd_params_zero();
          D_EntityList processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
          if(processes.count != 0)
          {
            D_CmdParams params = d_cmd_params_zero();
            d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_Continue), &params);
          }
          else if(!d_ctrl_targets_running())
          {
            d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_LaunchAndRun), &params);
          }
        }break;
        case D_CmdKind_Restart:
        {
          // rjf: kill all
          {
            D_CmdParams params = d_cmd_params_zero();
            d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_KillAll), &params);
          }
          
          // rjf: gather targets corresponding to all launched processes
          D_EntityList targets = {0};
          {
            D_EntityList processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
            for(D_EntityNode *n = processes.first; n != 0; n = n->next)
            {
              D_Entity *process = n->entity;
              D_Entity *target = d_entity_from_handle(process->entity_handle);
              if(!d_entity_is_nil(target))
              {
                d_entity_list_push(scratch.arena, &targets, target);
              }
            }
          }
          
          // rjf: re-launch targets
          {
            D_CmdParams params = d_cmd_params_zero();
            params.entity_list = d_handle_list_from_entity_list(scratch.arena, targets);
            d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_LaunchAndRun), &params);
          }
        }break;
        case D_CmdKind_StepInto:
        case D_CmdKind_StepOver:
        {
          D_EntityList processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
          if(processes.count != 0)
          {
            D_CmdKind step_cmd_kind = (core_cmd_kind == D_CmdKind_StepInto
                                       ? D_CmdKind_StepIntoLine
                                       : D_CmdKind_StepOverLine);
            B32 prefer_dasm = params.prefer_dasm;
            if(prefer_dasm)
            {
              step_cmd_kind = (core_cmd_kind == D_CmdKind_StepInto
                               ? D_CmdKind_StepIntoInst
                               : D_CmdKind_StepOverInst);
            }
            d_push_cmd(d_cmd_spec_from_kind(step_cmd_kind), &params);
          }
          else if(!d_ctrl_targets_running())
          {
            D_EntityList targets = d_push_active_target_list(scratch.arena);
            D_CmdParams p = params;
            p.entity_list = d_handle_list_from_entity_list(scratch.arena, targets);
            d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_LaunchAndInit), &p);
          }
        }break;
        case D_CmdKind_RunToCursor:
        {
          String8 file_path = d_regs()->file_path;
          if(file_path.size != 0)
          {
            d_cmd(D_CmdKind_RunToLine, .file_path = file_path, .text_point = d_regs()->cursor);
          }
          else
          {
            d_cmd(D_CmdKind_RunToAddress, .vaddr = d_regs()->vaddr_range.min);
          }
        }break;
        case D_CmdKind_SetNextStatement:
        {
          D_Entity *thread = d_entity_from_handle(d_regs()->thread);
          String8 file_path = d_regs()->file_path;
          U64 new_rip_vaddr = d_regs()->vaddr_range.min;
          if(file_path.size != 0)
          {
            D_LineList *lines = &d_regs()->lines;
            for(D_LineNode *n = lines->first; n != 0; n = n->next)
            {
              D_EntityList modules = d_modules_from_dbgi_key(scratch.arena, &n->v.dbgi_key);
              D_Entity *module = d_module_from_thread_candidates(thread, &modules);
              if(!d_entity_is_nil(module))
              {
                new_rip_vaddr = d_vaddr_from_voff(module, n->v.voff_range.min);
                break;
              }
            }
          }
          D_CmdParams p = d_cmd_params_zero();
          p.entity = d_handle_from_entity(thread);
          p.vaddr = new_rip_vaddr;
          d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_SetThreadIP), &p);
        }break;
        
        
        //- rjf: debug control context management operations
        case D_CmdKind_SelectThread:
        {
          D_Entity *thread = d_entity_from_handle(params.entity);
          D_Entity *module = d_module_from_thread(thread);
          D_Entity *process = d_entity_ancestor_from_kind(thread, D_EntityKind_Process);
          d_state->base_regs.v.unwind_count = 0;
          d_state->base_regs.v.inline_depth = 0;
          d_state->base_regs.v.thread = d_handle_from_entity(thread);
          d_state->base_regs.v.module = d_handle_from_entity(module);
          d_state->base_regs.v.process = d_handle_from_entity(process);
          df_push_cmd(df_cmd_spec_from_kind(DF_CmdKind_FindThread), &params);
        }break;
        case D_CmdKind_SelectUnwind:
        {
          DI_Scope *di_scope = di_scope_open();
          D_Entity *thread = d_entity_from_handle(d_state->base_regs.v.thread);
          D_Entity *process = d_entity_ancestor_from_kind(thread, D_EntityKind_Process);
          CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(thread);
          D_Unwind rich_unwind = d_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
          if(params.unwind_index < rich_unwind.frames.concrete_frame_count)
          {
            D_UnwindFrame *frame = &rich_unwind.frames.v[params.unwind_index];
            d_state->base_regs.v.unwind_count = params.unwind_index;
            d_state->base_regs.v.inline_depth = 0;
            if(params.inline_depth <= frame->inline_frame_count)
            {
              d_state->base_regs.v.inline_depth = params.inline_depth;
            }
          }
          df_push_cmd(df_cmd_spec_from_kind(DF_CmdKind_FindThread), &params);
          di_scope_close(di_scope);
        }break;
        case D_CmdKind_UpOneFrame:
        case D_CmdKind_DownOneFrame:
        {
          DI_Scope *di_scope = di_scope_open();
          D_Entity *thread = d_entity_from_handle(d_state->base_regs.v.thread);
          D_Entity *process = d_entity_ancestor_from_kind(thread, D_EntityKind_Process);
          CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(thread);
          D_Unwind rich_unwind = d_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
          U64 crnt_unwind_idx = d_state->base_regs.v.unwind_count;
          U64 crnt_inline_dpt = d_state->base_regs.v.inline_depth;
          U64 next_unwind_idx = crnt_unwind_idx;
          U64 next_inline_dpt = crnt_inline_dpt;
          if(crnt_unwind_idx < rich_unwind.frames.concrete_frame_count)
          {
            D_UnwindFrame *f = &rich_unwind.frames.v[crnt_unwind_idx];
            switch(core_cmd_kind)
            {
              default:{}break;
              case D_CmdKind_UpOneFrame:
              {
                if(crnt_inline_dpt < f->inline_frame_count)
                {
                  next_inline_dpt += 1;
                }
                else if(crnt_unwind_idx > 0)
                {
                  next_unwind_idx -= 1;
                  next_inline_dpt = 0;
                }
              }break;
              case D_CmdKind_DownOneFrame:
              {
                if(crnt_inline_dpt > 0)
                {
                  next_inline_dpt -= 1;
                }
                else if(crnt_unwind_idx < rich_unwind.frames.concrete_frame_count)
                {
                  next_unwind_idx += 1;
                  next_inline_dpt = (f+1)->inline_frame_count;
                }
              }break;
            }
          }
          D_CmdParams p = params;
          p.unwind_index = next_unwind_idx;
          p.inline_depth = next_inline_dpt;
          d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_SelectUnwind), &p);
          di_scope_close(di_scope);
        }break;
        case D_CmdKind_FreezeThread:
        case D_CmdKind_ThawThread:
        case D_CmdKind_FreezeProcess:
        case D_CmdKind_ThawProcess:
        case D_CmdKind_FreezeMachine:
        case D_CmdKind_ThawMachine:
        {
          D_CmdKind disptch_kind = ((core_cmd_kind == D_CmdKind_FreezeThread ||
                                     core_cmd_kind == D_CmdKind_FreezeProcess ||
                                     core_cmd_kind == D_CmdKind_FreezeMachine)
                                    ? D_CmdKind_FreezeEntity
                                    : D_CmdKind_ThawEntity);
          d_push_cmd(d_cmd_spec_from_kind(disptch_kind), &params);
        }break;
        case D_CmdKind_FreezeLocalMachine:
        {
          CTRL_MachineID machine_id = CTRL_MachineID_Local;
          D_CmdParams params = d_cmd_params_zero();
          params.entity = d_handle_from_entity(d_machine_entity_from_machine_id(machine_id));
          d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_FreezeMachine), &params);
        }break;
        case D_CmdKind_ThawLocalMachine:
        {
          CTRL_MachineID machine_id = CTRL_MachineID_Local;
          D_CmdParams params = d_cmd_params_zero();
          params.entity = d_handle_from_entity(d_machine_entity_from_machine_id(machine_id));
          d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_ThawMachine), &params);
        }break;
        case D_CmdKind_FreezeEntity:
        case D_CmdKind_ThawEntity:
        {
          B32 should_freeze = (core_cmd_kind == D_CmdKind_FreezeEntity);
          D_Entity *root = d_entity_from_handle(params.entity);
          for(D_Entity *e = root; !d_entity_is_nil(e); e = d_entity_rec_depth_first_pre(e, root).next)
          {
            if(e->kind == D_EntityKind_Thread)
            {
              CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, e->ctrl_handle);
              thread->is_frozen = should_freeze;
              CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
              msg->kind   = (should_freeze ? CTRL_MsgKind_FreezeThread : CTRL_MsgKind_ThawThread);
              msg->entity = thread->handle;
            }
          }
        }break;
        
        //- rjf: attaching
        case D_CmdKind_Attach:
        {
          U64 pid = params.id;
          if(pid != 0)
          {
            CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
            msg->kind      = CTRL_MsgKind_Attach;
            msg->entity_id = (U32)pid;
            MemoryCopyArray(msg->exception_code_filters, d_state->ctrl_exception_code_filters);
          }
        }break;
      }
      
      // rjf: do run if needed
      if(need_run)
      {
        // rjf: compute hash of all run-parameterization entities, store
        {
          d_state->ctrl_last_run_param_state_hash = ctrl_param_state_hash;
        }
        
        // rjf: push & fill run message
        CTRL_Msg *msg = ctrl_msg_list_push(scratch.arena, &ctrl_msgs);
        {
          CTRL_Entity *process = ctrl_entity_ancestor_from_kind(run_thread, CTRL_EntityKind_Process);
          msg->kind = (run_kind == D_RunKind_Run || run_kind == D_RunKind_Step) ? CTRL_MsgKind_Run : CTRL_MsgKind_SingleStep;
          msg->run_flags  = run_flags;
          msg->entity     = run_thread->handle;
          msg->parent     = process->handle;
          MemoryCopyArray(msg->exception_code_filters, d_state->ctrl_exception_code_filters);
          MemoryCopyStruct(&msg->traps, &run_traps);
          for(U64 idx = 0; idx < breakpoints->count; idx += 1)
          {
            // rjf: unpack user breakpoint entity
            D_Breakpoint *bp = &breakpoints->v[idx];
            
            // rjf: textual location -> add breakpoints for all possible override locations
            if(bp->file_path.size != 0 && bp->pt.line != 0)
            {
              String8List overrides = d_possible_overrides_from_file_path(scratch.arena, bp->file_path);
              for(String8Node *n = overrides.first; n != 0; n = n->next)
              {
                CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_FileNameAndLineColNumber};
                ctrl_user_bp.string    = n->string;
                ctrl_user_bp.pt        = bp->pt;
                ctrl_user_bp.condition = bp->condition;
                ctrl_user_breakpoint_list_push(scratch.arena, &msg->user_bps, &ctrl_user_bp);
              }
            }
            
            // rjf: virtual address location -> add breakpoint for address
            if(bp->vaddr != 0)
            {
              CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_VirtualAddress};
              ctrl_user_bp.u64       = bp->vaddr;
              ctrl_user_bp.condition = bp->condition;
              ctrl_user_breakpoint_list_push(scratch.arena, &msg->user_bps, &ctrl_user_bp);
            }
            
            // rjf: symbol name location -> add breakpoint for symbol name
            if(bp->symbol_name.size != 0)
            {
              CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_SymbolNameAndOffset};
              ctrl_user_bp.string    = bp->symbol_name;
              ctrl_user_bp.condition = bp->condition;
              ctrl_user_breakpoint_list_push(scratch.arena, &msg->user_bps, &ctrl_user_bp);
            }
          }
        }
        
        // rjf: copy run traps to scratch (needed, if run_traps can be `d_state->ctrl_last_run_traps`)
        CTRL_TrapList run_traps_copy = ctrl_trap_list_copy(scratch.arena, &run_traps);
        
        // rjf: store last run info
        arena_clear(d_state->ctrl_last_run_arena);
        d_state->ctrl_last_run_kind              = run_kind;
        d_state->ctrl_last_run_frame_idx         = d_frame_index();
        d_state->ctrl_last_run_thread_handle     = run_thread->handle;
        d_state->ctrl_last_run_flags             = run_flags;
        d_state->ctrl_last_run_traps             = ctrl_trap_list_copy(d_state->ctrl_last_run_arena, &run_traps_copy);
        d_state->ctrl_is_running                 = 1;
        
        // rjf: reset selected frame to top unwind
        d_state->base_regs.v.unwind_count = 0;
        d_state->base_regs.v.inline_depth = 0;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: clear command batch
  //
  {
    arena_clear(d_state->cmds_arena);
    MemoryZeroStruct(&d_state->cmds);
  }
  
  //////////////////////////////
  //- rjf: push new control messages to queue - try to send queue to control,
  // clear queue if successful (if not, we'll just keep them around until
  // the next tick)
  //
  {
    CTRL_MsgList msgs_copy = ctrl_msg_list_deep_copy(d_state->ctrl_msg_arena, &ctrl_msgs);
    ctrl_msg_list_concat_in_place(&d_state->ctrl_msgs, &msgs_copy);
    if(d_state->ctrl_msgs.count != 0)
    {
      if(ctrl_u2c_push_msgs(&d_state->ctrl_msgs, os_now_microseconds()+100))
      {
        MemoryZeroStruct(&d_state->ctrl_msgs);
        arena_clear(d_state->ctrl_msg_arena);
      }
    }
  }
  
  ProfEnd();
  scratch_end(scratch);
  return result;
}
