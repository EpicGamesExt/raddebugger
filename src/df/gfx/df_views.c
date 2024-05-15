// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Quick Sort Comparisons

internal int
df_qsort_compare_file_info__default(DF_FileInfo *a, DF_FileInfo *b)
{
  int result = 0;
  if(a->props.flags & FilePropertyFlag_IsFolder && !(b->props.flags & FilePropertyFlag_IsFolder))
  {
    result = -1;
  }
  else if(b->props.flags & FilePropertyFlag_IsFolder && !(a->props.flags & FilePropertyFlag_IsFolder))
  {
    result = +1;
  }
  else
  {
    result = df_qsort_compare_file_info__filename(a, b);
  }
  return result;
}

internal int
df_qsort_compare_file_info__default_filtered(DF_FileInfo *a, DF_FileInfo *b)
{
  int result = 0;
  if(a->filename.size < b->filename.size)
  {
    result = -1;
  }
  else if(a->filename.size > b->filename.size)
  {
    result = +1;
  }
  return result;
}

internal int
df_qsort_compare_file_info__filename(DF_FileInfo *a, DF_FileInfo *b)
{
  return strncmp((char *)a->filename.str, (char *)b->filename.str, Min(a->filename.size, b->filename.size));
}

internal int
df_qsort_compare_file_info__last_modified(DF_FileInfo *a, DF_FileInfo *b)
{
  return ((a->props.modified < b->props.modified) ? -1 :
          (a->props.modified > b->props.modified) ? +1 :
          0);
}

internal int
df_qsort_compare_file_info__size(DF_FileInfo *a, DF_FileInfo *b)
{
  return ((a->props.size < b->props.size) ? -1 :
          (a->props.size > b->props.size) ? +1 :
          0);
}

internal int
df_qsort_compare_process_info(DF_ProcessInfo *a, DF_ProcessInfo *b)
{
  int result = 0;
  if(a->pid_match_ranges.count > b->pid_match_ranges.count)
  {
    result = -1;
  }
  else if(a->pid_match_ranges.count < b->pid_match_ranges.count)
  {
    result = +1;
  }
  else if(a->name_match_ranges.count < b->name_match_ranges.count)
  {
    result = -1;
  }
  else if(a->name_match_ranges.count > b->name_match_ranges.count)
  {
    result = +1;
  }
  else if(a->attached_match_ranges.count < b->attached_match_ranges.count)
  {
    result = -1;
  }
  else if(a->attached_match_ranges.count > b->attached_match_ranges.count)
  {
    result = +1;
  }
  return result;
}

internal int
df_qsort_compare_cmd_lister__strength(DF_CmdListerItem *a, DF_CmdListerItem *b)
{
  int result = 0;
  if(a->name_match_ranges.count > b->name_match_ranges.count)
  {
    result = -1;
  }
  else if(a->name_match_ranges.count < b->name_match_ranges.count)
  {
    result = +1;
  }
  else if(a->desc_match_ranges.count > b->desc_match_ranges.count)
  {
    result = -1;
  }
  else if(a->desc_match_ranges.count < b->desc_match_ranges.count)
  {
    result = +1;
  }
  else if(a->tags_match_ranges.count > b->tags_match_ranges.count)
  {
    result = -1;
  }
  else if(a->tags_match_ranges.count < b->tags_match_ranges.count)
  {
    result = +1;
  }
  else if(a->registrar_idx < b->registrar_idx)
  {
    result = -1;
  }
  else if(a->registrar_idx > b->registrar_idx)
  {
    result = +1;
  }
  else if(a->ordering_idx < b->ordering_idx)
  {
    result = -1;
  }
  else if(a->ordering_idx > b->ordering_idx)
  {
    result = +1;
  }
  return result;
}

internal int
df_qsort_compare_entity_lister__strength(DF_EntityListerItem *a, DF_EntityListerItem *b)
{
  int result = 0;
  if(a->name_match_ranges.count > b->name_match_ranges.count)
  {
    result = -1;
  }
  else if(a->name_match_ranges.count < b->name_match_ranges.count)
  {
    result = +1;
  }
  return result;
}

////////////////////////////////
//~ rjf: Command Lister

internal DF_CmdListerItemList
df_cmd_lister_item_list_from_needle(Arena *arena, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  DF_CmdSpecList specs = df_push_cmd_spec_list(scratch.arena);
  DF_CmdListerItemList result = {0};
  for(DF_CmdSpecNode *n = specs.first; n != 0; n = n->next)
  {
    DF_CmdSpec *spec = n->spec;
    if(!(spec->info.flags & DF_CmdSpecFlag_OmitFromLists))
    {
      String8 cmd_display_name = spec->info.display_name;
      String8 cmd_desc = spec->info.description;
      String8 cmd_tags = spec->info.search_tags;
      FuzzyMatchRangeList name_matches = fuzzy_match_find(arena, needle, cmd_display_name);
      FuzzyMatchRangeList desc_matches = fuzzy_match_find(arena, needle, cmd_desc);
      FuzzyMatchRangeList tags_matches = fuzzy_match_find(arena, needle, cmd_tags);
      if(name_matches.count == name_matches.needle_part_count ||
         desc_matches.count == name_matches.needle_part_count ||
         tags_matches.count > 0 ||
         name_matches.needle_part_count == 0)
      {
        DF_CmdListerItemNode *node = push_array(arena, DF_CmdListerItemNode, 1);
        node->item.cmd_spec = spec;
        node->item.registrar_idx = spec->registrar_index;
        node->item.ordering_idx = spec->ordering_index;
        node->item.name_match_ranges = name_matches;
        node->item.desc_match_ranges = desc_matches;
        node->item.tags_match_ranges = tags_matches;
        SLLQueuePush(result.first, result.last, node);
        result.count += 1;
      }
    }
  }
  scratch_end(scratch);
  return result;
}

internal DF_CmdListerItemArray
df_cmd_lister_item_array_from_list(Arena *arena, DF_CmdListerItemList list)
{
  DF_CmdListerItemArray result = {0};
  result.count = list.count;
  result.v = push_array(arena, DF_CmdListerItem, result.count);
  U64 idx = 0;
  for(DF_CmdListerItemNode *n = list.first; n != 0; n = n->next, idx += 1)
  {
    result.v[idx] = n->item;
  }
  return result;
}

internal void
df_cmd_lister_item_array_sort_by_strength__in_place(DF_CmdListerItemArray array)
{
  qsort(array.v, array.count, sizeof(DF_CmdListerItem), (int (*)(const void *, const void *))df_qsort_compare_cmd_lister__strength);
}

////////////////////////////////
//~ rjf: System Process Lister

internal DF_ProcessInfoList
df_process_info_list_from_query(Arena *arena, String8 query)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: gather PIDs that we're currently attached to
  U64 attached_process_count = 0;
  U32 *attached_process_pids = 0;
  {
    DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
    attached_process_count = processes.count;
    attached_process_pids = push_array(scratch.arena, U32, attached_process_count);
    U64 idx = 0;
    for(DF_EntityNode *n = processes.first; n != 0; n = n->next, idx += 1)
    {
      DF_Entity *process = n->entity;
      attached_process_pids[idx] = process->ctrl_id;
    }
  }
  
  //- rjf: build list
  DF_ProcessInfoList list = {0};
  {
    DMN_ProcessIter iter = {0};
    dmn_process_iter_begin(&iter);
    for(DMN_ProcessInfo info = {0}; dmn_process_iter_next(scratch.arena, &iter, &info);)
    {
      // rjf: skip root-level or otherwise 0-pid processes
      if(info.pid == 0)
      {
        continue;
      }
      
      // rjf: determine if this process is attached
      B32 is_attached = 0;
      for(U64 attached_idx = 0; attached_idx < attached_process_count; attached_idx += 1)
      {
        if(attached_process_pids[attached_idx] == info.pid)
        {
          is_attached = 1;
          break;
        }
      }
      
      // rjf: gather fuzzy matches
      FuzzyMatchRangeList attached_match_ranges = {0};
      FuzzyMatchRangeList name_match_ranges     = fuzzy_match_find(arena, query, info.name);
      FuzzyMatchRangeList pid_match_ranges      = fuzzy_match_find(arena, query, push_str8f(scratch.arena, "%i", info.pid));
      if(is_attached)
      {
        attached_match_ranges = fuzzy_match_find(arena, query, str8_lit("[attached]"));
      }
      
      // rjf: determine if this item is filtered out
      B32 matches_query = (query.size == 0 ||
                           (attached_match_ranges.needle_part_count != 0 && attached_match_ranges.count >= attached_match_ranges.needle_part_count) ||
                           (name_match_ranges.count != 0 && name_match_ranges.count >= name_match_ranges.needle_part_count) ||
                           (pid_match_ranges.count != 0 && pid_match_ranges.count >= pid_match_ranges.needle_part_count));
      
      // rjf: push if unfiltered
      if(matches_query)
      {
        DF_ProcessInfoNode *n = push_array(arena, DF_ProcessInfoNode, 1);
        n->info.info = info;
        n->info.info.name = push_str8_copy(arena, info.name);
        n->info.is_attached = is_attached;
        n->info.attached_match_ranges = attached_match_ranges;
        n->info.name_match_ranges = name_match_ranges;
        n->info.pid_match_ranges = pid_match_ranges;
        SLLQueuePush(list.first, list.last, n);
        list.count += 1;
      }
    }
    dmn_process_iter_end(&iter);
  }
  
  scratch_end(scratch);
  return list;
}

internal DF_ProcessInfoArray
df_process_info_array_from_list(Arena *arena, DF_ProcessInfoList list)
{
  DF_ProcessInfoArray array = {0};
  array.count = list.count;
  array.v = push_array(arena, DF_ProcessInfo, array.count);
  U64 idx = 0;
  for(DF_ProcessInfoNode *n = list.first; n != 0; n = n->next, idx += 1)
  {
    array.v[idx] = n->info;
  }
  return array;
}

internal void
df_process_info_array_sort_by_strength__in_place(DF_ProcessInfoArray array)
{
  qsort(array.v, array.count, sizeof(DF_ProcessInfo), (int (*)(const void *, const void *))df_qsort_compare_process_info);
}

////////////////////////////////
//~ rjf: Entity Lister

internal DF_EntityListerItemList
df_entity_lister_item_list_from_needle(Arena *arena, DF_EntityKind kind, DF_EntityFlags omit_flags, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  DF_EntityListerItemList result = {0};
  DF_EntityList ent_list = df_query_cached_entity_list_with_kind(kind);
  for(DF_EntityNode *n = ent_list.first; n != 0; n = n->next)
  {
    DF_Entity *entity = n->entity;
    if(!(entity->flags & omit_flags))
    {
      String8 display_string = df_display_string_from_entity(scratch.arena, entity);
      FuzzyMatchRangeList match_rngs = fuzzy_match_find(arena, needle, display_string);
      if(match_rngs.count != 0 || needle.size == 0)
      {
        DF_EntityListerItemNode *item_n = push_array(arena, DF_EntityListerItemNode, 1);
        item_n->item.entity = entity;
        item_n->item.name_match_ranges = match_rngs;
        SLLQueuePush(result.first, result.last, item_n);
        result.count += 1;
      }
    }
  }
  scratch_end(scratch);
  return result;
}

internal DF_EntityListerItemArray
df_entity_lister_item_array_from_list(Arena *arena, DF_EntityListerItemList list)
{
  DF_EntityListerItemArray result = {0};
  result.count = list.count;
  result.v = push_array(arena, DF_EntityListerItem, result.count);
  {
    U64 idx = 0;
    for(DF_EntityListerItemNode *n = list.first; n != 0; n = n->next, idx += 1)
    {
      result.v[idx] = n->item;
    }
  }
  return result;
}

internal void
df_entity_lister_item_array_sort_by_strength__in_place(DF_EntityListerItemArray array)
{
  qsort(array.v, array.count, sizeof(DF_EntityListerItem), (int (*)(const void *, const void *))df_qsort_compare_entity_lister__strength);
}

////////////////////////////////
//~ rjf: Watch Views

//- rjf: eval watch view instance -> eval view key

internal DF_EvalViewKey
df_eval_view_key_from_eval_watch_view(DF_WatchViewState *ewv)
{
  DF_EvalViewKey key = df_eval_view_key_make((U64)ewv, df_hash_from_string(str8_struct(&ewv)));
  return key;
}

//- rjf: root allocation/deallocation/mutation

internal DF_EvalRoot *
df_eval_root_alloc(DF_View *view, DF_WatchViewState *ews)
{
  DF_EvalRoot *result = ews->first_free_root;
  if(result != 0)
  {
    SLLStackPop(ews->first_free_root);
    result->expr_buffer_string_size = 0;
  }
  else
  {
    result = push_array(view->arena, DF_EvalRoot, 1);
    result->expr_buffer_cap = 1024;
    result->expr_buffer = push_array_no_zero(view->arena, U8, result->expr_buffer_cap);
  }
  DLLPushBack(ews->first_root, ews->last_root, result);
  ews->root_count += 1;
  return result;
}

internal void
df_eval_root_release(DF_WatchViewState *ews, DF_EvalRoot *root)
{
  DLLRemove(ews->first_root, ews->last_root, root);
  SLLStackPush(ews->first_free_root, root);
  ews->root_count -= 1;
}

internal void
df_eval_root_equip_string(DF_EvalRoot *root, String8 string)
{
  root->expr_buffer_string_size = Min(string.size, root->expr_buffer_cap);
  MemoryCopy(root->expr_buffer, string.str, root->expr_buffer_string_size);
}

internal DF_EvalRoot *
df_eval_root_from_string(DF_WatchViewState *ews, String8 string)
{
  DF_EvalRoot *root = 0;
  for(DF_EvalRoot *r = ews->first_root; r != 0; r = r->next)
  {
    String8 r_string = df_string_from_eval_root(r);
    if(str8_match(r_string, string, 0))
    {
      root = r;
      break;
    }
  }
  return root;
}

internal DF_EvalRoot *
df_eval_root_from_expand_key(DF_WatchViewState *ews, DF_EvalView *eval_view, DF_ExpandKey expand_key)
{
  DF_EvalRoot *root = 0;
  for(DF_EvalRoot *r = ews->first_root; r != 0; r = r->next)
  {
    DF_ExpandKey key = df_expand_key_from_eval_root(r);
    if(df_expand_key_match(key, expand_key))
    {
      root = r;
      break;
    }
  }
  return root;
}

internal String8
df_string_from_eval_root(DF_EvalRoot *root)
{
  String8 string = str8(root->expr_buffer, root->expr_buffer_string_size);
  return string;
}

internal DF_ExpandKey
df_parent_expand_key_from_eval_root(DF_EvalRoot *root)
{
  DF_ExpandKey parent_key = df_expand_key_make(5381, (U64)root);
  return parent_key;
}

internal DF_ExpandKey
df_expand_key_from_eval_root(DF_EvalRoot *root)
{
  DF_ExpandKey parent_key = df_parent_expand_key_from_eval_root(root);
  DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(parent_key), (U64)root);
  return key;
}

//- rjf: watch view points <-> table coordinates

internal DF_WatchViewPoint
df_watch_view_point_from_tbl(DF_EvalVizBlockList *blocks, Vec2S64 tbl)
{
  DF_WatchViewPoint pt = zero_struct;
  pt.column_kind = (DF_WatchViewColumnKind)(tbl.x%DF_WatchViewColumnKind_COUNT);
  pt.key         = df_key_from_viz_block_list_row_num(blocks, tbl.y);
  pt.parent_key  = df_parent_key_from_viz_block_list_row_num(blocks, tbl.y);
  return pt;
}

internal Vec2S64
df_tbl_from_watch_view_point(DF_EvalVizBlockList *blocks, DF_WatchViewPoint pt)
{
  Vec2S64 tbl = {0};
  tbl.x = (S64)pt.column_kind;
  tbl.y = df_row_num_from_viz_block_list_key(blocks, pt.key);
  return tbl;
}

//- rjf: table coordinates -> strings

internal String8
df_string_from_eval_viz_row_column_kind(Arena *arena, DF_EvalView *ev, TG_Graph *graph, RDI_Parsed *rdi, DF_EvalVizRow *row, DF_WatchViewColumnKind col_kind, B32 editable)
{
  String8 result = {0};
  switch(col_kind)
  {
    default:{}break;
    case DF_WatchViewColumnKind_Expr:    {result = editable ? row->edit_expr : row->display_expr;}break;
    case DF_WatchViewColumnKind_Value:   {result = editable ? row->edit_value : row->display_value;}break;
    case DF_WatchViewColumnKind_Type:    {result = tg_string_from_key(arena, graph, rdi, row->eval.type_key);}break;
    case DF_WatchViewColumnKind_ViewRule:{result = df_eval_view_rule_from_key(ev, row->key);}break;
  }
  return result;
}

//- rjf: table coordinates -> text edit state

internal DF_WatchViewTextEditState *
df_watch_view_text_edit_state_from_pt(DF_WatchViewState *wv, DF_WatchViewPoint pt)
{
  DF_WatchViewTextEditState *result = &wv->dummy_text_edit_state;
  if(wv->text_edit_state_slots_count != 0 && wv->text_editing != 0)
  {
    U64 hash = df_hash_from_expand_key(pt.key);
    U64 slot_idx = hash%wv->text_edit_state_slots_count;
    for(DF_WatchViewTextEditState *s = wv->text_edit_state_slots[slot_idx]; s != 0; s = s->pt_hash_next)
    {
      if(pt.column_kind == s->pt.column_kind &&
         df_expand_key_match(pt.parent_key, s->pt.parent_key) &&
         df_expand_key_match(pt.key, s->pt.key))
      {
        result = s;
        break;
      }
    }
  }
  return result;
}

//- rjf: windowed watch tree visualization (both single-line and multi-line)

internal DF_EvalVizBlockList
df_eval_viz_block_list_from_watch_view_state(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_View *view, DF_WatchViewState *ews)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  DF_EvalVizBlockList blocks = {0};
  DF_EvalViewKey eval_view_key = df_eval_view_key_from_eval_watch_view(ews);
  DF_EvalView *eval_view = df_eval_view_from_key(eval_view_key);
  String8 filter = str8(view->query_buffer, view->query_string_size);
  DBGI_FuzzySearchTarget dbgi_target = DBGI_FuzzySearchTarget_UDTs;
  switch(ews->fill_kind)
  {
    ////////////////////////////
    //- rjf: mutable watch fill -> build blocks from top-level mutable root expressions
    //
    default:
    case DF_WatchViewFillKind_Mutable:
    {
      for(DF_EvalRoot *root = ews->first_root; root != 0; root = root->next)
      {
        String8 root_expr_string = df_string_from_eval_root(root);
        FuzzyMatchRangeList matches = fuzzy_match_find(arena, filter, root_expr_string);
        if(matches.count == matches.needle_part_count)
        {
          DF_ExpandKey parent_key = df_parent_expand_key_from_eval_root(root);
          DF_ExpandKey key = df_expand_key_from_eval_root(root);
          DF_EvalVizBlockList root_blocks = df_eval_viz_block_list_from_eval_view_expr_keys(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval_view, root_expr_string, parent_key, key);
          df_eval_viz_block_list_concat__in_place(&blocks, &root_blocks);
        }
      }
    }break;
    
    ////////////////////////////
    //- rjf: registers fill -> build blocks via iterating all registers/aliases as root-level expressions
    //
    case DF_WatchViewFillKind_Registers:
    {
      DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
      Architecture arch = df_architecture_from_entity(thread);
      U64 reg_count = regs_reg_code_count_from_architecture(arch);
      String8 *reg_strings = regs_reg_code_string_table_from_architecture(arch);
      U64 alias_count = regs_alias_code_count_from_architecture(arch);
      String8 *alias_strings = regs_alias_code_string_table_from_architecture(arch);
      U64 num = 1;
      for(U64 reg_idx = 1; reg_idx < reg_count; reg_idx += 1, num += 1)
      {
        String8 root_expr_string = reg_strings[reg_idx];
        FuzzyMatchRangeList matches = fuzzy_match_find(arena, filter, root_expr_string);
        if(matches.count == matches.needle_part_count)
        {
          DF_ExpandKey parent_key = df_expand_key_make(5381, 0);
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(parent_key), num);
          DF_EvalVizBlockList root_blocks = df_eval_viz_block_list_from_eval_view_expr_keys(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval_view, root_expr_string, parent_key, key);
          df_eval_viz_block_list_concat__in_place(&blocks, &root_blocks);
        }
      }
      for(U64 alias_idx = 1; alias_idx < alias_count; alias_idx += 1, num += 1)
      {
        String8 root_expr_string = alias_strings[alias_idx];
        FuzzyMatchRangeList matches = fuzzy_match_find(arena, filter, root_expr_string);
        if(matches.count == matches.needle_part_count)
        {
          DF_ExpandKey parent_key = df_expand_key_make(5381, 0);
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(parent_key), num);
          DF_EvalVizBlockList root_blocks = df_eval_viz_block_list_from_eval_view_expr_keys(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval_view, root_expr_string, parent_key, key);
          df_eval_viz_block_list_concat__in_place(&blocks, &root_blocks);
        }
      }
    }break;
    
    ////////////////////////////
    //- rjf: locals fill -> build blocks via iterating all locals as root-level expressions
    //
    case DF_WatchViewFillKind_Locals:
    {
      U64 num = 1;
      for(EVAL_String2NumMapNode *n = parse_ctx->locals_map->first; n != 0; n = n->order_next, num += 1)
      {
        String8 root_expr_string = n->string;
        FuzzyMatchRangeList matches = fuzzy_match_find(arena, filter, root_expr_string);
        if(matches.count == matches.needle_part_count)
        {
          DF_ExpandKey parent_key = df_expand_key_make(5381, 0);
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(parent_key), num);
          DF_EvalVizBlockList root_blocks = df_eval_viz_block_list_from_eval_view_expr_keys(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval_view, root_expr_string, parent_key, key);
          df_eval_viz_block_list_concat__in_place(&blocks, &root_blocks);
        }
      }
    }break;
    
    ////////////////////////////
    //- rjf: debug info table fill -> build split debug info table blocks
    //
    case DF_WatchViewFillKind_Globals:      dbgi_target = DBGI_FuzzySearchTarget_GlobalVariables; goto dbgi_table;
    case DF_WatchViewFillKind_ThreadLocals: dbgi_target = DBGI_FuzzySearchTarget_ThreadVariables; goto dbgi_table;
    case DF_WatchViewFillKind_Types:        dbgi_target = DBGI_FuzzySearchTarget_UDTs;            goto dbgi_table;
    case DF_WatchViewFillKind_Procedures:   dbgi_target = DBGI_FuzzySearchTarget_Procedures;      goto dbgi_table;
    dbgi_table:;
    {
      //- rjf: unpack context
      DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
      DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
      U64 thread_rip_unwind_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx->unwind_count);
      DF_Entity *module = df_module_from_process_vaddr(process, thread_rip_unwind_vaddr);
      DF_Entity *binary = df_binary_file_from_module(module);
      String8 exe_path = df_full_path_from_entity(scratch.arena, binary);
      DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, os_now_microseconds()+100);
      RDI_Parsed *rdi = &dbgi->rdi;
      
      //- rjf: calculate top-level keys, expand root-level, grab root expansion node
      DF_ExpandKey parent_key = df_expand_key_make(5381, 0);
      DF_ExpandKey root_key = df_expand_key_make(df_hash_from_expand_key(parent_key), 0);
      df_expand_set_expansion(eval_view->arena, &eval_view->expand_tree_table, df_expand_key_zero(), parent_key, 1);
      DF_ExpandNode *root_node = df_expand_node_from_key(&eval_view->expand_tree_table, parent_key);
      
      //- rjf: query all filtered items from dbgi searching system
      U128 fuzzy_search_key = {(U64)view, df_hash_from_string(str8_struct(&view))};
      B32 items_stale = 0;
      DBGI_FuzzySearchItemArray items = dbgi_fuzzy_search_items_from_key_exe_query(scope, fuzzy_search_key, exe_path, filter, dbgi_target, os_now_microseconds()+100, &items_stale);
      if(items_stale)
      {
        df_gfx_request_frame();
      }
      
      //- rjf: gather unsorted child expansion keys
      //
      // Nodes are sorted in the underlying expansion tree data structure, but
      // ONLY by THEIR ORDER IN THE UNDERLYING DEBUG INFO TABLE. This is
      // because debug info watch rows use the DEBUG INFO TABLE INDEX to form
      // their key - this provides more stable/predictable behavior as rows
      // are reordered, filtered, and shuffled around, as the user filters.
      //
      // When we actually build viz blocks, however, we want to produce viz
      // blocks BY THE ORDER OF SUB-EXPANSIONS IN THE FILTERED ITEM ARRAY
      // SPACE, so that all of the expansions come out in the right order.
      //
      DF_ExpandKey *sub_expand_keys = 0;
      U64 *sub_expand_item_idxs = 0;
      U64 sub_expand_keys_count = 0;
      {
        for(DF_ExpandNode *child = root_node->first; child != 0; child = child->next)
        {
          sub_expand_keys_count += 1;
        }
        sub_expand_keys = push_array(scratch.arena, DF_ExpandKey, sub_expand_keys_count);
        sub_expand_item_idxs = push_array(scratch.arena, U64, sub_expand_keys_count);
        U64 idx = 0;
        for(DF_ExpandNode *child = root_node->first; child != 0; child = child->next)
        {
          U64 item_num = dbgi_fuzzy_item_num_from_array_element_idx__linear_search(&items, child->key.child_num);
          if(item_num != 0)
          {
            sub_expand_keys[idx] = child->key;
            sub_expand_item_idxs[idx] = item_num-1;
            idx += 1;
          }
          else
          {
            sub_expand_keys_count -= 1;
          }
        }
      }
      
      //- rjf: sort child expansion keys
      {
        for(U64 idx1 = 0; idx1 < sub_expand_keys_count; idx1 += 1)
        {
          U64 min_idx2 = 0;
          U64 min_item_idx = sub_expand_item_idxs[idx1];
          for(U64 idx2 = idx1+1; idx2 < sub_expand_keys_count; idx2 += 1)
          {
            if(sub_expand_item_idxs[idx2] < min_item_idx)
            {
              min_idx2 = idx2;
              min_item_idx = sub_expand_item_idxs[idx2];
            }
          }
          if(min_idx2 != 0)
          {
            Swap(DF_ExpandKey, sub_expand_keys[idx1], sub_expand_keys[min_idx2]);
            Swap(U64, sub_expand_item_idxs[idx1], sub_expand_item_idxs[min_idx2]);
          }
        }
      }
      
      //- rjf: build blocks for all table items, split by sorted sub-expansions
      DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_DebugInfoTable, parent_key, root_key, 0);
      {
        last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, items.count);
        last_vb->dbgi_target = dbgi_target;
        last_vb->backing_search_items = items;
      }
      for(U64 sub_expand_idx = 0; sub_expand_idx < sub_expand_keys_count; sub_expand_idx += 1)
      {
        // rjf: form split: truncate & complete last block; begin next block
        last_vb = df_eval_viz_block_split_and_continue(arena, &blocks, last_vb, sub_expand_item_idxs[sub_expand_idx]);
        
        // rjf: grab name for the expanded row
        String8 name = dbgi_fuzzy_item_string_from_rdi_target_element_idx(&dbgi->rdi, dbgi_target, sub_expand_keys[sub_expand_idx].child_num);
        
        // rjf: recurse for sub-expansion
        {
          DF_CfgTable child_cfg = {0};
          {
            String8 view_rule_string = df_eval_view_rule_from_key(eval_view, df_expand_key_make(df_hash_from_expand_key(parent_key), sub_expand_keys[sub_expand_idx].child_num));
            if(view_rule_string.size != 0)
            {
              df_cfg_table_push_unparsed_string(arena, &child_cfg, view_rule_string, DF_CfgSrc_User);
            }
          }
          DF_Eval eval = df_eval_from_string(arena, scope, ctrl_ctx, parse_ctx, macro_map, name);
          df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, macro_map, parent_key, sub_expand_keys[sub_expand_idx], name, eval, 0, &child_cfg, 0, &blocks);
        }
      }
      df_eval_viz_block_end(&blocks, last_vb);
    }break;
  }
  scratch_end(scratch);
  ProfEnd();
  return blocks;
}

//- rjf: eval/watch views main hooks

internal void
df_watch_view_init(DF_WatchViewState *ewv, DF_View *view, DF_WatchViewFillKind fill_kind)
{
  if(ewv->initialized == 0)
  {
    ewv->initialized = 1;
    ewv->expr_column_pct      = 0.25f;
    ewv->value_column_pct     = 0.3f;
    ewv->type_column_pct      = 0.15f;
    ewv->view_rule_column_pct = 0.30f;
    ewv->fill_kind = fill_kind;
    ewv->text_edit_arena = df_view_push_arena_ext(view);
  }
}

internal void
df_watch_view_cmds(DF_Window *ws, DF_Panel *panel, DF_View *view, DF_WatchViewState *ewv, DF_CmdList *cmds)
{
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    
    // rjf: process
    switch(core_cmd_kind)
    {
      default:break;
      
      //- rjf: watch expression toggling
      case DF_CoreCmdKind_ToggleWatchExpression:
      if(cmd->params.string.size != 0)
      {
        DF_EvalRoot *already_existing_root = df_eval_root_from_string(ewv, cmd->params.string);
        if(already_existing_root != 0)
        {
          df_eval_root_release(ewv, already_existing_root);
        }
        else
        {
          DF_EvalRoot *root = df_eval_root_alloc(view, ewv);
          df_eval_root_equip_string(root, cmd->params.string);
        }
      }break;
    }
  }
}

internal void
df_watch_view_build(DF_Window *ws, DF_Panel *panel, DF_View *view, DF_WatchViewState *ewv, B32 modifiable, U32 default_radix, Rng2F32 rect)
{
  ProfBeginFunction();
  DBGI_Scope *scope = dbgi_scope_open();
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: unpack arguments
  //
  F_Tag code_font = df_font_from_slot(DF_FontSlot_Code);
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  U64 thread_ip_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx.unwind_count);
  DF_EvalViewKey eval_view_key = df_eval_view_key_from_eval_watch_view(ewv);
  DF_EvalView *eval_view = df_eval_view_from_key(eval_view_key);
  String8 filter = str8(view->query_buffer, view->query_string_size);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  S64 num_possible_visible_rows = (S64)(dim_2f32(rect).y/row_height_px);
  
  //////////////////////////////
  //- rjf: process * thread info -> parse_ctx
  //
  EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(scope, process, thread_ip_vaddr);
  
  //////////////////////////////
  //- rjf: determine autocompletion string
  //
  String8 autocomplete_hint_string = {0};
  {
    UI_EventList *events = ui_events();
    for(UI_EventNode *n = events->first; n != 0; n = n->next)
    {
      if(n->v.kind == UI_EventKind_AutocompleteHint)
      {
        autocomplete_hint_string = n->v.string;
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: consume events & perform navigations/edits - calculate state
  //
  EVAL_String2ExprMap macro_map = {0};
  DF_EvalVizBlockList blocks = {0};
  UI_ScrollListRowBlockArray row_blocks = {0};
  Vec2S64 cursor_tbl = {0};
  Vec2S64 mark_tbl = {0};
  Rng2S64 selection_tbl = {0};
  UI_Focus(UI_FocusKind_On)
  {
    UI_EventList *events = ui_events();
    B32 state_dirty = 1;
    B32 cursor_dirty__tbl = 0;
    B32 take_autocomplete = 0;
    for(UI_EventNode *event_n = events->first, *next = 0;; event_n = next)
    {
      //////////////////////////
      //- rjf: state -> macro map
      //
      if(state_dirty)
      {
        macro_map = eval_string2expr_map_make(scratch.arena, 256);
        for(DF_EvalRoot *root = ewv->first_root; root != 0; root = root->next)
        {
          String8 root_expr = str8(root->expr_buffer, root->expr_buffer_string_size);
          
          //- rjf: unpack arguments
          DF_Entity *process = thread->parent;
          U64 unwind_count = ctrl_ctx.unwind_count;
          CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
          Architecture arch = df_architecture_from_entity(thread);
          U64 reg_size = regs_block_size_from_architecture(arch);
          U64 thread_unwind_ip_vaddr = 0;
          void *thread_unwind_regs_block = push_array(scratch.arena, U8, reg_size);
          {
            U64 idx = 0;
            for(CTRL_UnwindFrame *f = unwind.first; f != 0; f = f->next, idx += 1)
            {
              if(idx == unwind_count)
              {
                thread_unwind_ip_vaddr = f->rip;
                thread_unwind_regs_block = f->regs;
                break;
              }
            }
          }
          
          //- rjf: lex & parse
          EVAL_TokenArray tokens = eval_token_array_from_text(scratch.arena, root_expr);
          EVAL_ParseResult parse = eval_parse_expr_from_text_tokens(scratch.arena, &parse_ctx, root_expr, &tokens);
          EVAL_ErrorList errors = parse.errors;
          if(errors.count == 0)
          {
            eval_push_leaf_ident_exprs_from_expr__in_place(scratch.arena, &macro_map, parse.expr, &errors);
          }
        }
      }
      
      //////////////////////////
      //- rjf: state -> viz blocks
      //
      if(state_dirty)
      {
        blocks = df_eval_viz_block_list_from_watch_view_state(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, view, ewv);
      }
      
      //////////////////////////
      //- rjf: does this eval watch view allow mutation? -> add extra block for editable empty row
      //
      DF_ExpandKey empty_row_parent_key = df_expand_key_make(max_U64, max_U64);
      DF_ExpandKey empty_row_key = df_expand_key_make(df_hash_from_expand_key(empty_row_parent_key), 1);
      if(state_dirty && modifiable)
      {
        DF_EvalVizBlock *b = df_eval_viz_block_begin(scratch.arena, DF_EvalVizBlockKind_Null, empty_row_parent_key, empty_row_key, 0);
        b->visual_idx_range = b->semantic_idx_range = r1u64(0, 1);
        df_eval_viz_block_end(&blocks, b);
      }
      
      //////////////////////////
      //- rjf: viz blocks -> ui row blocks
      //
      {
        UI_ScrollListRowBlockChunkList row_block_chunks = {0};
        for(DF_EvalVizBlockNode *n = blocks.first; n != 0; n = n->next)
        {
          DF_EvalVizBlock *vb = &n->v;
          UI_ScrollListRowBlock block = {0};
          block.row_count = dim_1u64(vb->visual_idx_range);
          block.item_count = dim_1u64(vb->semantic_idx_range);
          ui_scroll_list_row_block_chunk_list_push(scratch.arena, &row_block_chunks, 256, &block);
        }
        row_blocks = ui_scroll_list_row_block_array_from_chunk_list(scratch.arena, &row_block_chunks);
      }
      
      //////////////////////////
      //- rjf: conclude state update
      //
      if(state_dirty)
      {
        state_dirty = 0;
      }
      
      //////////////////////////////
      //- rjf: 2D table coordinates * blocks -> stable cursor state
      //
      if(cursor_dirty__tbl)
      {
        cursor_dirty__tbl = 0;
        struct
        {
          DF_WatchViewPoint *pt_state;
          Vec2S64 pt_tbl;
        }
        points[] =
        {
          {&ewv->cursor, cursor_tbl},
          {&ewv->mark, mark_tbl},
        };
        for(U64 point_idx = 0; point_idx < ArrayCount(points); point_idx += 1)
        {
          DF_ExpandKey last_key = points[point_idx].pt_state->key;
          DF_ExpandKey last_parent_key = points[point_idx].pt_state->parent_key;
          points[point_idx].pt_state[0] = df_watch_view_point_from_tbl(&blocks, points[point_idx].pt_tbl);
          if(df_expand_key_match(df_expand_key_zero(), points[point_idx].pt_state->key))
          {
            points[point_idx].pt_state->key = last_parent_key;
            DF_ExpandNode *node = df_expand_node_from_key(&eval_view->expand_tree_table, last_parent_key);
            for(DF_ExpandNode *n = node; n != 0; n = n->parent)
            {
              points[point_idx].pt_state->key = n->key;
              if(n->expanded == 0)
              {
                break;
              }
            }
          }
          if(point_idx == 0 &&
             (!df_expand_key_match(ewv->cursor.key, last_key) ||
              !df_expand_key_match(ewv->cursor.parent_key, last_parent_key)))
          {
            ewv->text_editing = 0;
          }
        }
      }
      
      //////////////////////////
      //- rjf: stable cursor state * blocks -> 2D table coordinates
      //
      {
        cursor_tbl = df_tbl_from_watch_view_point(&blocks, ewv->cursor);
        mark_tbl = df_tbl_from_watch_view_point(&blocks, ewv->mark);
        selection_tbl = r2s64p(Min(cursor_tbl.x, mark_tbl.x), Min(cursor_tbl.y, mark_tbl.y),
                               Max(cursor_tbl.x, mark_tbl.x), Max(cursor_tbl.y, mark_tbl.y));
      }
      
      //////////////////////////
      //- rjf: grab next event, if any - otherwise exit the loop, as we now have
      // the most up-to-date state
      //
      if(event_n == 0 || !ui_is_focus_active())
      {
        break;
      }
      next = event_n->next;
      UI_Event *evt = &event_n->v;
      B32 taken = 0;
      
      //////////////////////////
      //- rjf: begin editing on some operations
      //
      if(!ewv->text_editing &&
         (evt->kind == UI_EventKind_Text ||
          evt->flags & UI_EventFlag_Paste ||
          (evt->kind == UI_EventKind_Press && (evt->key == OS_Key_Return || evt->key == OS_Key_F2))))
      {
        Vec2S64 selection_dim = dim_2s64(selection_tbl);
        ewv->text_editing = 1;
        arena_clear(ewv->text_edit_arena);
        ewv->text_edit_state_slots_count = u64_up_to_pow2(selection_dim.y+1);
        ewv->text_edit_state_slots_count = Max(ewv->text_edit_state_slots_count, 64);
        ewv->text_edit_state_slots = push_array(ewv->text_edit_arena, DF_WatchViewTextEditState*, ewv->text_edit_state_slots_count);
        DF_EvalVizWindowedRowList rows = df_eval_viz_windowed_row_list_from_viz_block_list(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, eval_view, default_radix, code_font, ui_top_font_size(),
                                                                                           r1s64(ui_scroll_list_row_from_item(&row_blocks, selection_tbl.min.y-1),
                                                                                                 ui_scroll_list_row_from_item(&row_blocks, selection_tbl.max.y-1)+1), &blocks);
        DF_EvalVizRow *row = rows.first;
        for(S64 y = selection_tbl.min.y; y <= selection_tbl.max.y; y += 1, row = row->next)
        {
          for(S64 x = selection_tbl.min.x; x <= selection_tbl.max.x; x += 1)
          {
            String8 string = df_string_from_eval_viz_row_column_kind(scratch.arena, eval_view, parse_ctx.type_graph, parse_ctx.rdi, row, (DF_WatchViewColumnKind)x, 1);
            string.size = Min(string.size, sizeof(ewv->dummy_text_edit_state.input_buffer));
            DF_WatchViewPoint pt = {(DF_WatchViewColumnKind)x, row->parent_key, row->key};
            U64 hash = df_hash_from_expand_key(pt.key);
            U64 slot_idx = hash%ewv->text_edit_state_slots_count;
            DF_WatchViewTextEditState *edit_state = push_array(ewv->text_edit_arena, DF_WatchViewTextEditState, 1);
            SLLStackPush_N(ewv->text_edit_state_slots[slot_idx], edit_state, pt_hash_next);
            edit_state->pt = pt;
            edit_state->cursor = txt_pt(1, string.size+1);
            edit_state->mark = txt_pt(1, 1);
            edit_state->input_size = string.size;
            MemoryCopy(edit_state->input_buffer, string.str, string.size);
            edit_state->initial_size = string.size;
            MemoryCopy(edit_state->initial_buffer, string.str, string.size);
          }
        }
      }
      
      //////////////////////////
      //- rjf: [text] apply textual edits
      //
      if(ewv->text_editing)
      {
        B32 editing_complete = ((evt->kind == UI_EventKind_Press && (evt->key == OS_Key_Esc || evt->key == OS_Key_Return)) ||
                                (evt->kind == UI_EventKind_Navigate && evt->delta_2s32.y != 0));
        if(editing_complete ||
           ((evt->kind == UI_EventKind_Edit ||
             evt->kind == UI_EventKind_Navigate ||
             evt->kind == UI_EventKind_Text) &&
            evt->delta_2s32.y == 0))
        {
          taken = 1;
          for(S64 y = selection_tbl.min.y; y <= selection_tbl.max.y; y += 1)
          {
            for(S64 x = selection_tbl.min.x; x <= selection_tbl.max.x; x += 1)
            {
              DF_WatchViewPoint pt = df_watch_view_point_from_tbl(&blocks, v2s64(x, y));
              DF_WatchViewTextEditState *edit_state = df_watch_view_text_edit_state_from_pt(ewv, pt);
              String8 string = str8(edit_state->input_buffer, edit_state->input_size);
              UI_TxtOp op = ui_single_line_txt_op_from_event(scratch.arena, evt, string, edit_state->cursor, edit_state->mark);
              
              // rjf: any valid op & autocomplete hint? -> perform autocomplete first, then re-compute op
              if(autocomplete_hint_string.size != 0)
              {
                take_autocomplete = 1;
                String8 word_query = df_autocomp_query_word_from_input_string_off(string, edit_state->cursor.column-1);
                U64 word_off = (U64)(word_query.str - string.str);
                String8 new_string = ui_push_string_replace_range(scratch.arena, string, r1s64(word_off+1, word_off+1+word_query.size), autocomplete_hint_string);
                new_string.size = Min(sizeof(edit_state->input_buffer), new_string.size);
                MemoryCopy(edit_state->input_buffer, new_string.str, new_string.size);
                edit_state->input_size = new_string.size;
                edit_state->cursor = edit_state->mark = txt_pt(1, word_off+1+autocomplete_hint_string.size);
                string = str8(edit_state->input_buffer, edit_state->input_size);
                op = ui_single_line_txt_op_from_event(scratch.arena, evt, string, edit_state->cursor, edit_state->mark);
              }
              
              // rjf: cancel? -> revert to initial string
              if(editing_complete && evt->key == OS_Key_Esc)
              {
                string = str8(edit_state->initial_buffer, edit_state->initial_size);
              }
              
              // rjf: obtain edited string
              String8 new_string = string;
              if(!txt_pt_match(op.range.min, op.range.max) || op.replace.size != 0)
              {
                new_string = ui_push_string_replace_range(scratch.arena, string, r1s64(op.range.min.column, op.range.max.column), op.replace);
              }
              
              // rjf: commit to edit state
              new_string.size = Min(new_string.size, sizeof(edit_state->input_buffer));
              MemoryCopy(edit_state->input_buffer, new_string.str, new_string.size);
              edit_state->input_size = new_string.size;
              edit_state->cursor = op.cursor;
              edit_state->mark = op.mark;
              
              // rjf: commit edited cell string
              Vec2S64 tbl = v2s64(x, y);
              switch((DF_WatchViewColumnKind)x)
              {
                default:{}break;
                case DF_WatchViewColumnKind_Expr:
                {
                  DF_WatchViewPoint pt = df_watch_view_point_from_tbl(&blocks, tbl);
                  DF_EvalRoot *root = df_eval_root_from_expand_key(ewv, eval_view, pt.key);
                  if(root != 0)
                  {
                    df_eval_root_equip_string(root, new_string);
                    state_dirty = 1;
                  }
                  else if(editing_complete && new_string.size != 0 && df_expand_key_match(pt.key, empty_row_key))
                  {
                    root = df_eval_root_alloc(view, ewv);
                    df_eval_root_equip_string(root, new_string);
                    state_dirty = 1;
                  }
                }break;
                case DF_WatchViewColumnKind_Value:
                if(editing_complete)
                {
                  DF_EvalVizWindowedRowList rows = df_eval_viz_windowed_row_list_from_viz_block_list(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, eval_view, default_radix, code_font, ui_top_font_size(),
                                                                                                     r1s64(ui_scroll_list_row_from_item(&row_blocks, y-1),
                                                                                                           ui_scroll_list_row_from_item(&row_blocks, y-1)+1), &blocks);
                  B32 success = 0;
                  if(rows.first != 0)
                  {
                    DF_Eval write_eval = df_eval_from_string(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, new_string);
                    success = df_commit_eval_value(parse_ctx.type_graph, parse_ctx.rdi, &ctrl_ctx, rows.first->eval, write_eval);
                  }
                  if(!success)
                  {
                    DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
                    params.string = str8_lit("Could not commit value successfully.");
                    df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
                  }
                }break;
                case DF_WatchViewColumnKind_Type:{}break;
                case DF_WatchViewColumnKind_ViewRule:
                if(editing_complete)
                {
                  DF_WatchViewPoint pt = df_watch_view_point_from_tbl(&blocks, tbl);
                  df_eval_view_set_key_rule(eval_view, pt.key, new_string);
                  state_dirty = 1;
                }break;
              }
            }
          }
        }
        if(editing_complete)
        {
          ewv->text_editing = 0;
        }
      }
      
      //////////////////////////
      //- rjf: [table] do cell-granularity copies
      //
      if(!ewv->text_editing && evt->flags & UI_EventFlag_Copy)
      {
        taken = 1;
        String8List strs = {0};
        DF_EvalVizWindowedRowList rows = df_eval_viz_windowed_row_list_from_viz_block_list(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, eval_view, default_radix, code_font, ui_top_font_size(),
                                                                                           r1s64(ui_scroll_list_row_from_item(&row_blocks, selection_tbl.min.y-1),
                                                                                                 ui_scroll_list_row_from_item(&row_blocks, selection_tbl.max.y-1)+1), &blocks);
        DF_EvalVizRow *row = rows.first;
        for(S64 y = selection_tbl.min.y; y <= selection_tbl.max.y && row != 0; y += 1, row = row->next)
        {
          for(S64 x = selection_tbl.min.x; x <= selection_tbl.max.x; x += 1)
          {
            String8 cell_string = df_string_from_eval_viz_row_column_kind(scratch.arena, eval_view, parse_ctx.type_graph, parse_ctx.rdi, row, (DF_WatchViewColumnKind)x, 0);
            str8_list_pushf(scratch.arena, &strs, "%S%s", cell_string, x+1 <= selection_tbl.max.x ? "  " : "");
          }
          if(y+1 <= selection_tbl.max.y)
          {
            str8_list_push(scratch.arena, &strs, str8_lit("\n"));
          }
        }
        String8 string = str8_list_join(scratch.arena, &strs, 0);
        os_set_clipboard_text(string);
      }
      
      //////////////////////////
      //- rjf: [table] do cell-granularity deletions
      //
      if(!ewv->text_editing && evt->flags & UI_EventFlag_Delete)
      {
        taken = 1;
        for(S64 y = selection_tbl.min.y; y <= selection_tbl.max.y; y += 1)
        {
          DF_WatchViewPoint pt = df_watch_view_point_from_tbl(&blocks, v2s64(0, y));
          
          // rjf: row deletions
          if(selection_tbl.min.x <= 0)
          {
            DF_EvalRoot *root = df_eval_root_from_expand_key(ewv, eval_view, pt.key);
            if(root != 0)
            {
              DF_ExpandKey new_cursor_key = empty_row_key;
              DF_ExpandKey new_cursor_parent_key = empty_row_parent_key;
              if((evt->delta_2s32.x < 0 || evt->delta_2s32.y < 0) && root->prev != 0)
              {
                new_cursor_key = df_expand_key_from_eval_root(root->prev);
                new_cursor_parent_key = df_parent_expand_key_from_eval_root(root->prev);
              }
              else if(root->next != 0)
              {
                new_cursor_key = df_expand_key_from_eval_root(root->next);
                new_cursor_parent_key = df_parent_expand_key_from_eval_root(root->next);
              }
              DF_WatchViewPoint new_cursor_pt = {DF_WatchViewColumnKind_Expr, new_cursor_parent_key, new_cursor_key};
              df_eval_root_release(ewv, root);
              ewv->cursor = ewv->mark = new_cursor_pt;
            }
          }
          
          // rjf: view rule deletions
          else if(selection_tbl.min.x <= DF_WatchViewColumnKind_ViewRule && DF_WatchViewColumnKind_ViewRule <= selection_tbl.max.x)
          {
            df_eval_view_set_key_rule(eval_view, pt.key, str8_zero());
          }
        }
      }
      
      //////////////////////////
      //- rjf: [table] apply deltas to cursor & mark
      //
      if(!ewv->text_editing && !(evt->flags & UI_EventFlag_Delete))
      {
        B32 cursor_tbl_min_is_empty_selection[Axis2_COUNT] = {0, 1};
        Rng2S64 cursor_tbl_range = r2s64(v2s64(0, 0), v2s64(3, blocks.total_semantic_row_count));
        Rng1S64 item_range = r1s64(0, 1 + blocks.total_visual_row_count);
        Vec2S32 delta = evt->delta_2s32;
        if(evt->flags & UI_EventFlag_PickSelectSide && !MemoryMatchStruct(&selection_tbl.min, &selection_tbl.max))
        {
          if(delta.x > 0 || delta.y > 0)
          {
            cursor_tbl.x = selection_tbl.max.x;
            cursor_tbl.y = selection_tbl.max.y;
          }
          else if(delta.x < 0 || delta.y < 0)
          {
            cursor_tbl.x = selection_tbl.min.x;
            cursor_tbl.y = selection_tbl.min.y;
          }
        }
        if(evt->flags & UI_EventFlag_ZeroDeltaOnSelect && !MemoryMatchStruct(&selection_tbl.min, &selection_tbl.max))
        {
          MemoryZeroStruct(&delta);
        }
        B32 moved = 1;
        switch(evt->delta_unit)
        {
          default:{moved = 0;}break;
          case UI_EventDeltaUnit_Char:
          {
            for(EachEnumVal(Axis2, axis))
            {
              cursor_tbl.v[axis] += delta.v[axis];
              if(cursor_tbl.v[axis] < cursor_tbl_range.min.v[axis])
              {
                cursor_tbl.v[axis] = cursor_tbl_range.max.v[axis];
              }
              if(cursor_tbl.v[axis] > cursor_tbl_range.max.v[axis])
              {
                cursor_tbl.v[axis] = cursor_tbl_range.min.v[axis];
              }
              cursor_tbl.v[axis] = clamp_1s64(r1s64(cursor_tbl_range.min.v[axis], cursor_tbl_range.max.v[axis]), cursor_tbl.v[axis]);
            }
          }break;
          case UI_EventDeltaUnit_Word:
          case UI_EventDeltaUnit_Line:
          case UI_EventDeltaUnit_Page:
          {
            cursor_tbl.x  = (delta.x>0 ? (cursor_tbl_range.max.x) :
                             delta.x<0 ? (cursor_tbl_range.min.x + !!cursor_tbl_min_is_empty_selection[Axis2_X]) :
                             cursor_tbl.x);
            cursor_tbl.y += ((delta.y>0 ? +(num_possible_visible_rows-3) :
                              delta.y<0 ? -(num_possible_visible_rows-3) :
                              0));
            cursor_tbl.y = clamp_1s64(r1s64(cursor_tbl_range.min.y + !!cursor_tbl_min_is_empty_selection[Axis2_Y],
                                            cursor_tbl_range.max.y),
                                      cursor_tbl.y);
          }break;
          case UI_EventDeltaUnit_Whole:
          {
            for(EachEnumVal(Axis2, axis))
            {
              cursor_tbl.v[axis] = (delta.v[axis]>0 ? cursor_tbl_range.max.v[axis] : delta.v[axis]<0 ? cursor_tbl_range.min.v[axis] + !!cursor_tbl_min_is_empty_selection[axis] : cursor_tbl.v[axis]);
            }
          }break;
        }
        if(moved)
        {
          taken = 1;
          cursor_dirty__tbl = 1;
          {
            Rng1S64 scroll_row_idx_range = r1s64(item_range.min, ClampBot(item_range.min, item_range.max-1));
            S64 cursor_item_idx = cursor_tbl.y-1;
            if(item_range.min <= cursor_item_idx && cursor_item_idx <= item_range.max)
            {
              UI_ScrollPt *scroll_pt = &view->scroll_pos.y;
              
              //- rjf: compute visible row range
              Rng1S64 visible_row_range = r1s64(scroll_pt->idx + 0 - !!(scroll_pt->off < 0),
                                                scroll_pt->idx + 0 + num_possible_visible_rows + 1);
              
              //- rjf: compute cursor row range from cursor item
              Rng1S64 cursor_visibility_row_range = {0};
              if(row_blocks.count == 0)
              {
                cursor_visibility_row_range = r1s64(cursor_item_idx-1, cursor_item_idx+3);
              }
              else
              {
                cursor_visibility_row_range.min = (S64)ui_scroll_list_row_from_item(&row_blocks, (U64)cursor_item_idx);
                cursor_visibility_row_range.max = cursor_visibility_row_range.min + 4;
              }
              
              //- rjf: compute deltas & apply
              S64 min_delta = Min(0, cursor_visibility_row_range.min-visible_row_range.min);
              S64 max_delta = Max(0, cursor_visibility_row_range.max-visible_row_range.max);
              S64 new_idx = scroll_pt->idx+min_delta+max_delta;
              new_idx = clamp_1s64(scroll_row_idx_range, new_idx);
              ui_scroll_pt_target_idx(scroll_pt, new_idx);
            }
          }
          
        }
      }
      
      //////////////////////////
      //- rjf: [table] stick table mark to cursor if needed
      //
      if(!ewv->text_editing)
      {
        if(taken && !(evt->flags & UI_EventFlag_KeepMark))
        {
          mark_tbl = cursor_tbl;
        }
      }
      
      //////////////////////////
      //- rjf: consume event, if taken
      //
      if(taken)
      {
        ui_eat_event(events, event_n);
      }
    }
    if(take_autocomplete)
    {
      for(UI_EventNode *n = events->first; n != 0; n = n->next)
      {
        if(n->v.kind == UI_EventKind_AutocompleteHint)
        {
          ui_eat_event(events, n);
          break;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build ui
  //
  F32 *col_pcts[] =
  {
    &ewv->expr_column_pct,
    &ewv->value_column_pct,
    &ewv->type_column_pct,
    &ewv->view_rule_column_pct,
  };
  B32 pressed = 0;
  Vec2S64 next_cursor_tbl = cursor_tbl;
  Rng1S64 visible_row_rng = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = floor_f32(ui_top_font_size()*2.5f);
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(3, blocks.total_semantic_row_count));
    scroll_list_params.item_range    = r1s64(0, 1 + blocks.total_visual_row_count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
    UI_ScrollListRowBlockChunkList row_block_chunks = {0};
    for(DF_EvalVizBlockNode *n = blocks.first; n != 0; n = n->next)
    {
      DF_EvalVizBlock *vb = &n->v;
      UI_ScrollListRowBlock block = {0};
      block.row_count = dim_1u64(vb->visual_idx_range);
      block.item_count = dim_1u64(vb->semantic_idx_range);
      ui_scroll_list_row_block_chunk_list_push(scratch.arena, &row_block_chunks, 256, &block);
    }
    scroll_list_params.row_blocks = ui_scroll_list_row_block_array_from_chunk_list(scratch.arena, &row_block_chunks);
  }
  UI_BoxFlags disabled_flags = ui_top_flags();
  if(df_ctrl_targets_running())
  {
    disabled_flags |= UI_BoxFlag_Disabled;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params, &view->scroll_pos.y,
                  0,
                  0,
                  &visible_row_rng,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_TableF(ArrayCount(col_pcts), col_pcts, "table_header")
  {
    next_cursor_tbl = cursor_tbl;
    
    //- rjf: build table header
    if(visible_row_rng.min == 0) UI_TableVector UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      UI_TableCell ui_label(str8_lit("Expression"));
      UI_TableCell ui_label(str8_lit("Value"));
      UI_TableCell ui_label(str8_lit("Type"));
      UI_TableCell if(df_help_label(str8_lit("View Rule"))) UI_Tooltip
      {
        F32 max_width = ui_top_font_size()*35;
        ui_label_multiline(max_width, str8_lit("View rules are used to tweak the way evaluated expressions are visualized. Multiple rules can be specified on each row. They are specified in a key:(value) form."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("array:(N)");
        ui_label_multiline(max_width, str8_lit("Specifies that a pointer points to N elements, rather than only 1."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("omit:(member_1 ... member_n)");
        ui_label_multiline(max_width, str8_lit("Omits a list of member names from appearing in struct, union, or class evaluations."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("only:(member_1 ... member_n)");
        ui_label_multiline(max_width, str8_lit("Specifies that only the specified members should appear in struct, union, or class evaluations."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("list:(next_link_member_name)");
        ui_label_multiline(max_width, str8_lit("Specifies that some struct, union, or class forms the top of a linked list, with next_link_member_name being the member which points at the next element in the list."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("dec");
        ui_label_multiline(max_width, str8_lit("Specifies that all integral evaluations should appear in base-10 form."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("hex");
        ui_label_multiline(max_width, str8_lit("Specifies that all integral evaluations should appear in base-16 form."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("oct");
        ui_label_multiline(max_width, str8_lit("Specifies that all integral evaluations should appear in base-8 form."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("bin");
        ui_label_multiline(max_width, str8_lit("Specifies that all integral evaluations should appear in base-2 form."));
        ui_spacer(ui_em(1.5f, 1));
        UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText)) ui_labelf("no_addr");
        ui_label_multiline(max_width, str8_lit("Displays only what pointers point to, if possible, without the pointer's address value."));
        ui_spacer(ui_em(1.5f, 1));
      }
    }
    
    //- rjf: viz blocks -> rows
    DF_EvalVizWindowedRowList rows = {0};
    {
      rows = df_eval_viz_windowed_row_list_from_viz_block_list(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, eval_view, default_radix, code_font, ui_top_font_size(), r1s64(visible_row_rng.min-1, visible_row_rng.max), &blocks);
    }
    
    //- rjf: build table
    ProfScope("build table")
    {
      //- rjf: build rows
      U64 semantic_idx = rows.count_before_semantic;
      for(DF_EvalVizRow *row = rows.first; row != 0; row = row->next, semantic_idx += 1)
      {
        //- rjf: unpack row info
        U64 row_hash = df_hash_from_expand_key(row->key);
        U64 expr_hash = df_hash_from_string(row->display_expr);
        df_expand_tree_table_animate(&eval_view->expand_tree_table, df_dt());
        B32 row_selected = (selection_tbl.min.y <= (semantic_idx+1) && (semantic_idx+1) <= selection_tbl.max.y);
        B32 row_expanded = df_expand_key_is_set(&eval_view->expand_tree_table, row->key);
        
        //- rjf: determine if row's data is fresh and/or bad
        B32 row_is_fresh = 0;
        B32 row_is_bad = 0;
        switch(row->eval.mode)
        {
          default:{}break;
          case EVAL_EvalMode_Addr:
          {
            U64 size = tg_byte_size_from_graph_rdi_key(parse_ctx.type_graph, parse_ctx.rdi, row->eval.type_key);
            size = Min(size, 64);
            Rng1U64 vaddr_rng = r1u64(row->eval.offset, row->eval.offset+size);
            CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, vaddr_rng, 0);
            for(U64 idx = 0; idx < (slice.data.size+63)/64; idx += 1)
            {
              if(slice.byte_changed_flags[idx] != 0)
              {
                row_is_fresh = 1;
              }
              if(slice.byte_bad_flags[idx] != 0)
              {
                row_is_bad = 1;
              }
            }
          }break;
        }
        
        //- rjf: build canvas row
        if(row->flags & DF_EvalVizRowFlag_Canvas) UI_FocusHot(row_selected ? UI_FocusKind_On : UI_FocusKind_Off) ProfScope("canvas row")
        {
          //- rjf: build
          ui_set_next_flags(disabled_flags);
          ui_set_next_pref_width(ui_pct(1, 0));
          ui_set_next_pref_height(ui_px(scroll_list_params.row_height_px*row->size_in_rows, 1.f));
          UI_Box *vector = ui_build_box_from_stringf(UI_BoxFlag_DrawSideBottom|UI_BoxFlag_RequireFocusBackground|UI_BoxFlag_Clickable, "row_%I64x", row_hash);
          UI_Parent(vector)
          {
            ui_set_next_fixed_y(-1.f * (row->skipped_size_in_rows) * scroll_list_params.row_height_px);
            ui_set_next_fixed_height((row->skipped_size_in_rows + row->size_in_rows + row->chopped_size_in_rows) * scroll_list_params.row_height_px);
            ui_set_next_child_layout_axis(Axis2_X);
            UI_Box *canvas_box = ui_build_box_from_stringf(UI_BoxFlag_FloatingY, "###canvas_%I64x", row_hash);
            if(row->expand_ui_rule_spec != &df_g_nil_gfx_view_rule_spec && row->expand_ui_rule_spec->info.block_ui)
            {
              UI_Parent(canvas_box) UI_WidthFill UI_HeightFill
              {
                Vec2F32 canvas_dim = v2f32(scroll_list_params.dim_px.x - ui_top_font_size()*1.5f,
                                           (row->skipped_size_in_rows+row->size_in_rows+row->chopped_size_in_rows)*scroll_list_params.row_height_px);
                row->expand_ui_rule_spec->info.block_ui(ws, row->key, row->eval, row->edit_expr, scope, &ctrl_ctx, &parse_ctx, &macro_map, row->expand_ui_rule_node, canvas_dim);
              }
            }
          }
          
          //- rjf: take interaction 
          UI_Signal sig = ui_signal_from_box(vector);
          
          //- rjf: press -> focus
          if(ui_pressed(sig))
          {
            // TODO(rjf): edit_commit = edit_commit || (!row_selected && ewv->text_editing);
            next_cursor_tbl = v2s64(DF_WatchViewColumnKind_Expr, (semantic_idx+1));
            pressed = 1;
          }
          
          //- rjf: double clicked or keyboard clicked -> open dedicated tab
          if(ui_double_clicked(sig) || sig.f & UI_SignalFlag_KeyboardPressed)
          {
            DF_CfgNode *cfg = df_cfg_tree_copy(scratch.arena, row->expand_ui_rule_node);
            DF_CfgNode *cfg_root = push_array(scratch.arena, DF_CfgNode, 1);
            cfg_root->first = cfg_root->last = cfg;
            cfg_root->next = cfg_root->parent = &df_g_nil_cfg_node;
            if(cfg != &df_g_nil_cfg_node)
            {
              cfg->parent = cfg_root;
            }
            DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
            p.string = row->edit_expr;
            p.view_spec = df_tab_view_spec_from_gfx_view_rule_spec(row->expand_ui_rule_spec);
            p.cfg_node = cfg_root;
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_ViewSpec);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_CfgNode);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_OpenTab));
          }
        }
        
        //- rjf: build normal row
        if(!(row->flags & DF_EvalVizRowFlag_Canvas)) ProfScope("row")
        {
          if(row_is_fresh)
          {
            ui_set_next_flags(disabled_flags|UI_BoxFlag_DrawOverlay);
            ui_set_next_overlay_color(mul_4f32(df_rgba_from_theme_color(DF_ThemeColor_Highlight0), v4f32(1, 1, 1, 0.2f)));
          }
          else
          {
            ui_set_next_flags(disabled_flags);
          }
          UI_NamedTableVectorF("row_%I64x_%I64x", row_hash, ewv->root_count)
          {
            //- rjf: draw start of cache lines in expansions
            if((row->eval.mode == EVAL_EvalMode_Addr || row->eval.mode == EVAL_EvalMode_NULL) &&
               row->eval.errors.count == 0 &&
               row->eval.offset%64 == 0 && row->depth > 0 &&
               !row_expanded)
            {
              ui_set_next_fixed_x(0);
              ui_set_next_fixed_y(0);
              ui_set_next_fixed_height(ui_top_font_size()*0.1f);
              ui_set_next_background_color(df_rgba_from_theme_color(DF_ThemeColor_Highlight0));
              ui_build_box_from_key(UI_BoxFlag_Floating|UI_BoxFlag_DrawBackground, ui_key_zero());
            }
            
            //- rjf: draw mid-row cache line boundaries in expansions
            if((row->eval.mode == EVAL_EvalMode_Addr || row->eval.mode == EVAL_EvalMode_NULL) &&
               row->eval.errors.count == 0 &&
               row->eval.offset%64 != 0 &&
               row->depth > 0 &&
               !row_expanded)
            {
              U64 next_off = (row->eval.offset + tg_byte_size_from_graph_rdi_key(parse_ctx.type_graph, parse_ctx.rdi, row->eval.type_key));
              if(next_off%64 != 0 && row->eval.offset/64 < next_off/64)
              {
                ui_set_next_fixed_x(0);
                ui_set_next_fixed_y(scroll_list_params.row_height_px - ui_top_font_size()*0.5f);
                ui_set_next_fixed_height(ui_top_font_size()*1.f);
                Vec4F32 boundary_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
                boundary_color.w *= 0.25f;
                ui_set_next_background_color(boundary_color);
                ui_build_box_from_key(UI_BoxFlag_Floating|UI_BoxFlag_DrawBackground, ui_key_zero());
              }
            }
            
            //- rjf: expression
            ProfScope("expr")
            {
              DF_WatchViewPoint pt = {DF_WatchViewColumnKind_Expr, row->parent_key, row->key};
              DF_WatchViewTextEditState *edit_state = df_watch_view_text_edit_state_from_pt(ewv, pt);
              B32 cell_selected = (row_selected && selection_tbl.min.x <= pt.column_kind && pt.column_kind <= selection_tbl.max.x);
              B32 can_edit_expr = !(row->depth > 0 || modifiable == 0);
              
              // rjf: begin editing
#if 0
              if(cell_selected && (edit_begin || (edit_begin_or_expand && !(row->flags & DF_EvalVizRowFlag_CanExpand))) && can_edit_expr)
              {
                ewv->text_editing = 1;
                ewv->input_size = Min(sizeof(ewv->input_buffer), row->display_expr.size);
                MemoryCopy(ewv->input_buffer, row->display_expr.str, ewv->input_size);
                ewv->input_cursor = txt_pt(1, 1+ewv->input_size);
                ewv->input_mark = txt_pt(1, 1);
              }
#endif
              
              // rjf: build
              UI_Signal sig = {0};
              B32 next_expanded = row_expanded;
              if(row->flags & DF_EvalVizRowFlag_ExprIsSpecial)
              {
                ui_set_next_flags(disabled_flags|UI_BoxFlag_DrawOverlay);
                ui_set_next_overlay_color(mul_4f32(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground), v4f32(1, 1, 1, 0.2f)));
              }
              UI_TableCell
                UI_FocusHot(cell_selected ? UI_FocusKind_On : UI_FocusKind_Off)
                UI_FocusActive((cell_selected && ewv->text_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
              {
                B32 expr_editing_active = ui_is_focus_active();
                B32 is_inherited = (row->inherited_type_key_chain.count != 0);
                UI_Font(code_font) UI_TextColor((row->flags & DF_EvalVizRowFlag_ExprIsSpecial) ?
                                                df_rgba_from_theme_color(DF_ThemeColor_Highlight0) :
                                                row->depth > 0 ? df_rgba_from_theme_color(DF_ThemeColor_WeakText) : ui_top_text_color())
                {
                  if(is_inherited)
                  {
                    Vec4F32 inherited_bg_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight1);
                    inherited_bg_color.w *= 0.2f;
                    ui_set_next_background_color(inherited_bg_color);
                  }
                  FuzzyMatchRangeList matches = {0};
                  if(filter.size != 0)
                  {
                    matches = fuzzy_match_find(scratch.arena, filter, row->display_expr);
                  }
                  sig = df_line_editf((DF_LineEditFlag_CodeContents*(!(row->flags & DF_EvalVizRowFlag_ExprIsSpecial))|
                                       DF_LineEditFlag_NoBackground*(!is_inherited)|
                                       DF_LineEditFlag_DisableEdit*(!can_edit_expr)|
                                       DF_LineEditFlag_Expander*!!(row->flags & DF_EvalVizRowFlag_CanExpand)|
                                       DF_LineEditFlag_ExpanderPlaceholder*(row->depth==0)|
                                       DF_LineEditFlag_ExpanderSpace*(row->depth!=0)),
                                      row->depth,
                                      filter.size ? &matches : 0,
                                      &edit_state->cursor, &edit_state->mark, edit_state->input_buffer, sizeof(edit_state->input_buffer), &edit_state->input_size, &next_expanded,
                                      row->display_expr,
                                      "###row_%I64x", row_hash);
                }
                // TODO(rjf): edit_commit = edit_commit || ui_committed(sig);
                if(is_inherited && ui_hovering(sig)) UI_Tooltip
                {
                  String8List inheritance_chain_type_names = {0};
                  for(TG_KeyNode *n = row->inherited_type_key_chain.first; n != 0; n = n->next)
                  {
                    String8 inherited_type_name = tg_string_from_key(scratch.arena, parse_ctx.type_graph, parse_ctx.rdi, n->v);
                    inherited_type_name = str8_skip_chop_whitespace(inherited_type_name);
                    str8_list_push(scratch.arena, &inheritance_chain_type_names, inherited_type_name);
                  }
                  StringJoin join = {0};
                  join.sep = str8_lit("::");
                  String8 inheritance_type = str8_list_join(scratch.arena, &inheritance_chain_type_names, &join);
                  ui_set_next_pref_width(ui_children_sum(1));
                  UI_Row
                  {
                    ui_labelf("Inherited from ");
                    UI_Font(df_font_from_slot(DF_FontSlot_Code)) df_code_label(1.f, 1.f, df_rgba_from_theme_color(DF_ThemeColor_CodeType), inheritance_type);
                  }
                }
                if(DEV_eval_watch_key_tooltips && ui_hovering(sig)) UI_Tooltip UI_Font(df_font_from_slot(DF_FontSlot_Code))
                {
                  ui_labelf("Parent Key:   %I64x, %I64x", row->parent_key.parent_hash, row->parent_key.child_num);
                  ui_labelf("Hover Key:    %I64x, %I64x", row->key.parent_hash, row->key.child_num);
                  ui_labelf("Cursor Key:   %I64x, %I64x", ewv->cursor.key.parent_hash, ewv->cursor.key.child_num);
                }
                if(DEV_eval_compiler_tooltips && row->depth == 0 && ui_hovering(sig)) UI_Tooltip
                {
                  Temp scratch = scratch_begin(0, 0);
                  String8 string = row->display_expr;
                  
                  // rjf: lex & parse
                  EVAL_TokenArray tokens = eval_token_array_from_text(scratch.arena, string);
                  EVAL_ParseResult parse = eval_parse_expr_from_text_tokens(scratch.arena, &parse_ctx, string, &tokens);
                  EVAL_ErrorList errors = parse.errors;
                  ui_labelf("Tokens:");
                  UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
                    for(U64 idx = 0; idx < tokens.count; idx += 1)
                  {
                    EVAL_Token *token = tokens.v+idx;
                    String8 token_string = str8_substr(string, token->range);
                    String8 token_kind_name = str8_lit("Token");
                    switch(token->kind)
                    {
                      default:break;
                      case EVAL_TokenKind_Identifier:   {token_kind_name = str8_lit("Identifier");}break;
                      case EVAL_TokenKind_Numeric:      {token_kind_name = str8_lit("Numeric");}break;
                      case EVAL_TokenKind_StringLiteral:{token_kind_name = str8_lit("StringLiteral");}break;
                      case EVAL_TokenKind_CharLiteral:  {token_kind_name = str8_lit("CharLiteral");}break;
                      case EVAL_TokenKind_Symbol:       {token_kind_name = str8_lit("Symbol");}break;
                    }
                    ui_labelf("%S -> \"%S\"", token_kind_name, token_string);
                  }
                  
                  // rjf: produce IR tree & type
                  EVAL_IRTreeAndType ir_tree_and_type = {&eval_irtree_nil};
                  if(parse.expr != &eval_expr_nil && errors.count == 0)
                  {
                    ui_labelf("Type:");
                    ir_tree_and_type = eval_irtree_and_type_from_expr(scratch.arena, parse_ctx.type_graph, parse_ctx.rdi, &eval_string2expr_map_nil, parse.expr, &errors);
                    TG_Key type_key = ir_tree_and_type.type_key;
                    String8 type_string = tg_string_from_key(scratch.arena, parse_ctx.type_graph, parse_ctx.rdi, type_key);
                    UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
                      ui_label(type_string);
                  }
                  
                  scratch_end(scratch);
                }
                
                // rjf: autocomplete lister
                if(expr_editing_active &&
                   selection_tbl.min.x == selection_tbl.max.x && selection_tbl.min.y == selection_tbl.max.y &&
                   txt_pt_match(edit_state->cursor, edit_state->mark))
                {
                  String8 input = str8(edit_state->input_buffer, edit_state->input_size);
                  DF_AutoCompListerParams params = {DF_AutoCompListerFlag_Locals};
                  df_set_autocomp_lister_query(ws, sig.box->key, ctrl_ctx, &params, input, edit_state->cursor.column-1);
                }
                
#if 0 // TODO(rjf):
                if(expr_editing_active && !edit_end && txt_pt_match(ewv->input_cursor, ewv->input_mark))
                {
                  String8 input = str8(ewv->input_buffer, ewv->input_size);
                  DF_AutoCompListerParams params = {DF_AutoCompListerFlag_Locals};
                  df_set_autocomp_lister_query(ws, sig.box->key, ctrl_ctx, &params, input, ewv->input_cursor.column-1);
                }
#endif
              }
              
              // rjf: press -> commit if editing & select
#if 0 // TODO(rjf):
              if(ui_pressed(sig))
              {
                edit_commit = edit_commit || (!cell_selected && ewv->text_editing);
                next_cursor_tbl = v2s64(DF_WatchViewColumnKind_Expr, (semantic_idx+1));
                pressed = 1;
              }
#endif
              
              // rjf: keyboard-click & expandable -> expand
#if 0// TODO(rjf):
              if(cell_selected && edit_begin_or_expand && row->flags & DF_EvalVizRowFlag_CanExpand)
              {
                next_expanded ^= 1;
              }
#endif
              
              // rjf: double-click -> start editing
              // TODO(rjf):
#if 0
              if(ui_double_clicked(sig) && !ewv->text_editing && can_edit_expr)
              {
                ui_kill_action();
                ewv->text_editing = 1;
                ewv->input_size = Min(sizeof(ewv->input_buffer), row->display_expr.size);
                MemoryCopy(ewv->input_buffer, row->display_expr.str, ewv->input_size);
                ewv->input_cursor = txt_pt(1, 1+ewv->input_size);
                ewv->input_mark = txt_pt(1, 1);
              }
#endif
              
              // rjf: commit expansion state
              if(next_expanded != row_expanded)
              {
                df_expand_set_expansion(eval_view->arena, &eval_view->expand_tree_table, row->parent_key, row->key, next_expanded);
              }
            }
            
            //- rjf: value
            ProfScope("value")
            {
              DF_WatchViewPoint pt = {DF_WatchViewColumnKind_Value, row->parent_key, row->key};
              DF_WatchViewTextEditState *edit_state = df_watch_view_text_edit_state_from_pt(ewv, pt);
              B32 cell_selected = (row_selected && selection_tbl.min.x <= pt.column_kind && pt.column_kind <= selection_tbl.max.x);
              B32 value_is_error   = (row->eval.errors.count != 0);
              B32 value_is_hook    = (!value_is_error && row->value_ui_rule_spec != &df_g_nil_gfx_view_rule_spec && row->value_ui_rule_spec != 0);
              B32 value_is_complex = (!value_is_error && !value_is_hook && !(row->flags & DF_EvalVizRowFlag_CanEditValue));
              B32 value_is_simple  = (!value_is_error && !value_is_hook &&  (row->flags & DF_EvalVizRowFlag_CanEditValue));
              
              // rjf: begin editing
#if 0 // TODO(rjf):
              if(cell_selected && (edit_begin || edit_begin_or_expand) && value_is_simple)
              {
                ewv->text_editing = 1;
                ewv->input_size = Min(sizeof(ewv->input_buffer), row->edit_value.size);
                MemoryCopy(ewv->input_buffer, row->edit_value.str, ewv->input_size);
                ewv->input_cursor = txt_pt(1, 1+ewv->input_size);
                ewv->input_mark = txt_pt(1, 1);
              }
#endif
              
              // rjf: build
              UI_Signal sig = {0};
              if(row_is_bad)
              {
                ui_set_next_flags(disabled_flags|UI_BoxFlag_DrawOverlay);
                ui_set_next_overlay_color(mul_4f32(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground), v4f32(1, 1, 1, 0.2f)));
              }
              UI_TableCell UI_Font(code_font)
                UI_FocusHot(cell_selected ? UI_FocusKind_On : UI_FocusKind_Off)
                UI_FocusActive((cell_selected && ewv->text_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
              {
                // rjf: errors? -> show errors
                if(value_is_error) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground)) UI_Font(df_font_from_slot(DF_FontSlot_Main))
                {
                  String8List strings = {0};
                  for(EVAL_Error *error = row->eval.errors.first; error != 0; error = error->next)
                  {
                    str8_list_push(scratch.arena, &strings, error->text);
                  }
                  StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
                  String8 error_string = str8_list_join(scratch.arena, &strings, &join);
                  sig = df_error_label(error_string);
                }
                
                // rjf: hook -> call hook
                if(value_is_hook) UI_Font(df_font_from_slot(DF_FontSlot_Main))
                {
                  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_Clickable, "###val_%I64x", row_hash);
                  UI_Parent(box)
                  {
                    row->value_ui_rule_spec->info.row_ui(row->key, row->eval, scope, &ctrl_ctx, &parse_ctx, &macro_map, row->value_ui_rule_node);
                  }
                  sig = ui_signal_from_box(box);
                }
                
                // rjf: complex values
                if(value_is_complex)
                {
                  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_Clickable, "###val_%I64x", row_hash);
                  UI_Parent(box)
                  {
                    df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), row->display_value);
                  }
                  sig = ui_signal_from_box(box);
                }
                
                // rjf: simple values (editable)
                if(value_is_simple)
                {
                  sig = df_line_editf(DF_LineEditFlag_CodeContents|DF_LineEditFlag_NoBackground, 0, 0, &edit_state->cursor, &edit_state->mark, edit_state->input_buffer, sizeof(edit_state->input_buffer), &edit_state->input_size, 0, row->display_value, "%S###val_%I64x", row->display_value, row_hash);
                  // TODO(rjf): edit_commit = (edit_commit || ui_committed(sig));
                }
              }
              
              // rjf: bad & hovering -> display
              if(row_is_bad && ui_hovering(sig)) UI_Tooltip
              {
                UI_PrefWidth(ui_children_sum(1)) df_error_label(str8_lit("Could not read process memory successfully."));
              }
              
              // rjf: press -> focus & commit if editing & not selected
              if(ui_pressed(sig))
              {
                pressed = 1;
                // TODO(rjf): edit_commit = edit_commit || (ewv->text_editing && !cell_selected);
                next_cursor_tbl = v2s64(DF_WatchViewColumnKind_Value, (semantic_idx+1));
              }
              
              // rjf: double-click -> start editing
#if 0
              if(ui_double_clicked(sig) && value_is_simple)
              {
                ui_kill_action();
                ewv->text_editing = 1;
                ewv->input_size = Min(sizeof(ewv->input_buffer), row->edit_value.size);
                MemoryCopy(ewv->input_buffer, row->edit_value.str, ewv->input_size);
                ewv->input_cursor = txt_pt(1, 1+ewv->input_size);
                ewv->input_mark = txt_pt(1, 1);
              }
#endif
            }
            
            //- rjf: type
            ProfScope("type")
            {
              DF_WatchViewPoint pt = {DF_WatchViewColumnKind_Type, row->parent_key, row->key};
              DF_WatchViewTextEditState *edit_state = df_watch_view_text_edit_state_from_pt(ewv, pt);
              B32 cell_selected = (row_selected && selection_tbl.min.x <= pt.column_kind && pt.column_kind <= selection_tbl.max.x);
              UI_TableCell UI_Font(code_font)
                UI_FocusHot(cell_selected ? UI_FocusKind_On : UI_FocusKind_Off)
                UI_FocusActive((cell_selected && ewv->text_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
              {
                TG_Key key = row->eval.type_key;
                String8 string = tg_string_from_key(scratch.arena, parse_ctx.type_graph, parse_ctx.rdi, key);
                string = str8_skip_chop_whitespace(string);
                UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_Clickable, "###type_%I64x", row_hash);
                if(!tg_key_match(key, tg_key_zero())) UI_Parent(box)
                {
                  df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeType), string);
                }
                UI_Signal sig = ui_signal_from_box(box);
                if(ui_pressed(sig))
                {
                  pressed = 1;
                  // TODO(rjf): edit_commit = edit_commit || (ewv->text_editing && !cell_selected);
                  next_cursor_tbl = v2s64(DF_WatchViewColumnKind_Type, (semantic_idx+1));
                }
              }
            }
            
            //- rjf: view rule
            ProfScope("view rule")
            {
              DF_WatchViewPoint pt = {DF_WatchViewColumnKind_ViewRule, row->parent_key, row->key};
              DF_WatchViewTextEditState *edit_state = df_watch_view_text_edit_state_from_pt(ewv, pt);
              B32 cell_selected = (row_selected && selection_tbl.min.x <= pt.column_kind && pt.column_kind <= selection_tbl.max.x);
              String8 view_rule = df_eval_view_rule_from_key(eval_view, row->key);
              
              // rjf: begin editing
#if 0 // TODO(rjf):
              if(cell_selected && (edit_begin || edit_begin_or_expand))
              {
                ewv->text_editing = 1;
                ewv->input_size = Min(sizeof(ewv->input_buffer), view_rule.size);
                MemoryCopy(ewv->input_buffer, view_rule.str, ewv->input_size);
                ewv->input_cursor = txt_pt(1, 1+ewv->input_size);
                ewv->input_mark = txt_pt(1, 1);
              }
#endif
              
              // rjf: build
              UI_Signal sig = {0};
              B32 rule_editing_active = 0;
              UI_TableCell UI_Font(code_font)
                UI_FocusHot(cell_selected ? UI_FocusKind_On : UI_FocusKind_Off)
                UI_FocusActive((cell_selected && ewv->text_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
              {
                rule_editing_active = ui_is_focus_active();
                sig = df_line_editf(DF_LineEditFlag_CodeContents|DF_LineEditFlag_NoBackground, 0, 0, &edit_state->cursor, &edit_state->mark, edit_state->input_buffer, sizeof(edit_state->input_buffer), &edit_state->input_size, 0, view_rule, "###view_rule_%I64x", row_hash);
                // TODO(rjf): edit_commit = edit_commit || ui_committed(sig);
              }
              
              // rjf: press -> commit if not selected, select this cell
              if(ui_pressed(sig))
              {
                pressed = 1;
                // TODO(rjf): edit_commit = edit_commit || (ewv->text_editing && !cell_selected);
                next_cursor_tbl = v2s64(DF_WatchViewColumnKind_ViewRule, (semantic_idx+1));
              }
              
              // rjf: autocomplete lister
              if(rule_editing_active &&
                 selection_tbl.min.x == selection_tbl.max.x && selection_tbl.min.y == selection_tbl.max.y &&
                 txt_pt_match(edit_state->cursor, edit_state->mark))
              {
                String8 input = str8(edit_state->input_buffer, edit_state->input_size);
                DF_AutoCompListerParams params = df_view_rule_autocomp_lister_params_from_input_cursor(scratch.arena, input, edit_state->cursor.column-1);
                if(params.flags == 0)
                {
                  params.flags = DF_AutoCompListerFlag_ViewRules;
                }
                df_set_autocomp_lister_query(ws, sig.box->key, ctrl_ctx, &params, input, edit_state->cursor.column-1);
              }
              
              // rjf: double-click -> begin editing
              // TODO(rjf):
#if 0
              if(ui_double_clicked(sig) && !ewv->text_editing)
              {
                ui_kill_action();
                ewv->text_editing = 1;
                ewv->input_size = Min(sizeof(ewv->input_buffer), view_rule.size);
                MemoryCopy(ewv->input_buffer, view_rule.str, ewv->input_size);
                ewv->input_cursor = txt_pt(1, 1+ewv->input_size);
                ewv->input_mark = txt_pt(1, 1);
              }
#endif
              
              // rjf: autocomplete lister
              // TODO(rjf):
#if 0
              if(rule_editing_active && !edit_end && txt_pt_match(ewv->input_cursor, ewv->input_mark))
              {
                String8 input = str8(ewv->input_buffer, ewv->input_size);
                DF_AutoCompListerParams params = df_view_rule_autocomp_lister_params_from_input_cursor(scratch.arena, input, ewv->input_cursor.column-1);
                if(params.flags == 0)
                {
                  params.flags = DF_AutoCompListerFlag_ViewRules;
                }
                df_set_autocomp_lister_query(ws, sig.box->key, ctrl_ctx, &params, input, ewv->input_cursor.column-1);
              }
#endif
            }
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: general table-wide press logic
  //
  if(pressed)
  {
    DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
    df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
  }
  
  //////////////////////////////
  //- rjf: commit edits
  //
#if 0
  {
    DF_WatchViewColumnKind commit_column = (DF_WatchViewColumnKind)cursor_tbl.x;
    if(!MemoryMatchStruct(&cursor_tbl, &next_cursor_tbl))
    {
      mark_tbl = next_cursor_tbl;
    }
    cursor_tbl = next_cursor_tbl;
    if(commit_row != 0 && edit_commit)
    {
      ewv->text_editing = 0;
      String8 commit_string = str8(ewv->input_buffer, ewv->input_size);
      switch(commit_column)
      {
        default:break;
        
        //- rjf: expression commits
        case DF_WatchViewColumnKind_Expr: if(modifiable)
        {
          if(commit_string.size == 0)
          {
            DF_EvalRoot *root = df_eval_root_from_expand_key(ewv, eval_view, commit_row->key);
            if(root != 0)
            {
              df_eval_root_release(ewv, root);
            }
          }
          else
          {
            DF_EvalRoot *root = df_eval_root_from_expand_key(ewv, eval_view, commit_row->key);
            if(!root && df_expand_key_match(commit_row->key, empty_row_key))
            {
              root = df_eval_root_alloc(view, ewv);
              DF_ExpandKey parent_key = df_parent_expand_key_from_eval_root(root);
              DF_ExpandKey key = df_expand_key_from_eval_root(root);
              df_expand_set_expansion(eval_view->arena, &eval_view->expand_tree_table, parent_key, key, 0);
              df_eval_view_set_key_rule(eval_view, key, str8_lit(""));
            }
            if(root != 0)
            {
              df_eval_root_equip_string(root, commit_string);
            }
          }
        }break;
        
        //- rjf: value commits
        case DF_WatchViewColumnKind_Value:
        {
          Temp scratch = scratch_begin(0, 0);
          DF_Eval write_eval = df_eval_from_string(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, commit_string);
          B32 success = df_commit_eval_value(parse_ctx.type_graph, parse_ctx.rdi, &ctrl_ctx, commit_row->eval, write_eval);
          if(success == 0)
          {
            DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
            params.string = str8_lit("Could not commit value successfully.");
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
          scratch_end(scratch);
        }break;
        
        //- rjf: type commits
        case DF_WatchViewColumnKind_Type:
        {
        }break;
        
        //- rjf: view rule commits
        case DF_WatchViewColumnKind_ViewRule:
        {
          df_eval_view_set_key_rule(eval_view, commit_row->key, commit_string);
        }break;
      }
      if(edit_submit && commit_string.size != 0)
      {
        cursor_tbl.y += 1;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: end edits
  //
  if(edit_end)
  {
    ewv->text_editing = 0;
  }
  
  //////////////////////////////
  //- rjf: commits occurred -> re-compute blocks to adjust to new state
  //
  if(edit_commit)
  {
    blocks = df_eval_viz_block_list_from_watch_view_state(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &macro_map, view, ewv);
    if(modifiable)
    {
      DF_EvalVizBlock *b = df_eval_viz_block_begin(scratch.arena, DF_EvalVizBlockKind_Null, empty_row_parent_key, empty_row_key, 0);
      b->visual_idx_range = b->semantic_idx_range = r1u64(0, 1);
      df_eval_viz_block_end(&blocks, b);
    }
  }
  
  //////////////////////////////
  //- rjf: convert new table coordinates back to selection state
  //
  struct
  {
    DF_WatchViewPoint *pt_state;
    Vec2S64 pt_tbl;
  }
  points[] =
  {
    {&ewv->cursor, cursor_tbl},
    {&ewv->mark, mark_tbl},
  };
  for(U64 point_idx = 0; point_idx < ArrayCount(points); point_idx += 1)
  {
    DF_ExpandKey last_key = points[point_idx].pt_state->key;
    DF_ExpandKey last_parent_key = points[point_idx].pt_state->parent_key;
    points[point_idx].pt_state->column_kind= (DF_WatchViewColumnKind)points[point_idx].pt_tbl.x;
    points[point_idx].pt_state->key        = df_key_from_viz_block_list_row_num(&blocks, points[point_idx].pt_tbl.y);
    points[point_idx].pt_state->parent_key = df_parent_key_from_viz_block_list_row_num(&blocks, points[point_idx].pt_tbl.y);
    if(df_expand_key_match(df_expand_key_zero(), points[point_idx].pt_state->key))
    {
      points[point_idx].pt_state->key = last_parent_key;
      DF_ExpandNode *node = df_expand_node_from_key(&eval_view->expand_tree_table, last_parent_key);
      for(DF_ExpandNode *n = node; n != 0; n = n->parent)
      {
        points[point_idx].pt_state->key = n->key;
        if(n->expanded == 0)
        {
          break;
        }
      }
    }
    if(point_idx == 0 &&
       (!df_expand_key_match(ewv->cursor.key, last_key) ||
        !df_expand_key_match(ewv->cursor.parent_key, last_parent_key)))
    {
      ewv->text_editing = 0;
    }
  }
#endif
  
  scratch_end(scratch);
  dbgi_scope_close(scope);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Null @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Null) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Null) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Null) {}
DF_VIEW_UI_FUNCTION_DEF(Null) {}

////////////////////////////////
//~ rjf: Empty @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Empty) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Empty) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Empty) {}
DF_VIEW_UI_FUNCTION_DEF(Empty)
{
  ui_set_next_flags(UI_BoxFlag_DefaultFocusNav);
  UI_Focus(UI_FocusKind_On) UI_WidthFill UI_HeightFill UI_NamedColumn(str8_lit("empty_view")) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    UI_Padding(ui_pct(1, 0)) UI_Focus(UI_FocusKind_Null)
  {
    UI_PrefHeight(ui_em(3.f, 1.f))
      UI_Row
      UI_Padding(ui_pct(1, 0))
      UI_TextAlignment(UI_TextAlign_Center)
      UI_PrefWidth(ui_em(15.f, 1.f))
      UI_CornerRadius(ui_top_font_size()/2.f)
      UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
      UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBorder))
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureText))
    {
      if(ui_clicked(df_icon_buttonf(DF_IconKind_X, 0, "Close Panel")))
      {
        DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
        df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ClosePanel));
      }
    }
  }
}

////////////////////////////////
//~ rjf: GettingStarted @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(GettingStarted) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(GettingStarted) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(GettingStarted) {}
DF_VIEW_UI_FUNCTION_DEF(GettingStarted)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  ui_set_next_flags(UI_BoxFlag_DefaultFocusNav);
  UI_Focus(UI_FocusKind_On) UI_WidthFill UI_HeightFill UI_NamedColumn(str8_lit("empty_view")) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    UI_Padding(ui_pct(1, 0)) UI_Focus(UI_FocusKind_Null)
  {
    DF_EntityList targets = df_push_active_target_list(scratch.arena);
    DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
    
    //- rjf: icon & info
    UI_Padding(ui_em(2.f, 1.f))
    {
      //- rjf: icon
      {
        F32 icon_dim = ui_top_font_size()*10.f;
        UI_PrefHeight(ui_px(icon_dim, 1.f))
          UI_Row
          UI_Padding(ui_pct(1, 0))
          UI_PrefWidth(ui_px(icon_dim, 1.f))
        {
          R_Handle texture = df_gfx_state->icon_texture;
          Vec2S32 texture_dim = r_size_from_tex2d(texture);
          ui_image(texture, R_Tex2DSampleKind_Linear, r2f32p(0, 0, texture_dim.x, texture_dim.y), v4f32(1, 1, 1, 1), 0, str8_lit(""));
        }
      }
      
      //- rjf: info
      UI_Padding(ui_em(2.f, 1.f))
        UI_WidthFill UI_PrefHeight(ui_em(2.f, 1.f))
        UI_Row
        UI_Padding(ui_pct(1, 0))
        UI_TextAlignment(UI_TextAlign_Center)
        UI_PrefWidth(ui_text_dim(10, 1))
      {
        ui_label(str8_lit(BUILD_TITLE_STRING_LITERAL));
      }
    }
    
    //- rjf: targets state dependent helper
    B32 helper_built = 0;
    if(processes.count == 0)
    {
      helper_built = 1;
      switch(targets.count)
      {
        //- rjf: user has no targets. build helper for adding them
        case 0:
        {
          UI_PrefHeight(ui_em(3.75f, 1.f))
            UI_Row
            UI_Padding(ui_pct(1, 0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_PrefWidth(ui_em(22.f, 1.f))
            UI_CornerRadius(ui_top_font_size()/2.f)
            UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBackground))
            UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBorder))
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_ActionText))
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Add, 0, "Add Target")))
          {
            DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
            params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddTarget);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
          }
        }break;
        
        //- rjf: user has 1 target. build helper for launching it
        case 1:
        {
          DF_Entity *target = df_first_entity_from_list(&targets);
          String8 target_full_path = target->name;
          String8 target_name = str8_skip_last_slash(target_full_path);
          UI_PrefHeight(ui_em(3.75f, 1.f))
            UI_Row
            UI_Padding(ui_pct(1, 0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_PrefWidth(ui_em(22.f, 1.f))
            UI_CornerRadius(ui_top_font_size()/2.f)
            UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBackground))
            UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBorder))
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_ActionText))
          {
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Play, 0, "Launch %S", target_name)))
            {
              DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
              params.entity = df_handle_from_entity(target);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
            }
            ui_spacer(ui_em(1.5f, 1));
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Play, 0, "Step Into %S", target_name)))
            {
              DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
              params.entity = df_handle_from_entity(target);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndInit));
            }
          }
        }break;
        
        //- rjf: user has N targets.
        default:
        {
          helper_built = 0;
        }break;
      }
    }
    
    //- rjf: or text
    if(helper_built)
    {
      UI_PrefHeight(ui_em(2.25f, 1.f))
        UI_Row
        UI_Padding(ui_pct(1, 0))
        UI_TextAlignment(UI_TextAlign_Center)
        UI_WidthFill
        ui_labelf("- or -");
    }
    
    //- rjf: helper text for command lister activation
    UI_PrefHeight(ui_em(2.25f, 1.f)) UI_Row
      UI_PrefWidth(ui_text_dim(10, 1))
      UI_TextAlignment(UI_TextAlign_Center)
      UI_Padding(ui_pct(1, 0))
    {
      ui_labelf("use");
      DF_CmdSpec *spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand);
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText))
        UI_Flags(UI_BoxFlag_DrawBorder)
        UI_TextAlignment(UI_TextAlign_Center)
        df_cmd_binding_button(spec);
      ui_labelf("to open command menu");
    }
  }
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Commands @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Commands) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Commands) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Commands) {}
DF_VIEW_UI_FUNCTION_DEF(Commands)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: grab state
  typedef struct DF_CmdsViewState DF_CmdsViewState;
  struct DF_CmdsViewState
  {
    DF_CmdSpec *selected_cmd_spec;
  };
  DF_CmdsViewState *cv = df_view_user_state(view, DF_CmdsViewState);
  
  //- rjf: build filtered array of commands
  String8 query = str8(view->query_buffer, view->query_string_size);
  DF_CmdListerItemList cmd_list = df_cmd_lister_item_list_from_needle(scratch.arena, query);
  DF_CmdListerItemArray cmd_array = df_cmd_lister_item_array_from_list(scratch.arena, cmd_list);
  df_cmd_lister_item_array_sort_by_strength__in_place(cmd_array);
  
  //- rjf: submit best match when hitting enter w/ no selection
  if(cv->selected_cmd_spec == &df_g_nil_cmd_spec && ui_key_press(0, OS_Key_Return))
  {
    DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
    if(cmd_array.count > 0)
    {
      DF_CmdListerItem *item = &cmd_array.v[0];
      params.cmd_spec = item->cmd_spec;
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
    }
    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
  }
  
  //- rjf: selected kind -> cursor
  Vec2S64 cursor = {0};
  {
    for(U64 idx = 0; idx < cmd_array.count; idx += 1)
    {
      if(cmd_array.v[idx].cmd_spec == cv->selected_cmd_spec)
      {
        cursor.y = (S64)idx+1;
        break;
      }
    }
  }
  
  //- rjf: build contents
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = floor_f32(ui_top_font_size()*6.5f);
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(0, cmd_array.count));
    scroll_list_params.item_range    = r1s64(0, cmd_array.count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    //- rjf: build buttons
    for(S64 row_idx = visible_row_range.min;
        row_idx <= visible_row_range.max && row_idx < cmd_array.count;
        row_idx += 1)
    {
      DF_CmdListerItem *item = &cmd_array.v[row_idx];
      
      //- rjf: build context menu for this command
      UI_Key item_ctx_menu_key = ui_key_from_stringf(ui_key_zero(), "###%p_cmd_ctx_%p", view, item->cmd_spec);
      UI_CtxMenu(item_ctx_menu_key) UI_PrefWidth(ui_em(33, 1))
      {
        // rjf: row with icon & name
        UI_Row UI_PrefWidth(ui_text_dim(10, 1)) UI_PrefHeight(ui_pct(1, 0))
        {
          // rjf: icon
          DF_IconKind icon = item->cmd_spec->info.canonical_icon_kind;
          if(icon != DF_IconKind_Null)
            UI_TextAlignment(UI_TextAlign_Center)
            UI_Font(df_font_from_slot(DF_FontSlot_Icons))
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            UI_PrefWidth(ui_em(2.25f, 1.f))
          {
            ui_label(df_g_icon_kind_text_table[icon]);
          }
          
          // rjf: display name
          ui_label(item->cmd_spec->info.display_name);
        }
        
        // rjf: row with ipc syntax
        UI_Row UI_PrefWidth(ui_text_dim(10, 1)) UI_PrefHeight(ui_pct(1, 0))
        {
          // rjf: name
          UI_TextAlignment(UI_TextAlign_Left)
            UI_Font(df_font_from_slot(DF_FontSlot_Code))
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Code))
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            ui_label(item->cmd_spec->info.string);
        }
      }
      
      //- rjf: build row contents
      ui_set_next_hover_cursor(OS_Cursor_HandPoint);
      ui_set_next_child_layout_axis(Axis2_X);
      UI_Box *box = &ui_g_nil_box;
      UI_Focus(cursor.y == row_idx+1 ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                        UI_BoxFlag_DrawBorder|
                                        UI_BoxFlag_DrawBackground|
                                        UI_BoxFlag_DrawHotEffects|
                                        UI_BoxFlag_DrawActiveEffects,
                                        "###cmd_button_%p", item->cmd_spec);
      }
      UI_Parent(box) UI_PrefHeight(ui_em(1.65f, 1.f))
      {
        //- rjf: icon
        UI_PrefWidth(ui_em(3.f, 1.f))
          UI_HeightFill
          UI_Column
          UI_Font(df_font_from_slot(DF_FontSlot_Icons))
          UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
          UI_HeightFill
          UI_TextAlignment(UI_TextAlign_Center)
        {
          DF_IconKind icon = item->cmd_spec->info.canonical_icon_kind;
          if(icon != DF_IconKind_Null)
          {
            ui_label(df_g_icon_kind_text_table[icon]);
          }
        }
        
        //- rjf: name + description
        ui_set_next_pref_height(ui_pct(1, 0));
        UI_Column UI_Padding(ui_pct(1, 0))
        {
          F_Tag font = ui_top_font();
          F32 font_size = ui_top_font_size();
          F_Metrics font_metrics = f_metrics_from_tag_size(font, font_size);
          F32 font_line_height = f_line_height_from_metrics(&font_metrics);
          String8 cmd_display_name = item->cmd_spec->info.display_name;
          String8 cmd_desc = item->cmd_spec->info.description;
          UI_Box *name_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%S##name_%p", cmd_display_name, item->cmd_spec);
          UI_Box *desc_box = &ui_g_nil_box;
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefHeight(ui_em(1.8f, 1.f))
          {
            desc_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%S##desc_%p", cmd_desc, item->cmd_spec);
          }
          ui_box_equip_fuzzy_match_ranges(name_box, &item->name_match_ranges);
          ui_box_equip_fuzzy_match_ranges(desc_box, &item->desc_match_ranges);
        }
        
        //- rjf: binding
        UI_PrefWidth(ui_pct(0.15f, 1.f)) UI_HeightFill UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
        {
          df_cmd_binding_button(item->cmd_spec);
        }
      }
      
      //- rjf: interact
      UI_Signal sig = ui_signal_from_box(box);
      if(ui_clicked(sig))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.cmd_spec = item->cmd_spec;
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
      }
      if(ui_right_clicked(sig))
      {
        ui_ctx_menu_open(item_ctx_menu_key, ui_key_zero(), sub_2f32(ui_mouse(), v2f32(2, 2)));
      }
    }
  }
  
  //- rjf: map selected num -> selected kind
  if(1 <= cursor.y && cursor.y <= cmd_array.count)
  {
    cv->selected_cmd_spec = cmd_array.v[cursor.y-1].cmd_spec;
  }
  else
  {
    cv->selected_cmd_spec = &df_g_nil_cmd_spec;
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: FileSystem @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(FileSystem)
{
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(FileSystem)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(FileSystem)
{
}

DF_VIEW_UI_FUNCTION_DEF(FileSystem)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  String8 query = str8(view->query_buffer, view->query_string_size);
  String8 query_normalized = path_normalized_from_string(scratch.arena, query);
  B32 query_has_slash = (query.size != 0 && char_to_correct_slash(query.str[query.size-1]) == '/');
  String8 query_normalized_with_opt_slash = push_str8f(scratch.arena, "%S%s", query_normalized, query_has_slash ? "/" : "");
  DF_PathQuery path_query = df_path_query_from_string(query_normalized_with_opt_slash);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  B32 file_selection = !!(ws->query_cmd_spec->info.query.flags & DF_CmdQueryFlag_AllowFiles);
  B32 dir_selection = !!(ws->query_cmd_spec->info.query.flags & DF_CmdQueryFlag_AllowFolders);
  
  //- rjf: get extra state for this view
  DF_FileSystemViewState *fs = df_view_user_state(view, DF_FileSystemViewState);
  if(fs->initialized == 0)
  {
    fs->initialized = 1;
    fs->path_state_table_size = 256;
    fs->path_state_table = push_array(view->arena, DF_FileSystemViewPathState *, fs->path_state_table_size);
    fs->cached_files_arena = df_view_push_arena_ext(view);
    fs->col_pcts[0] = 0.60f;
    fs->col_pcts[1] = 0.20f;
    fs->col_pcts[2] = 0.20f;
  }
  
  //- rjf: grab state for the current path
  DF_FileSystemViewPathState *ps = 0;
  {
    String8 key = query_normalized;
    U64 hash = df_hash_from_string(key);
    U64 slot = hash % fs->path_state_table_size;
    for(DF_FileSystemViewPathState *p = fs->path_state_table[slot]; p != 0; p = p->hash_next)
    {
      if(str8_match(p->normalized_path, key, 0))
      {
        ps = p;
        break;
      }
    }
    if(ps == 0)
    {
      ps = push_array(view->arena, DF_FileSystemViewPathState, 1);
      ps->hash_next = fs->path_state_table[slot];
      fs->path_state_table[slot] = ps;
      ps->normalized_path = push_str8_copy(view->arena, key);
    }
  }
  
  //- rjf: get file array from the current path
  U64 file_count = fs->cached_file_count;
  DF_FileInfo *files = fs->cached_files;
  if(!str8_match(fs->cached_files_path, query_normalized_with_opt_slash, 0) ||
     fs->cached_files_sort_kind != fs->sort_kind ||
     fs->cached_files_sort_side != fs->sort_side)
  {
    arena_clear(fs->cached_files_arena);
    
    //- rjf: store off path that we're gathering from
    fs->cached_files_path = push_str8_copy(fs->cached_files_arena, query_normalized_with_opt_slash);
    fs->cached_files_sort_kind = fs->sort_kind;
    fs->cached_files_sort_side = fs->sort_side;
    
    //- rjf: use stored path as the new browse path for the whole frontend
    // (multiple file system views may conflict here. that's okay. we'll just always
    // choose the most recent change to a file browser path, and live with the
    // consequences).
    {
      DF_CmdParams p = df_cmd_params_zero();
      p.file_path = path_query.path;
      df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetCurrentPath));
    }
    
    //- rjf: get files, filtered
    U64 new_file_count = 0;
    DF_FileInfoNode *first_file = 0;
    DF_FileInfoNode *last_file = 0;
    {
      OS_FileIter *it = os_file_iter_begin(scratch.arena, path_query.path, 0);
      for(OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, it, &info);)
      {
        FuzzyMatchRangeList match_ranges = fuzzy_match_find(fs->cached_files_arena, path_query.search, info.name);
        B32 fits_search = (path_query.search.size == 0 || match_ranges.count == match_ranges.needle_part_count);
        B32 fits_dir_only = !!(info.props.flags & FilePropertyFlag_IsFolder) || !dir_selection;
        if(fits_search && fits_dir_only)
        {
          DF_FileInfoNode *node = push_array(scratch.arena, DF_FileInfoNode, 1);
          node->file_info.filename = push_str8_copy(fs->cached_files_arena, info.name);
          node->file_info.props = info.props;
          node->file_info.match_ranges = match_ranges;
          SLLQueuePush(first_file, last_file, node);
          new_file_count += 1;
        }
      }
      os_file_iter_end(it);
    }
    
    //- rjf: convert list to array
    DF_FileInfo *new_files = push_array(fs->cached_files_arena, DF_FileInfo, new_file_count);
    {
      U64 idx = 0;
      for(DF_FileInfoNode *n = first_file; n != 0; n = n->next, idx += 1)
      {
        new_files[idx] = n->file_info;
      }
    }
    
    //- rjf: apply sort
    switch(fs->sort_kind)
    {
      default:
      {
        if(path_query.search.size != 0)
        {
          qsort(new_files, new_file_count, sizeof(DF_FileInfo), (int (*)(const void *, const void *))df_qsort_compare_file_info__default_filtered);
        }
        else
        {
          qsort(new_files, new_file_count, sizeof(DF_FileInfo), (int (*)(const void *, const void *))df_qsort_compare_file_info__default);
        }
      }break;
      case DF_FileSortKind_Filename:
      {
        qsort(new_files, new_file_count, sizeof(DF_FileInfo), (int (*)(const void *, const void *))df_qsort_compare_file_info__filename);
      }break;
      case DF_FileSortKind_LastModified:
      {
        qsort(new_files, new_file_count, sizeof(DF_FileInfo), (int (*)(const void *, const void *))df_qsort_compare_file_info__last_modified);
      }break;
      case DF_FileSortKind_Size:
      {
        qsort(new_files, new_file_count, sizeof(DF_FileInfo), (int (*)(const void *, const void *))df_qsort_compare_file_info__size);
      }break;
    }
    
    //- rjf: apply reverse
    if(fs->sort_kind != DF_FileSortKind_Null && fs->sort_side == Side_Max)
    {
      for(U64 idx = 0; idx < new_file_count/2; idx += 1)
      {
        U64 rev_idx = new_file_count - idx - 1;
        Swap(DF_FileInfo, new_files[idx], new_files[rev_idx]);
      }
    }
    
    fs->cached_file_count = file_count = new_file_count;
    fs->cached_files = files = new_files;
  }
  
  //- rjf: submit best match when hitting enter w/ no selection
  if(ps->cursor.y == 0 && ui_key_press(0, OS_Key_Return))
  {
    FileProperties query_normalized_with_opt_slash_props = os_properties_from_file_path(query_normalized_with_opt_slash);
    FileProperties path_query_path_props = os_properties_from_file_path(path_query.path);
    
    // rjf: command search part is empty, but directory matches some file:
    if(path_query_path_props.created != 0 && path_query.search.size == 0)
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.file_path = query_normalized_with_opt_slash;
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
    }
    
    // rjf: command argument exactly matches some file:
    else if(query_normalized_with_opt_slash_props.created != 0 && path_query.search.size != 0)
    {
      // rjf: is a folder -> autocomplete to slash
      if(query_normalized_with_opt_slash_props.flags & FilePropertyFlag_IsFolder)
      {
        String8 new_path = push_str8f(scratch.arena, "%S%S/", path_query.path, path_query.search);
        df_view_equip_spec(ws, view, view->spec, df_entity_from_handle(view->entity), new_path, &df_g_nil_cfg_node);
      }
      
      // rjf: is a file -> complete view
      else
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.file_path = query_normalized_with_opt_slash;
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
      }
    }
    
    // rjf: command argument is empty, picking folders -> use current folder
    else if(path_query.search.size == 0 && dir_selection)
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.file_path = path_query.path;
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
    }
    
    // rjf: command argument does not exactly match any file, but lister results are in:
    else if(file_count != 0)
    {
      String8 filename = files[0].filename;
      if(files[0].props.flags & FilePropertyFlag_IsFolder)
      {
        String8 existing_path = str8_chop_last_slash(path_query.path);
        String8 new_path = push_str8f(scratch.arena, "%S/%S/", existing_path, files[0].filename);
        df_view_equip_spec(ws, view, view->spec, df_entity_from_handle(view->entity), new_path, &df_g_nil_cfg_node);
      }
      else
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.file_path = push_str8f(scratch.arena, "%S%S", path_query.path, filename);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
      }
    }
    
    // rjf: command argument does not match any file, and lister is empty (new file)
    else
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
    }
  }
  
  //- rjf: build non-scrolled table header
  U64 row_num = 1;
  F32 **col_pcts = push_array(scratch.arena, F32 *, ArrayCount(fs->col_pcts));
  for(U64 idx = 0; idx < ArrayCount(fs->col_pcts); idx += 1)
  {
    col_pcts[idx] = &fs->col_pcts[idx];
  }
  UI_PrefHeight(ui_px(row_height_px, 1)) UI_Focus(UI_FocusKind_Off) UI_TableF(ArrayCount(fs->col_pcts), col_pcts, "###fs_tbl")
  {
    UI_TableVector UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      struct
      {
        DF_FileSortKind kind;
        String8 string;
      }
      kinds[] =
      {
        { DF_FileSortKind_Filename,     str8_lit_comp("Filename") },
        { DF_FileSortKind_LastModified, str8_lit_comp("Last Modified") },
        { DF_FileSortKind_Size,         str8_lit_comp("Size") },
      };
      for(U64 idx = 0; idx < ArrayCount(kinds); idx += 1)
      {
        B32 sorting = fs->sort_kind == kinds[idx].kind;
        if(sorting)
        {
          ui_push_text_color(df_rgba_from_theme_color(DF_ThemeColor_PlainText));
        }
        UI_TableCell
        {
          UI_Signal sig = ui_sort_header(sorting,
                                         fs->cached_files_sort_side == Side_Min,
                                         kinds[idx].string);
          if(ui_clicked(sig))
          {
            if(fs->sort_kind != kinds[idx].kind)
            {
              fs->sort_kind = kinds[idx].kind;
              fs->sort_side = Side_Max;
            }
            else if(fs->sort_kind == kinds[idx].kind && fs->sort_side == Side_Max)
            {
              fs->sort_side = Side_Min;
            }
            else if(fs->sort_kind == kinds[idx].kind && fs->sort_side == Side_Min)
            {
              fs->sort_kind = DF_FileSortKind_Null;
            }
          }
        }
        if(sorting)
        {
          ui_pop_text_color();
        }
      }
    }
  }
  
  //- rjf: build file list
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    Vec2F32 content_dim = dim_2f32(rect);
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = v2f32(content_dim.x, content_dim.y-row_height_px);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(0, file_count+1));
    scroll_list_params.item_range    = r1s64(0, file_count+1);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &ps->cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    // rjf: up-one-directory button (at idx 0)
    if(visible_row_range.min == 0)
    {
      // rjf: build
      UI_Signal sig = {0};
      UI_FocusHot(ps->cursor.y == row_num ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        sig = ui_buttonf("###up_one");
      }
      
      // rjf: make content
      UI_Parent(sig.box)
      {
        // rjf: icons
        UI_Font(df_font_from_slot(DF_FontSlot_Icons))
          UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
          UI_PrefWidth(ui_em(3.f, 1.f))
          UI_TextAlignment(UI_TextAlign_Center)
        {
          ui_label(df_g_icon_kind_text_table[DF_IconKind_LeftArrow]);
        }
        
        // rjf: text
        {
          ui_label(str8_lit("Up One Directory"));
        }
        
        row_num += 1;
      }
      
      // rjf: click => up one directory
      if(ui_clicked(sig))
      {
        String8 new_path = str8_chop_last_slash(str8_chop_last_slash(path_query.path));
        if(new_path.size != 0)
        {
          new_path = path_normalized_from_string(scratch.arena, new_path);
          String8 new_cmd = push_str8f(scratch.arena, "%S/", new_path);
          df_view_equip_spec(ws, view, view->spec, df_entity_from_handle(view->entity), new_cmd, &df_g_nil_cfg_node);
        }
      }
    }
    
    // rjf: file buttons
    for(U64 row_idx = Max(visible_row_range.min, 1);
        row_idx <= visible_row_range.max && row_idx <= file_count;
        row_idx += 1, row_num += 1)
    {
      U64 file_idx = row_idx-1;
      DF_FileInfo *file = &files[file_idx];
      B32 file_kb_focus = (ps->cursor.y == (row_idx+1));
      
      // rjf: make button
      UI_Signal file_sig = {0};
      UI_FocusHot(file_kb_focus ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        file_sig = ui_buttonf("##%S_%p", file->filename, view);
      }
      
      // rjf: make content
      UI_Parent(file_sig.box)
      {
        UI_PrefWidth(ui_pct(fs->col_pcts[0], 1)) UI_Row
        {
          // rjf: icon to signify directory
          UI_Font(df_font_from_slot(DF_FontSlot_Icons))
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_PrefWidth(ui_em(3.f, 1.f))
            UI_TextAlignment(UI_TextAlign_Center)
          {
            if(file->props.flags & FilePropertyFlag_IsFolder)
            {
              ui_label((ui_key_match(ui_hot_key(), file_sig.box->key) || file_kb_focus)
                       ? df_g_icon_kind_text_table[DF_IconKind_FolderOpenFilled]
                       : df_g_icon_kind_text_table[DF_IconKind_FolderClosedFilled]);
            }
            else
            {
              ui_label(df_g_icon_kind_text_table[DF_IconKind_FileOutline]);
            }
          }
          
          // rjf: filename
          UI_PrefWidth(ui_pct(1, 0))
          {
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%S##%p", file->filename, view);
            ui_box_equip_fuzzy_match_ranges(box, &file->match_ranges);
          }
        }
        
        // rjf: last-modified time
        UI_PrefWidth(ui_pct(fs->col_pcts[1], 1)) UI_Row
          UI_PrefWidth(ui_pct(1, 0))
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
        {
          DateTime time = date_time_from_dense_time(file->props.modified);
          DateTime time_local = os_local_time_from_universal_time(&time);
          String8 string = push_date_time_string(scratch.arena, &time_local);
          ui_label(string);
        }
        
        // rjf: file size
        UI_PrefWidth(ui_pct(fs->col_pcts[2], 1)) UI_Row
          UI_PrefWidth(ui_pct(1, 0))
        {
          if(file->props.size != 0)
          {
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_label(str8_from_memory_size(scratch.arena, file->props.size));
          }
        }
      }
      
      // rjf: click => activate this file
      if(ui_clicked(file_sig))
      {
        String8 existing_path = str8_chop_last_slash(path_query.path);
        String8 new_path = push_str8f(scratch.arena, "%S/%S/", existing_path, file->filename);
        new_path = path_normalized_from_string(scratch.arena, new_path);
        if(file->props.flags & FilePropertyFlag_IsFolder)
        {
          String8 new_cmd = push_str8f(scratch.arena, "%S/", new_path);
          df_view_equip_spec(ws, view, view->spec, df_entity_from_handle(view->entity), new_cmd, &df_g_nil_cfg_node);
        }
        else
        {
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
          params.file_path = new_path;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
        }
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: SystemProcesses  @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(SystemProcesses)
{
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(SystemProcesses)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(SystemProcesses)
{
}

DF_VIEW_UI_FUNCTION_DEF(SystemProcesses)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  
  //- rjf: grab state
  typedef struct DF_SystemProcessesViewState DF_SystemProcessesViewState;
  struct DF_SystemProcessesViewState
  {
    B32 initialized;
    B32 need_initial_gather;
    U32 selected_pid;
    Arena *cached_process_arena;
    String8 cached_process_arg;
    DF_ProcessInfoArray cached_process_array;
  };
  DF_SystemProcessesViewState *sp = df_view_user_state(view, DF_SystemProcessesViewState);
  if(sp->initialized == 0)
  {
    sp->initialized = 1;
    sp->need_initial_gather = 1;
    sp->cached_process_arena = df_view_push_arena_ext(view);
  }
  
  //- rjf: gather list of filtered process infos
  String8 query = str8(view->query_buffer, view->query_string_size);
  DF_ProcessInfoArray process_info_array = sp->cached_process_array;
  if(sp->need_initial_gather || !str8_match(sp->cached_process_arg, query, 0))
  {
    arena_clear(sp->cached_process_arena);
    sp->need_initial_gather = 0;
    sp->cached_process_arg = push_str8_copy(sp->cached_process_arena, query);
    DF_ProcessInfoList list = df_process_info_list_from_query(sp->cached_process_arena, query);
    sp->cached_process_array = df_process_info_array_from_list(sp->cached_process_arena, list);
    process_info_array = sp->cached_process_array;
    df_process_info_array_sort_by_strength__in_place(process_info_array);
  }
  
  //- rjf: submit best match when hitting enter w/ no selection
  if(sp->selected_pid == 0 && process_info_array.count > 0 && ui_key_press(0, OS_Key_Return))
  {
    DF_ProcessInfo *info = &process_info_array.v[0];
    DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
    params.id = info->info.pid;
    df_cmd_params_mark_slot(&params, DF_CmdParamSlot_ID);
    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
  }
  
  //- rjf: selected PID -> cursor
  Vec2S64 cursor = {0};
  {
    for(U64 idx = 0; idx < process_info_array.count; idx += 1)
    {
      if(process_info_array.v[idx].info.pid == sp->selected_pid)
      {
        cursor.y = idx+1;
        break;
      }
    }
  }
  
  //- rjf: build contents
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    Vec2F32 content_dim = dim_2f32(rect);
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = v2f32(content_dim.x, content_dim.y);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(0, process_info_array.count));
    scroll_list_params.item_range    = r1s64(0, process_info_array.count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    //- rjf: build rows
    for(U64 idx = visible_row_range.min;
        idx <= visible_row_range.max && idx < process_info_array.count;
        idx += 1)
    {
      DF_ProcessInfo *info = &process_info_array.v[idx];
      B32 is_attached = info->is_attached;
      UI_Signal sig = {0};
      UI_FocusHot(cursor.y == idx+1 ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        sig = ui_buttonf("###proc_%i", info->info.pid);
      }
      UI_Parent(sig.box)
      {
        // rjf: icon
        UI_Font(df_font_from_slot(DF_FontSlot_Icons))
          UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
          UI_PrefWidth(ui_em(3.f, 1.f))
          UI_TextAlignment(UI_TextAlign_Center)
        {
          ui_label(df_g_icon_kind_text_table[DF_IconKind_Threads]);
        }
        
        // rjf: attached indicator
        if(is_attached) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_Highlight1)) UI_PrefWidth(ui_text_dim(10, 1))
        {
          UI_Box *attached_label = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "[attached]##attached_label_%i", (int)info->info.pid);
          ui_box_equip_fuzzy_match_ranges(attached_label, &info->attached_match_ranges);
        }
        
        // rjf: process name
        UI_PrefWidth(ui_text_dim(10, 1))
        {
          UI_Box *name_label = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%S##name_label_%i", info->info.name, (int)info->info.pid);
          ui_box_equip_fuzzy_match_ranges(name_label, &info->name_match_ranges);
        }
        
        // rjf: process number
        UI_PrefWidth(ui_text_dim(1, 1)) UI_TextAlignment(UI_TextAlign_Center)
        {
          ui_labelf("[PID: ");
          UI_Box *pid_label = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%i##pid_label", info->info.pid);
          ui_box_equip_fuzzy_match_ranges(pid_label, &info->pid_match_ranges);
          ui_labelf("]");
        }
      }
      
      // rjf: click => activate this specific process
      if(ui_clicked(sig))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.id = info->info.pid;
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_ID);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
      }
    }
  }
  
  //- rjf: selected num -> selected PID
  {
    if(1 <= cursor.y && cursor.y <= process_info_array.count)
    {
      sp->selected_pid = process_info_array.v[cursor.y-1].info.pid;
    }
    else
    {
      sp->selected_pid = 0;
    }
  }
  
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: EntityLister @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(EntityLister)
{
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(EntityLister)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(EntityLister)
{
}

DF_VIEW_UI_FUNCTION_DEF(EntityLister)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_CmdSpec *spec = ws->query_cmd_spec;
  DF_EntityKind entity_kind = spec->info.query.entity_kind;
  DF_EntityFlags entity_flags_omit = DF_EntityFlag_IsFolder;
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  
  //- rjf: grab state
  typedef struct DF_EntityListerViewState DF_EntityListerViewState;
  struct DF_EntityListerViewState
  {
    DF_Handle selected_entity_handle;
  };
  DF_EntityListerViewState *fev = df_view_user_state(view, DF_EntityListerViewState);
  DF_Handle selected_entity_handle = fev->selected_entity_handle;
  DF_Entity *selected_entity = df_entity_from_handle(selected_entity_handle);
  
  //- rjf: build filtered array of entities
  String8 query = str8(view->query_buffer, view->query_string_size);
  DF_EntityListerItemList ent_list = df_entity_lister_item_list_from_needle(scratch.arena, entity_kind, entity_flags_omit, query);
  DF_EntityListerItemArray ent_arr = df_entity_lister_item_array_from_list(scratch.arena, ent_list);
  df_entity_lister_item_array_sort_by_strength__in_place(ent_arr);
  
  //- rjf: submit best match when hitting enter w/ no selection
  if(df_entity_is_nil(df_entity_from_handle(fev->selected_entity_handle)) && ent_arr.count != 0 && ui_key_press(0, OS_Key_Return))
  {
    DF_Entity *ent = ent_arr.v[0].entity;
    DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
    params.entity = df_handle_from_entity(ent);
    df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
    df_cmd_params_mark_slot(&params, DF_CmdParamSlot_EntityList);
    df_handle_list_push(scratch.arena, &params.entity_list, params.entity);
    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
  }
  
  //- rjf: selected entity -> cursor
  Vec2S64 cursor = {0};
  {
    for(U64 idx = 0; idx < ent_arr.count; idx += 1)
    {
      if(ent_arr.v[idx].entity == selected_entity)
      {
        cursor.y = (S64)(idx+1);
        break;
      }
    }
  }
  
  //- rjf: build list
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    Vec2F32 content_dim = dim_2f32(rect);
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = v2f32(content_dim.x, content_dim.y);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(0, ent_arr.count));
    scroll_list_params.item_range    = r1s64(0, ent_arr.count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    for(S64 idx = visible_row_range.min;
        idx <= visible_row_range.max && idx < ent_arr.count;
        idx += 1)
    {
      DF_EntityListerItem item = ent_arr.v[idx];
      DF_Entity *ent = item.entity;
      ui_set_next_hover_cursor(OS_Cursor_HandPoint);
      ui_set_next_child_layout_axis(Axis2_X);
      UI_Box *box = &ui_g_nil_box;
      UI_FocusHot(idx+1 == cursor.y ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                        UI_BoxFlag_DrawBorder|
                                        UI_BoxFlag_DrawBackground|
                                        UI_BoxFlag_DrawHotEffects|
                                        UI_BoxFlag_DrawActiveEffects,
                                        "###ent_btn_%p", ent);
      }
      UI_Parent(box)
      {
        DF_IconKind icon_kind = df_g_entity_kind_icon_kind_table[ent->kind];
        if(icon_kind != DF_IconKind_Null)
        {
          UI_TextAlignment(UI_TextAlign_Center)
            UI_Font(df_font_from_slot(DF_FontSlot_Icons))
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            UI_PrefWidth(ui_text_dim(10, 1))
            ui_label(df_g_icon_kind_text_table[icon_kind]);
        }
        String8 display_string = df_display_string_from_entity(scratch.arena, ent);
        Vec4F32 color = df_rgba_from_entity(ent);
        if(color.w != 0)
        {
          ui_set_next_text_color(color);
        }
        UI_Box *name_label = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%S##label_%p", display_string, ent);
        ui_box_equip_fuzzy_match_ranges(name_label, &item.name_match_ranges);
      }
      if(ui_clicked(ui_signal_from_box(box)))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.entity = df_handle_from_entity(ent);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_EntityList);
        df_handle_list_push(scratch.arena, &params.entity_list, params.entity);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
      }
    }
  }
  
  //- rjf: selected entity num -> handle
  {
    fev->selected_entity_handle = (1 <= cursor.y && cursor.y <= ent_arr.count) ? df_handle_from_entity(ent_arr.v[cursor.y-1].entity) : df_handle_zero();
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: SymbolLister @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(SymbolLister)
{
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(SymbolLister)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(SymbolLister)
{
}

DF_VIEW_UI_FUNCTION_DEF(SymbolLister)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  U64 thread_unwind_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx.unwind_count);
  DF_Entity *module = df_module_from_process_vaddr(process, thread_unwind_rip_vaddr);
  DF_Entity *binary = df_binary_file_from_module(module);
  String8 exe_path = df_full_path_from_entity(scratch.arena, binary);
  String8 query = str8(view->query_buffer, view->query_string_size);
  TG_Graph *graph = tg_graph_begin(bit_size_from_arch(df_architecture_from_entity(thread))/8, 256);
  
  //- rjf: grab state
  typedef struct DF_SymbolListerViewState DF_SymbolListerViewState;
  struct DF_SymbolListerViewState
  {
    Vec2S64 cursor;
  };
  DF_SymbolListerViewState *slv = df_view_user_state(view, DF_SymbolListerViewState);
  
  //- rjf: query -> raddbg, filtered items
  U128 fuzzy_search_key = {(U64)view, df_hash_from_string(str8_struct(&view))};
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, os_now_microseconds()+100);
  RDI_Parsed *rdi = &dbgi->rdi;
  B32 items_stale = 0;
  DBGI_FuzzySearchItemArray items = dbgi_fuzzy_search_items_from_key_exe_query(scope, fuzzy_search_key, exe_path, query, DBGI_FuzzySearchTarget_Procedures, os_now_microseconds()+100, &items_stale);
  if(items_stale)
  {
    df_gfx_request_frame();
  }
  
  //- rjf: submit best match when hitting enter w/ no selection
  if(slv->cursor.y == 0 && items.count != 0 && ui_key_press(0, OS_Key_Return))
  {
    RDI_Procedure *procedure = rdi_element_from_idx(rdi, procedures, items.v[0].idx);
    U64 name_size = 0;
    U8 *name_base = rdi_string_from_idx(rdi, procedure->name_string_idx, &name_size);
    String8 name = str8(name_base, name_size);
    if(name.size != 0)
    {
      DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
      p.string = name;
      df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
    }
  }
  
  //- rjf: build contents
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    Vec2F32 content_dim = dim_2f32(rect);
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = v2f32(content_dim.x, content_dim.y);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(0, items.count));
    scroll_list_params.item_range    = r1s64(0, items.count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &slv->cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_Font(df_font_from_slot(DF_FontSlot_Code))
  {
    //- rjf: build rows
    for(U64 idx = visible_row_range.min;
        idx <= visible_row_range.max && idx < items.count;
        idx += 1)
      UI_Focus((slv->cursor.y == idx+1) ? UI_FocusKind_On : UI_FocusKind_Off)
    {
      DBGI_FuzzySearchItem *item = &items.v[idx];
      RDI_Procedure *procedure = rdi_element_from_idx(rdi, procedures, item->idx);
      U64 name_size = 0;
      U8 *name_base = rdi_string_from_idx(rdi, procedure->name_string_idx, &name_size);
      String8 name = str8(name_base, name_size);
      RDI_TypeNode *type_node = rdi_element_from_idx(rdi, type_nodes, procedure->type_idx);
      TG_Key type_key = tg_key_ext(tg_kind_from_rdi_type_kind(type_node->kind), procedure->type_idx);
      ui_set_next_hover_cursor(OS_Cursor_HandPoint);
      UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                              UI_BoxFlag_DrawBackground|
                                              UI_BoxFlag_DrawBorder|
                                              UI_BoxFlag_DrawText|
                                              UI_BoxFlag_DrawHotEffects|
                                              UI_BoxFlag_DrawActiveEffects,
                                              "###procedure_%I64x", item->idx);
      UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 1))
      {
        UI_Box *box = df_code_label(1.f, 0, df_rgba_from_theme_color(DF_ThemeColor_CodeFunction), name);
        ui_box_equip_fuzzy_match_ranges(box, &item->match_ranges);
        if(!tg_key_match(tg_key_zero(), type_key))
        {
          String8 type_string = tg_string_from_key(scratch.arena, graph, rdi, type_key);
          df_code_label(0.5f, 0, df_rgba_from_theme_color(DF_ThemeColor_WeakText), type_string);
        }
      }
      UI_Signal sig = ui_signal_from_box(box);
      if(ui_clicked(sig))
      {
        DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
        p.string = name;
        df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
        df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
      }
      if(ui_hovering(sig)) UI_Tooltip
      {
        U64 binary_voff = df_voff_from_binary_symbol_name(binary, name);
        DF_TextLineDasm2SrcInfo dasm2src_info = df_text_line_dasm2src_info_from_binary_voff(binary, binary_voff);
        String8 file_path = df_full_path_from_entity(scratch.arena, dasm2src_info.file);
        S64 line_num = dasm2src_info.pt.line;
        df_code_label(1.f, 0, df_rgba_from_theme_color(DF_ThemeColor_CodeFunction), name);
        UI_Font(df_font_from_slot(DF_FontSlot_Main)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
          ui_labelf("Procedure #%I64u", item->idx);
        if(!df_entity_is_nil(dasm2src_info.file))
        {
          UI_Font(df_font_from_slot(DF_FontSlot_Main)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            ui_labelf("%S:%I64d", file_path, line_num);
        }
        else
        {
          UI_Font(df_font_from_slot(DF_FontSlot_Main)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            ui_label(str8_lit("(No source code location found)"));
        }
      }
    }
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Target @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Target)
{
  DF_TargetViewState *tv = df_view_user_state(view, DF_TargetViewState);
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Target)
{
  DF_TargetViewState *tv = df_view_user_state(view, DF_TargetViewState);
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(Target)
{
  DF_TargetViewState *tv = df_view_user_state(view, DF_TargetViewState);
  DF_Entity *entity = df_entity_from_handle(view->entity);
  
  // rjf: process commands
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched view => skip
    if(df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    // rjf: process command
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default:break;
      case DF_CoreCmdKind_PickFile:
      case DF_CoreCmdKind_PickFolder:
      {
        String8 pick_string = cmd->params.file_path;
        DF_Entity *storage_entity = entity;
        if(tv->pick_dst_kind != DF_EntityKind_Nil)
        {
          DF_Entity *child = df_entity_child_from_kind(entity, tv->pick_dst_kind);
          if(df_entity_is_nil(child))
          {
            child = df_entity_alloc(0, entity, tv->pick_dst_kind);
          }
          storage_entity = child;
        }
        df_entity_equip_name(0, storage_entity, pick_string);
      }break;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(Target)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_Entity *entity = df_entity_from_handle(view->entity);
  DF_EntityList custom_entry_points = df_push_entity_child_list_with_kind(scratch.arena, entity, DF_EntityKind_EntryPointName);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  
  //- rjf: grab state
  DF_TargetViewState *tv = df_view_user_state(view, DF_TargetViewState);
  if(tv->initialized == 0)
  {
    tv->initialized = 1;
    tv->key_pct = 0.2f;
    tv->value_pct = 0.8f;
  }
  
  //- rjf: set up key-value-pair info
  struct
  {
    B32 fill_with_file;
    B32 fill_with_folder;
    B32 use_code_font;
    String8 key;
    DF_EntityKind storage_child_kind;
    String8 current_text;
  }
  kv_info[] =
  {
    { 0, 0, 0, str8_lit("Label"),                DF_EntityKind_Nil,            entity->name },
    { 1, 0, 0, str8_lit("Executable"),           DF_EntityKind_Executable,     df_entity_child_from_kind(entity, DF_EntityKind_Executable)->name },
    { 0, 0, 0, str8_lit("Arguments"),            DF_EntityKind_Arguments,      df_entity_child_from_kind(entity, DF_EntityKind_Arguments)->name },
    { 0, 1, 0, str8_lit("Working Directory"),    DF_EntityKind_ExecutionPath,  df_entity_child_from_kind(entity, DF_EntityKind_ExecutionPath)->name },
    { 0, 0, 1, str8_lit("Entry Point Override"), DF_EntityKind_EntryPointName, df_entity_child_from_kind(entity, DF_EntityKind_EntryPointName)->name },
  };
  
  //- rjf: take controls to start/end editing
  B32 edit_begin  = 0;
  B32 edit_end    = 0;
  B32 edit_commit = 0;
  B32 edit_submit = 0;
  UI_Focus(UI_FocusKind_On) if(ui_is_focus_active())
  {
    if(!tv->input_editing)
    {
      UI_EventList *events = ui_events();
      for(UI_EventNode *n = events->first; n != 0; n = n->next)
      {
        if(n->v.string.size != 0 || n->v.flags & UI_EventFlag_Paste)
        {
          edit_begin = 1;
          break;
        }
      }
      if(ui_key_press(0, OS_Key_F2))
      {
        edit_begin = 1;
      }
      if(ui_key_press(0, OS_Key_Return))
      {
        edit_begin = 1;
      }
    }
    if(tv->input_editing)
    {
      if(ui_key_press(0, OS_Key_Esc))
      {
        edit_end = 1;
        edit_commit = 0;
      }
      if(ui_key_press(0, OS_Key_Return))
      {
        edit_end = 1;
        edit_commit = 1;
        edit_submit = 1;
      }
    }
  }
  
  //- rjf: build
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(0, (S64)ArrayCount(kv_info)));
    scroll_list_params.item_range    = r1s64(0, (S64)ArrayCount(kv_info));
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  DF_EntityKind commit_storage_child_kind = DF_EntityKind_Nil;
  Vec2S64 next_cursor = tv->cursor;
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  tv->input_editing ? 0 : &tv->cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    next_cursor = tv->cursor;
    F32 *col_pcts[] = {&tv->key_pct, &tv->value_pct};
    UI_TableF(ArrayCount(col_pcts), col_pcts, "###target_%p", view)
    {
      //- rjf: build fixed rows
      S64 row_idx = 0;
      for(S64 idx = visible_row_range.min;
          idx <= visible_row_range.max && idx < ArrayCount(kv_info);
          idx += 1, row_idx += 1)
        UI_TableVector
      {
        B32 row_selected = (tv->cursor.y == idx+1);
        B32 has_browse = kv_info[idx].fill_with_file || kv_info[idx].fill_with_folder;
        
        //- rjf: key (label)
        UI_TableCell UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
        {
          if(kv_info[idx].storage_child_kind == DF_EntityKind_EntryPointName)
          {
            if(df_help_label(str8_lit("Custom Entry Point"))) UI_Tooltip
            {
              ui_label_multiline(ui_top_font_size()*30.f, str8_lit("By default, the debugger attempts to find a target's entry point with a set of default names, such as:"));
              ui_spacer(ui_em(1.5f, 1.f));
              UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_CodeFunction))
              {
                ui_label(str8_lit("WinMain"));
                ui_label(str8_lit("wWinMain"));
                ui_label(str8_lit("main"));
                ui_label(str8_lit("wmain"));
                ui_label(str8_lit("WinMainCRTStartup"));
                ui_label(str8_lit("wWinMainCRTStartup"));
              }
              ui_spacer(ui_em(1.5f, 1.f));
              ui_label_multiline(ui_top_font_size()*30.f, str8_lit("A Custom Entry Point can be used to override these default symbol names with a symbol name of your choosing. If a symbol matching the Custom Entry Point is not found, the debugger will fall back to its default rules."));
            }
          }
          else
          {
            ui_build_box_from_string(UI_BoxFlag_Clickable|UI_BoxFlag_DrawText, kv_info[idx].key);
          }
        }
        
        //- rjf: value
        UI_TableCell
        {
          // rjf: value editor
          UI_WidthFill UI_Font(kv_info[idx].use_code_font ? df_font_from_slot(DF_FontSlot_Code) : df_font_from_slot(DF_FontSlot_Main))
          {
            // rjf: * => focus
            B32 value_selected = row_selected && (next_cursor.x == 0 || !has_browse);
            
            // rjf: begin editing
            if(value_selected && edit_begin)
            {
              tv->input_editing = 1;
              tv->input_size = Min(sizeof(tv->input_buffer), kv_info[idx].current_text.size);
              MemoryCopy(tv->input_buffer, kv_info[idx].current_text.str, tv->input_size);
              tv->input_cursor = txt_pt(1, 1+tv->input_size);
              tv->input_mark = txt_pt(1, 1);
            }
            
            // rjf: build main editor ui
            UI_Signal sig = {0};
            UI_FocusHot(value_selected ? UI_FocusKind_On : UI_FocusKind_Off)
              UI_FocusActive((value_selected && tv->input_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
            {
              sig = df_line_editf(DF_LineEditFlag_NoBackground, 0, 0, &tv->input_cursor, &tv->input_mark, tv->input_buffer, sizeof(tv->input_buffer), &tv->input_size, 0, kv_info[idx].current_text, "###kv_editor_%i", (S32)idx);
              edit_commit = edit_commit || ui_committed(sig);
            }
            
            // rjf: focus panel on press
            if(ui_pressed(sig))
            {
              DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
            }
            
            // rjf: begin editing on double-click
            if(!tv->input_editing && ui_double_clicked(sig))
            {
              ui_kill_action();
              tv->input_editing = 1;
              tv->input_size = Min(sizeof(tv->input_buffer), kv_info[idx].current_text.size);
              MemoryCopy(tv->input_buffer, kv_info[idx].current_text.str, tv->input_size);
              tv->input_cursor = txt_pt(1, 1+tv->input_size);
              tv->input_mark = txt_pt(1, 1);
            }
            
            // rjf: press on non-selected => commit edit, change selected cell
            if(ui_pressed(sig) && !value_selected)
            {
              edit_end = 1;
              edit_commit = tv->input_editing;
              next_cursor = v2s64(0, idx+1);
            }
            
            // rjf: apply commit deltas
            if(ui_committed(sig))
            {
              next_cursor.y += 1;
            }
            
            // rjf: grab commit destination
            if(value_selected)
            {
              commit_storage_child_kind = kv_info[idx].storage_child_kind;
            }
          }
          
          // rjf: browse button to fill text field
          if(has_browse) UI_PrefWidth(ui_text_dim(10, 1))
          {
            UI_FocusHot((row_selected && next_cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
              UI_TextAlignment(UI_TextAlign_Center)
              if(ui_clicked(ui_buttonf("Browse...")))
            {
              DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
              params.cmd_spec = df_cmd_spec_from_core_cmd_kind(kv_info[idx].fill_with_file ? DF_CoreCmdKind_PickFile : DF_CoreCmdKind_PickFolder);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
              tv->pick_dst_kind = kv_info[idx].storage_child_kind;
            }
          }
        }
      }
    }
  }
  
  //- rjf: apply commit
  if(edit_commit)
  {
    String8 new_string = str8(tv->input_buffer, tv->input_size);
    df_state_delta_history_push_batch(df_state_delta_history(), 0);
    switch(commit_storage_child_kind)
    {
      default:
      {
        DF_Entity *child = df_entity_child_from_kind(entity, commit_storage_child_kind);
        if(df_entity_is_nil(child))
        {
          child = df_entity_alloc(df_state_delta_history(), entity, commit_storage_child_kind);
        }
        df_entity_equip_name(df_state_delta_history(), child, new_string);
      }break;
      case DF_EntityKind_Nil:
      {
        df_entity_equip_name(df_state_delta_history(), entity, new_string);
      }break;
    }
  }
  
  //- rjf: apply editing finish
  if(edit_end)
  {
    tv->input_editing = 0;
  }
  if(edit_submit)
  {
    next_cursor.y += 1;
  }
  
  //- rjf: apply moves to selection
  tv->cursor = next_cursor;
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Targets @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Targets)
{
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Targets)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(Targets)
{
}

DF_VIEW_UI_FUNCTION_DEF(Targets)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_EntityList targets_list = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
  String8 query = str8(view->query_buffer, view->query_string_size);
  DF_EntityFuzzyItemArray targets = df_entity_fuzzy_item_array_from_entity_list_needle(scratch.arena, &targets_list, query);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  
  //- rjf: grab state
  typedef struct DF_TargetsViewState DF_TargetsViewState;
  struct DF_TargetsViewState
  {
    B32 selected_add;
    DF_Handle selected_target_handle;
    S64 selected_column;
  };
  DF_TargetsViewState *tv = df_view_user_state(view, DF_TargetsViewState);
  
  //- rjf: determine table bounds
  Vec2S64 table_bounds = {5, (S64)targets.count+1};
  
  //- rjf: selection state => cursor
  // NOTE(rjf): 0 => nothing, 1 => add new, 2 => first target
  Vec2S64 cursor = {0};
  {
    DF_Entity *selected_target = df_entity_from_handle(tv->selected_target_handle);
    for(U64 idx = 0; idx < targets.count; idx += 1)
    {
      if(selected_target == targets.v[idx].entity)
      {
        cursor.y = (S64)idx+2;
        break;
      }
    }
    if(tv->selected_add)
    {
      cursor.y = 1;
    }
    cursor.x = tv->selected_column;
  }
  
  //- rjf: build
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(5, Max(0, (S64)targets.count+1)));
    scroll_list_params.item_range    = r1s64(0, Max(0, (S64)targets.count+1));
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    // rjf: add new ctrl
    if(visible_row_range.min == 0)
    {
      UI_Signal add_sig = {0};
      UI_FocusHot(cursor.y == 1 ? UI_FocusKind_On : UI_FocusKind_Off)
        add_sig = df_icon_buttonf(DF_IconKind_Add, 0, "Add New Target");
      if(ui_clicked(add_sig))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddTarget);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
      }
    }
    
    // rjf: target rows
    for(S64 row_idx = Max(1, visible_row_range.min);
        row_idx <= visible_row_range.max && row_idx <= targets.count;
        row_idx += 1)
      UI_Row
    {
      DF_Entity *target = targets.v[row_idx-1].entity;
      B32 row_selected = ((U64)cursor.y == row_idx+1);
      
      // rjf: enabled
      UI_PrefWidth(ui_em(2.25f, 1))
        UI_FocusHot((row_selected && cursor.x == 0) ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        UI_Signal sig = df_icon_buttonf(target->b32 ? DF_IconKind_CheckFilled : DF_IconKind_CheckHollow, 0, "###ebl_%p", target);
        if(ui_clicked(sig) && sig.event_flags == 0)
        {
          DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
          p.entity = df_handle_from_entity(target);
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectTarget));
        }
        else if(ui_clicked(sig) && sig.event_flags == OS_EventFlag_Ctrl)
        {
          DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
          p.entity = df_handle_from_entity(target);
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(target->b32 ? DF_CoreCmdKind_DisableTarget : DF_CoreCmdKind_EnableTarget));
        }
      }
      
      // rjf: target name
      UI_WidthFill UI_FocusHot((row_selected && cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        df_entity_desc_button(ws, target, &targets.v[row_idx-1].matches, query);
      }
      
      // rjf: controls
      UI_PrefWidth(ui_em(2.25f, 1.f))
      {
        struct
        {
          DF_IconKind icon;
          String8 text;
          DF_CoreCmdKind cmd;
        }
        ctrls[] =
        {
          { DF_IconKind_PlayStepForward,  str8_lit("Launch and Initialize"), DF_CoreCmdKind_LaunchAndInit },
          { DF_IconKind_Play,             str8_lit("Launch and Run"),        DF_CoreCmdKind_LaunchAndRun  },
          { DF_IconKind_Pencil,           str8_lit("Edit"),                  DF_CoreCmdKind_Target        },
          { DF_IconKind_Trash,            str8_lit("Delete"),                DF_CoreCmdKind_RemoveTarget  },
        };
        for(U64 ctrl_idx = 0; ctrl_idx < ArrayCount(ctrls); ctrl_idx += 1)
        {
          UI_Signal sig = {0};
          UI_FocusHot((row_selected && cursor.x == 2+ctrl_idx) ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            sig = df_icon_buttonf(ctrls[ctrl_idx].icon, 0, "###%p_ctrl_%i", target, (int)ctrl_idx);
          }
          if(ui_hovering(sig)) UI_Tooltip
          {
            ui_label(ctrls[ctrl_idx].text);
          }
          if(ui_clicked(sig))
          {
            DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
            params.entity = df_handle_from_entity(target);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_handle_list_push(scratch.arena, &params.entity_list, params.entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(ctrls[ctrl_idx].cmd));
          }
        }
      }
    }
  }
  
  //- rjf: commit cursor to selection state
  {
    tv->selected_column = cursor.x;
    tv->selected_target_handle = (1 < cursor.y && cursor.y < targets.count+2) ? df_handle_from_entity(targets.v[cursor.y-2].entity) : df_handle_zero();
    tv->selected_add = (cursor.y == 1);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: FilePathMap @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(FilePathMap)
{
  DF_FilePathMapViewState *fpms = df_view_user_state(view, DF_FilePathMapViewState);
  if(fpms->initialized == 0)
  {
    fpms->initialized = 1;
    fpms->src_column_pct = 0.5f;
    fpms->dst_column_pct = 0.5f;
  }
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(FilePathMap)
{
  DF_FilePathMapViewState *fpms = df_view_user_state(view, DF_FilePathMapViewState);
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(FilePathMap)
{
  DF_FilePathMapViewState *fpms = df_view_user_state(view, DF_FilePathMapViewState);
  
  // rjf: process commands
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    //rjf: process
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default:break;
      case DF_CoreCmdKind_PickFile:
      case DF_CoreCmdKind_PickFolder:
      case DF_CoreCmdKind_PickFileOrFolder:
      {
        String8 pick_string = cmd->params.file_path;
        Side pick_side = fpms->pick_file_dst_side;
        DF_Entity *storage_entity = df_entity_from_handle(fpms->pick_file_dst_map);
        DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
        p.entity = df_handle_from_entity(storage_entity);
        p.file_path = pick_string;
        df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
        df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
        df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(pick_side == Side_Min ?
                                                             DF_CoreCmdKind_SetFileOverrideLinkSrc :
                                                             DF_CoreCmdKind_SetFileOverrideLinkDst));
      }break;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(FilePathMap)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_EntityList maps_list = df_query_cached_entity_list_with_kind(DF_EntityKind_OverrideFileLink);
  DF_EntityArray maps = df_entity_array_from_list(scratch.arena, &maps_list);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  
  //- rjf: grab state
  DF_FilePathMapViewState *fpms = df_view_user_state(view, DF_FilePathMapViewState);
  
  //- rjf: take controls to start/end editing
  B32 edit_begin  = 0;
  B32 edit_end    = 0;
  B32 edit_commit = 0;
  B32 edit_submit = 0;
  UI_Focus(UI_FocusKind_On) if(ui_is_focus_active())
  {
    if(!fpms->input_editing)
    {
      UI_EventList *events = ui_events();
      for(UI_EventNode *n = events->first; n != 0; n = n->next)
      {
        if(n->v.string.size != 0 || n->v.flags & UI_EventFlag_Paste)
        {
          edit_begin = 1;
          break;
        }
      }
      if(ui_key_press(0, OS_Key_F2))
      {
        edit_begin = 1;
      }
    }
    if(fpms->input_editing)
    {
      if(ui_key_press(0, OS_Key_Esc))
      {
        edit_end = 1;
        edit_commit = 0;
      }
      if(ui_key_press(0, OS_Key_Return))
      {
        edit_end = 1;
        edit_commit = 1;
        edit_submit = 1;
      }
    }
  }
  
  //- rjf: build
  DF_Handle commit_map = df_handle_zero();
  Side commit_side = Side_Invalid;
  F32 *col_pcts[] = { &fpms->src_column_pct, &fpms->dst_column_pct };
  Vec2S64 next_cursor = fpms->cursor;
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(3, maps.count + 1));
    scroll_list_params.item_range    = r1s64(0, maps.count+2);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  fpms->input_editing ? 0 : &fpms->cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_TableF(ArrayCount(col_pcts), col_pcts, "###tbl")
  {
    next_cursor = fpms->cursor;
    
    //- rjf: header
    if(visible_row_range.min == 0) UI_TableVector UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      UI_TableCell if(df_help_label(str8_lit("Source Path"))) UI_Tooltip
      {
        ui_label_multiline(ui_top_font_size()*30, str8_lit("When the debugger attempts to open a file or folder at a Source Path specified in this table, it will redirect to the file or folder specified by the Destination Path."));
      }
      UI_TableCell ui_label(str8_lit("Destination Path"));
    }
    
    //- rjf: map rows
    for(S64 row_idx = Max(1, visible_row_range.min);
        row_idx <= visible_row_range.max && row_idx <= maps.count+1;
        row_idx += 1) UI_TableVector
    {
      U64 map_idx = row_idx-1;
      DF_Entity *map = (map_idx < maps.count ? maps.v[map_idx] : &df_g_nil_entity);
      DF_Entity *map_link = df_entity_from_handle(map->entity_handle);
      String8 map_src_path = df_full_path_from_entity(scratch.arena, map);
      String8 map_dst_path = df_full_path_from_entity(scratch.arena, map_link);
      B32 row_selected = (fpms->cursor.y == row_idx);
      
      //- rjf: src
      UI_TableCell UI_WidthFill
      {
        //- rjf: editor
        {
          B32 value_selected = (row_selected && fpms->cursor.x == 0);
          
          // rjf: begin editing
          if(value_selected && edit_begin)
          {
            fpms->input_editing = 1;
            fpms->input_size = Min(sizeof(fpms->input_buffer), map_src_path.size);
            MemoryCopy(fpms->input_buffer, map_src_path.str, fpms->input_size);
            fpms->input_cursor = txt_pt(1, 1+fpms->input_size);
            fpms->input_mark = txt_pt(1, 1);
          }
          
          // rjf: build
          UI_Signal sig = {0};
          UI_FocusHot(value_selected ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_FocusActive((value_selected && fpms->input_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            sig = df_line_editf(DF_LineEditFlag_NoBackground, 0, 0, &fpms->input_cursor, &fpms->input_mark, fpms->input_buffer, sizeof(fpms->input_buffer), &fpms->input_size, 0, map_src_path, "###src_editor_%p", map);
            edit_commit = edit_commit || ui_committed(sig);
          }
          
          // rjf: focus panel on press
          if(ui_pressed(sig))
          {
            DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
          
          // rjf: begin editing on double-click
          if(!fpms->input_editing && ui_double_clicked(sig))
          {
            fpms->input_editing = 1;
            fpms->input_size = Min(sizeof(fpms->input_buffer), map_src_path.size);
            MemoryCopy(fpms->input_buffer, map_src_path.str, fpms->input_size);
            fpms->input_cursor = txt_pt(1, 1+fpms->input_size);
            fpms->input_mark = txt_pt(1, 1);
          }
          
          // rjf: press on non-selected => commit edit, change selected cell
          if(ui_pressed(sig) && !value_selected)
          {
            edit_end = 1;
            edit_commit = fpms->input_editing;
            next_cursor.x = 0;
            next_cursor.y = map_idx+1;
          }
          
          // rjf: store commit information
          if(value_selected)
          {
            commit_side = Side_Min;
            commit_map = df_handle_from_entity(map);
          }
        }
        
        //- rjf: browse button
        UI_FocusHot((row_selected && fpms->cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
          UI_PrefWidth(ui_text_dim(10, 1))
          if(ui_clicked(ui_buttonf("Browse...")))
        {
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
          params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PickFileOrFolder);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
          fpms->pick_file_dst_map = df_handle_from_entity(map);
          fpms->pick_file_dst_side = Side_Min;
        }
      }
      
      //- rjf: dst
      UI_TableCell UI_WidthFill
      {
        //- rjf: editor
        {
          B32 value_selected = (row_selected && fpms->cursor.x == 2);
          
          // rjf: begin editing
          if(value_selected && edit_begin)
          {
            fpms->input_editing = 1;
            fpms->input_size = Min(sizeof(fpms->input_buffer), map_dst_path.size);
            MemoryCopy(fpms->input_buffer, map_dst_path.str, fpms->input_size);
            fpms->input_cursor = txt_pt(1, 1+fpms->input_size);
            fpms->input_mark = txt_pt(1, 1);
          }
          
          // rjf: build
          UI_Signal sig = {0};
          UI_FocusHot(value_selected ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_FocusActive((value_selected && fpms->input_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            sig = df_line_editf(DF_LineEditFlag_NoBackground, 0, 0, &fpms->input_cursor, &fpms->input_mark, fpms->input_buffer, sizeof(fpms->input_buffer), &fpms->input_size, 0, map_dst_path, "###dst_editor_%p", map);
            edit_commit = edit_commit || ui_committed(sig);
          }
          
          // rjf: focus panel on press
          if(ui_pressed(sig))
          {
            DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
          
          // rjf: begin editing on double-click
          if(!fpms->input_editing && ui_double_clicked(sig))
          {
            fpms->input_editing = 1;
            fpms->input_size = Min(sizeof(fpms->input_buffer), map_dst_path.size);
            MemoryCopy(fpms->input_buffer, map_dst_path.str, fpms->input_size);
            fpms->input_cursor = txt_pt(1, 1+fpms->input_size);
            fpms->input_mark = txt_pt(1, 1);
          }
          
          // rjf: press on non-selected => commit edit, change selected cell
          if(ui_pressed(sig) && !value_selected)
          {
            edit_end = 1;
            edit_commit = fpms->input_editing;
            next_cursor.x = 2;
            next_cursor.y = map_idx+1;
          }
          
          // rjf: store commit information
          if(value_selected)
          {
            commit_side = Side_Max;
            commit_map = df_handle_from_entity(map);
          }
        }
        
        //- rjf: browse button
        {
          UI_FocusHot((row_selected && fpms->cursor.x == 3) ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_PrefWidth(ui_text_dim(10, 1))
            if(ui_clicked(ui_buttonf("Browse...")))
          {
            DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
            params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PickFileOrFolder);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
            fpms->pick_file_dst_map = df_handle_from_entity(map);
            fpms->pick_file_dst_side = Side_Max;
          }
        }
      }
    }
  }
  
  //- rjf: apply commit
  if(edit_commit && commit_side != Side_Invalid)
  {
    String8 new_string = str8(fpms->input_buffer, fpms->input_size);
    DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
    p.entity = commit_map;
    p.file_path = new_string;
    df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
    df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
    df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(commit_side == Side_Min ?
                                                         DF_CoreCmdKind_SetFileOverrideLinkSrc :
                                                         DF_CoreCmdKind_SetFileOverrideLinkDst));
  }
  
  //- rjf: apply editing finish
  if(edit_end)
  {
    fpms->input_editing = 0;
  }
  
  //- rjf: move down one row if submitted
  if(edit_submit)
  {
    next_cursor.y += 1;
  }
  
  //- rjf: apply moves to selection
  fpms->cursor = next_cursor;
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: AutoViewRules @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(AutoViewRules)
{
  DF_AutoViewRulesViewState *avrs = df_view_user_state(view, DF_AutoViewRulesViewState);
  if(avrs->initialized == 0)
  {
    avrs->initialized = 1;
    avrs->src_column_pct = 0.5f;
    avrs->dst_column_pct = 0.5f;
  }
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(AutoViewRules)
{
  DF_AutoViewRulesViewState *avrs = df_view_user_state(view, DF_AutoViewRulesViewState);
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(AutoViewRules)
{
  DF_AutoViewRulesViewState *avrs = df_view_user_state(view, DF_AutoViewRulesViewState);
  
  // rjf: process commands
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(AutoViewRules)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_EntityList maps_list = df_query_cached_entity_list_with_kind(DF_EntityKind_AutoViewRule);
  DF_EntityArray maps = df_entity_array_from_list(scratch.arena, &maps_list);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  
  //- rjf: grab state
  DF_AutoViewRulesViewState *avrs = df_view_user_state(view, DF_AutoViewRulesViewState);
  
  //- rjf: take controls to start/end editing
  B32 edit_begin  = 0;
  B32 edit_end    = 0;
  B32 edit_commit = 0;
  B32 edit_submit = 0;
  UI_Focus(UI_FocusKind_On) if(ui_is_focus_active())
  {
    if(!avrs->input_editing)
    {
      UI_EventList *events = ui_events();
      for(UI_EventNode *n = events->first; n != 0; n = n->next)
      {
        if(n->v.string.size != 0 || n->v.flags & UI_EventFlag_Paste)
        {
          edit_begin = 1;
          break;
        }
      }
      if(ui_key_press(0, OS_Key_F2))
      {
        edit_begin = 1;
      }
    }
    if(avrs->input_editing)
    {
      if(ui_key_press(0, OS_Key_Esc))
      {
        edit_end = 1;
        edit_commit = 0;
      }
      if(ui_key_press(0, OS_Key_Return))
      {
        edit_end = 1;
        edit_commit = 1;
        edit_submit = 1;
      }
    }
  }
  
  //- rjf: build
  DF_Handle commit_map = df_handle_zero();
  Side commit_side = Side_Invalid;
  F32 *col_pcts[] = { &avrs->src_column_pct, &avrs->dst_column_pct };
  Vec2S64 next_cursor = avrs->cursor;
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(1, maps.count + 1));
    scroll_list_params.item_range    = r1s64(0, maps.count+2);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  avrs->input_editing ? 0 : &avrs->cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_TableF(ArrayCount(col_pcts), col_pcts, "###tbl")
  {
    next_cursor = avrs->cursor;
    
    //- rjf: header
    if(visible_row_range.min == 0) UI_TableVector UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      UI_TableCell ui_label(str8_lit("Type"));
      UI_TableCell ui_label(str8_lit("View Rule"));
    }
    
    //- rjf: map rows
    for(S64 row_idx = Max(1, visible_row_range.min);
        row_idx <= visible_row_range.max && row_idx <= maps.count+1;
        row_idx += 1) UI_TableVector
    {
      U64 map_idx = row_idx-1;
      DF_Entity *map = (map_idx < maps.count ? maps.v[map_idx] : &df_g_nil_entity);
      DF_Entity *source = df_entity_child_from_kind(map, DF_EntityKind_Source);
      DF_Entity *dest = df_entity_child_from_kind(map, DF_EntityKind_Dest);
      String8 type = source->name;
      String8 view_rule = dest->name;
      B32 row_selected = (avrs->cursor.y == row_idx);
      
      //- rjf: type
      UI_TableCell UI_WidthFill
      {
        //- rjf: editor
        {
          B32 value_selected = (row_selected && avrs->cursor.x == 0);
          
          // rjf: begin editing
          if(value_selected && edit_begin)
          {
            avrs->input_editing = 1;
            avrs->input_size = Min(sizeof(avrs->input_buffer), type.size);
            MemoryCopy(avrs->input_buffer, type.str, avrs->input_size);
            avrs->input_cursor = txt_pt(1, 1+avrs->input_size);
            avrs->input_mark = txt_pt(1, 1);
          }
          
          // rjf: build
          UI_Signal sig = {0};
          UI_FocusHot(value_selected ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_FocusActive((value_selected && avrs->input_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_Font(df_font_from_slot(DF_FontSlot_Code))
          {
            sig = df_line_editf(DF_LineEditFlag_CodeContents|DF_LineEditFlag_NoBackground|DF_LineEditFlag_DisplayStringIsCode, 0, 0, &avrs->input_cursor, &avrs->input_mark, avrs->input_buffer, sizeof(avrs->input_buffer), &avrs->input_size, 0, type, "###src_editor_%p", map);
            edit_commit = edit_commit || ui_committed(sig);
          }
          
          // rjf: focus panel on press
          if(ui_pressed(sig))
          {
            DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
          
          // rjf: begin editing on double-click
          if(!avrs->input_editing && ui_double_clicked(sig))
          {
            avrs->input_editing = 1;
            avrs->input_size = Min(sizeof(avrs->input_buffer), type.size);
            MemoryCopy(avrs->input_buffer, type.str, avrs->input_size);
            avrs->input_cursor = txt_pt(1, 1+avrs->input_size);
            avrs->input_mark = txt_pt(1, 1);
          }
          
          // rjf: press on non-selected => commit edit, change selected cell
          if(ui_pressed(sig) && !value_selected)
          {
            edit_end = 1;
            edit_commit = avrs->input_editing;
            next_cursor.x = 0;
            next_cursor.y = map_idx+1;
          }
          
          // rjf: store commit information
          if(value_selected)
          {
            commit_side = Side_Min;
            commit_map = df_handle_from_entity(map);
          }
        }
      }
      
      //- rjf: dst
      UI_TableCell UI_WidthFill
      {
        //- rjf: editor
        {
          B32 value_selected = (row_selected && avrs->cursor.x == 1);
          
          // rjf: begin editing
          if(value_selected && edit_begin)
          {
            avrs->input_editing = 1;
            avrs->input_size = Min(sizeof(avrs->input_buffer), view_rule.size);
            MemoryCopy(avrs->input_buffer, view_rule.str, avrs->input_size);
            avrs->input_cursor = txt_pt(1, 1+avrs->input_size);
            avrs->input_mark = txt_pt(1, 1);
          }
          
          // rjf: build
          UI_Signal sig = {0};
          UI_FocusHot(value_selected ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_FocusActive((value_selected && avrs->input_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_Font(df_font_from_slot(DF_FontSlot_Code))
          {
            sig = df_line_editf(DF_LineEditFlag_CodeContents|DF_LineEditFlag_NoBackground|DF_LineEditFlag_DisplayStringIsCode, 0, 0, &avrs->input_cursor, &avrs->input_mark, avrs->input_buffer, sizeof(avrs->input_buffer), &avrs->input_size, 0, view_rule, "###dst_editor_%p", map);
            edit_commit = edit_commit || ui_committed(sig);
          }
          
          // rjf: focus panel on press
          if(ui_pressed(sig))
          {
            DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
          
          // rjf: begin editing on double-click
          if(!avrs->input_editing && ui_double_clicked(sig))
          {
            avrs->input_editing = 1;
            avrs->input_size = Min(sizeof(avrs->input_buffer), view_rule.size);
            MemoryCopy(avrs->input_buffer, view_rule.str, avrs->input_size);
            avrs->input_cursor = txt_pt(1, 1+avrs->input_size);
            avrs->input_mark = txt_pt(1, 1);
          }
          
          // rjf: press on non-selected => commit edit, change selected cell
          if(ui_pressed(sig) && !value_selected)
          {
            edit_end = 1;
            edit_commit = avrs->input_editing;
            next_cursor.x = 1;
            next_cursor.y = map_idx+1;
          }
          
          // rjf: store commit information
          if(value_selected)
          {
            commit_side = Side_Max;
            commit_map = df_handle_from_entity(map);
          }
        }
      }
    }
  }
  
  //- rjf: apply commit
  if(edit_commit && commit_side != Side_Invalid)
  {
    String8 new_string = str8(avrs->input_buffer, avrs->input_size);
    DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
    p.entity = commit_map;
    p.string = new_string;
    df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
    df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
    df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(commit_side == Side_Min ?
                                                         DF_CoreCmdKind_SetAutoViewRuleType :
                                                         DF_CoreCmdKind_SetAutoViewRuleViewRule));
  }
  
  //- rjf: apply editing finish
  if(edit_end)
  {
    avrs->input_editing = 0;
  }
  
  //- rjf: move down one row if submitted
  if(edit_submit)
  {
    next_cursor.y += 1;
  }
  
  //- rjf: apply moves to selection
  avrs->cursor = next_cursor;
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Scheduler @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Scheduler) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Scheduler) {return str8_lit("");}
DF_VIEW_CMD_FUNCTION_DEF(Scheduler) {}
DF_VIEW_UI_FUNCTION_DEF(Scheduler)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  String8 query = str8(view->query_buffer, view->query_string_size);
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  
  //- rjf: get state
  typedef struct DF_SchedulerViewState DF_SchedulerViewState;
  struct DF_SchedulerViewState
  {
    DF_Handle selected_entity;
    S64 selected_column;
  };
  DF_SchedulerViewState *sv = df_view_user_state(view, DF_SchedulerViewState);
  
  //- rjf: get entities
  DF_EntityList machines  = df_query_cached_entity_list_with_kind(DF_EntityKind_Machine);
  DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
  DF_EntityList threads   = df_query_cached_entity_list_with_kind(DF_EntityKind_Thread);
  
  //- rjf: produce list of items; no query -> all entities, in tree; query -> only show threads
  DF_EntityFuzzyItemArray items = {0};
  ProfScope("query -> entities")
  {
    if(query.size == 0)
    {
      //- rjf: build flat array of entities, arranged into row order
      DF_EntityArray entities = {0};
      {
        entities.count = machines.count+processes.count+threads.count;
        entities.v = push_array_no_zero(scratch.arena, DF_Entity *, entities.count);
        U64 idx = 0;
        for(DF_EntityNode *machine_n = machines.first; machine_n != 0; machine_n = machine_n->next)
        {
          DF_Entity *machine = machine_n->entity;
          entities.v[idx] = machine;
          idx += 1;
          for(DF_EntityNode *process_n = processes.first; process_n != 0; process_n = process_n->next)
          {
            DF_Entity *process = process_n->entity;
            if(df_entity_ancestor_from_kind(process, DF_EntityKind_Machine) != machine)
            {
              continue;
            }
            entities.v[idx] = process;
            idx += 1;
            for(DF_EntityNode *thread_n = threads.first; thread_n != 0; thread_n = thread_n->next)
            {
              DF_Entity *thread = thread_n->entity;
              if(df_entity_ancestor_from_kind(thread, DF_EntityKind_Process) != process)
              {
                continue;
              }
              entities.v[idx] = thread;
              idx += 1;
            }
          }
        }
      }
      
      //- rjf: entities -> fuzzy-filtered entities
      items = df_entity_fuzzy_item_array_from_entity_array_needle(scratch.arena, &entities, query);
    }
    else
    {
      items = df_entity_fuzzy_item_array_from_entity_list_needle(scratch.arena, &threads, query);
    }
  }
  
  //- rjf: selected column/entity -> selected cursor
  Vec2S64 cursor = {sv->selected_column};
  for(U64 idx = 0; idx < items.count; idx += 1)
  {
    if(items.v[idx].entity == df_entity_from_handle(sv->selected_entity))
    {
      cursor.y = (S64)(idx+1);
      break;
    }
  }
  
  //- rjf: build table
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = floor_f32(ui_top_font_size()*2.5f);
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(4, items.count));
    scroll_list_params.item_range    = r1s64(0, items.count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_TableF(0, 0, "scheduler_table")
  {
    Vec2S64 next_cursor = cursor;
    for(U64 idx = visible_row_range.min;
        idx <= visible_row_range.max && idx < items.count;
        idx += 1)
    {
      DF_Entity *entity = items.v[idx].entity;
      B32 row_is_selected = (cursor.y == (S64)(idx+1));
      F32 depth = 0.f;
      if(query.size == 0) switch(entity->kind)
      {
        default:{}break;
        case DF_EntityKind_Machine:{depth = 0.f;}break;
        case DF_EntityKind_Process:{depth = 1.f;}break;
        case DF_EntityKind_Thread: {depth = 2.f;}break;
      }
      Rng1S64 desc_col_rng = r1s64(1, 1);
      switch(entity->kind)
      {
        default:{}break;
        case DF_EntityKind_Machine:{desc_col_rng = r1s64(1, 4);}break;
        case DF_EntityKind_Process:{desc_col_rng = r1s64(1, 1);}break;
        case DF_EntityKind_Thread: {desc_col_rng = r1s64(1, 1);}break;
      }
      UI_NamedTableVectorF("entity_row_%p", entity)
      {
        UI_TableCellSized(ui_em(1.5f*depth, 1.f)) {}
        UI_TableCellSized(ui_em(2.25f, 1.f)) UI_FocusHot((row_is_selected && cursor.x == 0) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          B32 frozen_by_solo_mode = (entity->kind == DF_EntityKind_Thread && entity != df_entity_from_handle(ctrl_ctx.thread) && df_state->ctrl_solo_stepping_mode);
          B32 frozen = df_entity_is_frozen(entity);
          Vec4F32 frozen_color  = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
          Vec4F32 frozen_in_solo_mode_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
          Vec4F32 thawed_color = df_rgba_from_theme_color(DF_ThemeColor_SuccessBackground);
          UI_Signal sig = {0};
          UI_BackgroundColor(frozen ? frozen_color : thawed_color)
          {
            if(frozen_by_solo_mode)
            {
              ui_set_next_background_color(frozen_in_solo_mode_color);
            }
            sig = df_icon_buttonf(frozen ? DF_IconKind_Locked : DF_IconKind_Unlocked, 0, "###lock_%p", entity);
          }
          if(frozen_by_solo_mode && ui_hovering(sig)) UI_Tooltip
          {
            ui_label(str8_lit("This thread is frozen during stepping operations because it isn't selected, and Solo Stepping Mode is enabled."));
          }
          if(ui_clicked(sig))
          {
            DF_CoreCmdKind cmd_kind = frozen ? DF_CoreCmdKind_ThawEntity : DF_CoreCmdKind_FreezeEntity;
            DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(cmd_kind));
          }
        }
        UI_TableCellSized(ui_pct(1, 0))
          UI_FocusHot((row_is_selected && desc_col_rng.min <= cursor.x && cursor.x <= desc_col_rng.max) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          df_entity_desc_button(ws, entity, &items.v[idx].matches, query);
        }
        switch(entity->kind)
        {
          default:{}break;
          case DF_EntityKind_Machine:
          {
            
          }break;
          case DF_EntityKind_Process:
          {
            UI_TableCellSized(ui_children_sum(1.f)) UI_FocusHot((row_is_selected && cursor.x == 2) ? UI_FocusKind_On : UI_FocusKind_Off)
            {
              UI_PrefWidth(ui_text_dim(10, 1))
                UI_TextAlignment(UI_TextAlign_Center)
                if(ui_clicked(ui_buttonf("Detach")))
              {
                DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
                params.entity = df_handle_from_entity(entity);
                df_handle_list_push(scratch.arena, &params.entity_list, df_handle_from_entity(entity));
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_EntityList);
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Detach));
              }
            }
            UI_TableCellSized(ui_em(2.25f, 1.f)) UI_FocusHot((row_is_selected && cursor.x == 3) ? UI_FocusKind_On : UI_FocusKind_Off)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_Redo, 0, "###retry")))
              {
                DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
                df_handle_list_push(scratch.arena, &params.entity_list, df_handle_from_entity(entity));
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_EntityList);
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Restart));
              }
            }
            UI_TableCellSized(ui_em(2.25f, 1.f)) UI_FocusHot((row_is_selected && cursor.x == 4) ? UI_FocusKind_On : UI_FocusKind_Off)
            {
              UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
                UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureText))
                UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBorder))
                if(ui_clicked(df_icon_buttonf(DF_IconKind_X, 0, "###kill")))
              {
                DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
                df_handle_list_push(scratch.arena, &params.entity_list, df_handle_from_entity(entity));
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_EntityList);
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Kill));
              }
            }
          }break;
          case DF_EntityKind_Thread:
          {
            UI_TableCellSized(ui_children_sum(1.f)) UI_FocusHot((row_is_selected && cursor.x >= 2) ? UI_FocusKind_On : UI_FocusKind_Off)
            {
              DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
              U64 rip_vaddr = df_query_cached_rip_from_thread(entity);
              DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
              U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
              DF_Entity *binary = df_binary_file_from_module(module);
              DF_TextLineDasm2SrcInfo line_info = df_text_line_dasm2src_info_from_binary_voff(binary, rip_voff);
              if(!df_entity_is_nil(line_info.file))
              {
                UI_PrefWidth(ui_children_sum(0)) df_entity_src_loc_button(ws, line_info.file, line_info.pt);
              }
            }
          }break;
        }
      }
    }
    cursor = next_cursor;
  }
  
  //- rjf: selected num -> selected entity
  sv->selected_column = cursor.x;
  sv->selected_entity = (1 <= cursor.y && cursor.y <= items.count) ? df_handle_from_entity(items.v[cursor.y-1].entity) : df_handle_zero();
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: CallStack @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(CallStack) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(CallStack) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(CallStack) {}
DF_VIEW_UI_FUNCTION_DEF(CallStack)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  U64 selected_unwind_count = ctrl_ctx.unwind_count;
  DF_Entity *process = thread->parent;
  Vec4F32 thread_color = df_rgba_from_theme_color(DF_ThemeColor_PlainText);
  if(thread->flags & DF_EntityFlag_HasColor)
  {
    thread_color = df_rgba_from_entity(thread);
  }
  CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
  
  //- rjf: grab state
  typedef struct DF_CallStackViewState DF_CallStackViewState;
  struct DF_CallStackViewState
  {
    B32 initialized;
    Vec2S64 cursor;
    Vec2S64 mark;
    F32 selection_col_pct;
    F32 module_col_pct;
    F32 function_name_col_pct;
    F32 addr_col_pct;
  };
  DF_CallStackViewState *cs = df_view_user_state(view, DF_CallStackViewState);
  if(cs->initialized == 0)
  {
    cs->initialized = 1;
    cs->selection_col_pct = 0.05f;
    cs->module_col_pct = 0.35f;
    cs->function_name_col_pct = 0.4f;
    cs->addr_col_pct = 0.2f;
  }
  
  //- rjf: build ui
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = floor_f32(ui_top_font_size()*2.5f);
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(3, unwind.count));
    scroll_list_params.item_range    = r1s64(0, unwind.count+1);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cs->cursor,
                  &cs->mark,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    Vec2S64 next_cursor = cs->cursor;
    
    //- rjf: build table
    if(df_ctrl_targets_running())
    {
      ui_set_next_flags(UI_BoxFlag_Disabled);
    }
    F32 *col_pcts[] = { &cs->selection_col_pct, &cs->module_col_pct, &cs->function_name_col_pct, &cs->addr_col_pct };
    UI_TableF(ArrayCount(col_pcts), col_pcts, "###tbl")
    {
      //- rjf: header
      if(visible_row_range.min == 0) UI_TableVector UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      {
        UI_TableCell {}
        UI_TableCell ui_label(str8_lit("Module"));
        UI_TableCell ui_label(str8_lit("Function Name"));
        UI_TableCell ui_label(str8_lit("Address"));
      }
      
      //- rjf: frame rows
      U64 frame_idx = 0;
      for(CTRL_UnwindFrame *frame = unwind.first; frame != 0; frame = frame->next, frame_idx += 1)
      {
        // rjf: out of range -> skip (TODO(rjf): this should be an array...)
        if(frame_idx+1 < visible_row_range.min || visible_row_range.max < frame_idx+1)
        {
          continue;
        }
        
        // rjf: determine selection
        B32 row_selected = cs->cursor.y == ((S64)frame_idx+1);
        
        // rjf: regs => rip
        U64 rip_vaddr = frame->rip;
        
        // rjf: rip_vaddr => module
        DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
        
        // rjf: rip => validity?
        B32 frame_valid = (rip_vaddr != 0);
        
        // rjf: build row
        if(frame_valid) UI_NamedTableVectorF("###callstack_%p_%I64x", view, frame_idx)
        {
          // rjf: build cell for selection
          UI_TableCell
            UI_Font(df_font_from_slot(DF_FontSlot_Icons))
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_TextColor(thread_color)
            UI_WidthFill
            UI_TextAlignment(UI_TextAlign_Center)
            UI_FocusHot((row_selected && cs->cursor.x == 0) ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            String8 selected_string = selected_unwind_count == frame_idx ? df_g_icon_kind_text_table[DF_IconKind_RightArrow] : str8_lit("");
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|UI_BoxFlag_DrawText, "%S###selection_%i", selected_string,
                                                    (int)frame_idx);
            UI_Signal sig = ui_signal_from_box(box);
            if(ui_pressed(sig))
            {
              next_cursor = v2s64(0, (S64)frame_idx+1);
              DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
            }
            if(ui_double_clicked(sig) || sig.f&UI_SignalFlag_KeyboardPressed)
            {
              DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
              params.index = frame_idx;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Index);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectUnwind));
            }
          }
          
          // rjf: build cell for module
          UI_TableCell UI_FocusHot((row_selected && cs->cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            if(df_entity_is_nil(module)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            {
              UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_Clickable, "(No Module)###moduleless_frame_%I64x", frame_idx);
              UI_Signal sig = ui_signal_from_box(box);
              if(ui_pressed(sig))
              {
                next_cursor = v2s64(1, (S64)frame_idx+1);
                DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
              }
            }
            else
            {
              df_entity_desc_button(ws, module, 0, str8_zero());
            }
          }
          
          // rjf: build cell for function name
          UI_TableCell UI_Font(df_font_from_slot(DF_FontSlot_Code))
            UI_FocusHot((row_selected && cs->cursor.x == 2) ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            String8 symbol = df_symbol_name_from_process_vaddr(scratch.arena, process, rip_vaddr);
            if(symbol.size == 0)
            {
              symbol = str8_lit("[external code]");
              ui_set_next_text_color(df_rgba_from_theme_color(DF_ThemeColor_WeakText));
            }
            else
            {
              ui_set_next_text_color(df_rgba_from_theme_color(DF_ThemeColor_CodeFunction));
            }
            UI_Box *box = ui_build_box_from_string(UI_BoxFlag_DrawText|UI_BoxFlag_Clickable, symbol);
            UI_Signal sig = ui_signal_from_box(box);
            if(ui_pressed(sig))
            {
              next_cursor = v2s64(2, (S64)frame_idx+1);
              DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
            }
            if(ui_double_clicked(sig) || sig.f&UI_SignalFlag_KeyboardPressed)
            {
              DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
              params.index = frame_idx;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Index);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectUnwind));
            }
          }
          
          // rjf: build cell for rip
          UI_TableCell
            UI_FocusHot((row_selected && cs->cursor.x == 3) ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_Clickable, "0x%I64x", rip_vaddr);
            UI_Signal sig = ui_signal_from_box(box);
            if(ui_pressed(sig))
            {
              next_cursor = v2s64(3, (S64)frame_idx+1);
              DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
            }
            if(ui_double_clicked(sig) || sig.f&UI_SignalFlag_KeyboardPressed)
            {
              DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
              params.index = frame_idx;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Index);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectUnwind));
            }
          }
        }
        
        // rjf: end if hit invalid frame
        if(frame_valid == 0)
        {
          break;
        }
      }
      
      // rjf: apply moves to selection
      cs->cursor = next_cursor;
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Modules @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Modules)
{
  DF_ModulesViewState *mv = df_view_user_state(view, DF_ModulesViewState);
  if(mv->initialized == 0)
  {
    mv->initialized = 1;
    mv->idx_col_pct   = 0.05f;
    mv->desc_col_pct  = 0.15f;
    mv->range_col_pct = 0.30f;
    mv->dbg_col_pct   = 0.50f;
  }
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Modules) {return str8_lit("");}

DF_VIEW_CMD_FUNCTION_DEF(Modules)
{
  DF_ModulesViewState *mv = df_view_user_state(view, DF_ModulesViewState);
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    //rjf: process
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default:break;
      case DF_CoreCmdKind_PickFile:
      {
        Temp scratch = scratch_begin(0, 0);
        String8 pick_string = cmd->params.file_path;
        DF_Entity *module = df_entity_from_handle(mv->pick_file_dst_entity);
        if(module->kind == DF_EntityKind_Module)
        {
          String8 exe_path = module->name;
          String8 dbg_path = pick_string;
          dbgi_force_exe_path_dbg_path(exe_path, dbg_path);
        }
        scratch_end(scratch);
      }break;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(Modules)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  String8 query = str8(view->query_buffer, view->query_string_size);
  
  //- rjf: get state
  DF_ModulesViewState *mv = df_view_user_state(view, DF_ModulesViewState);
  F32 *col_pcts[] = {&mv->idx_col_pct, &mv->desc_col_pct, &mv->range_col_pct, &mv->dbg_col_pct};
  
  //- rjf: get entities
  DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
  DF_EntityList modules = df_query_cached_entity_list_with_kind(DF_EntityKind_Module);
  
  //- rjf: make filtered item array
  DF_EntityFuzzyItemArray items = {0};
  if(query.size == 0)
  {
    DF_EntityArray entities = {0};
    {
      entities.count = processes.count+modules.count;
      entities.v = push_array_no_zero(scratch.arena, DF_Entity *, entities.count);
      U64 idx = 0;
      for(DF_EntityNode *process_n = processes.first; process_n != 0; process_n = process_n->next)
      {
        DF_Entity *process = process_n->entity;
        entities.v[idx] = process;
        idx += 1;
        for(DF_EntityNode *module_n = modules.first; module_n != 0; module_n = module_n->next)
        {
          DF_Entity *module = module_n->entity;
          if(df_entity_ancestor_from_kind(module, DF_EntityKind_Process) != process)
          {
            continue;
          }
          entities.v[idx] = module;
          idx += 1;
        }
      }
    }
    items = df_entity_fuzzy_item_array_from_entity_array_needle(scratch.arena, &entities, query);
  }
  else
  {
    items = df_entity_fuzzy_item_array_from_entity_list_needle(scratch.arena, &modules, query);
  }
  
  //- rjf: selected column/entity -> selected cursor
  Vec2S64 cursor = {mv->selected_column};
  for(U64 idx = 0; idx < items.count; idx += 1)
  {
    if(items.v[idx].entity == df_entity_from_handle(mv->selected_entity))
    {
      cursor.y = (S64)(idx+1);
      break;
    }
  }
  
  //////////////////////////////
  //- rjf: do start/end editing interaction
  //
  B32 edit_begin           = 0;
  B32 edit_commit          = 0;
  B32 edit_end             = 0;
  B32 edit_submit          = 0;
  if(!mv->txt_editing && ui_is_focus_active())
  {
    UI_EventList *events = ui_events();
    for(UI_EventNode *n = events->first; n != 0; n = n->next)
    {
      if(n->v.string.size != 0 || n->v.flags & UI_EventFlag_Paste)
      {
        edit_begin = 1;
        break;
      }
    }
    if(ui_key_press(0, OS_Key_F2) ||
       ui_key_press(0, OS_Key_Return))
    {
      edit_begin = 1;
    }
  }
  if(mv->txt_editing && ui_is_focus_active())
  {
    if(ui_key_press(0, OS_Key_Esc))
    {
      edit_end = 1;
      edit_commit = 0;
    }
    if(ui_key_press(0, OS_Key_Return))
    {
      edit_end = 1;
      edit_commit = 1;
      edit_submit = 1;
    }
  }
  
  //- rjf: build table
  DF_Entity *commit_module = &df_g_nil_entity;
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = floor_f32(ui_top_font_size()*2.5f);
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(3, items.count));
    scroll_list_params.item_range    = r1s64(0, items.count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  mv->txt_editing ? 0 : &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_TableF(ArrayCount(col_pcts), col_pcts, "modules_table")
  {
    Vec2S64 next_cursor = cursor;
    U64 idx_in_process = 0;
    for(U64 idx = 0; idx < items.count; idx += 1)
    {
      DF_Entity *entity = items.v[idx].entity;
      B32 row_is_selected = (cursor.y == (S64)(idx+1));
      idx_in_process += (entity->kind == DF_EntityKind_Module);
      if(visible_row_range.min <= idx && idx <= visible_row_range.max)
      {
        switch(entity->kind)
        {
          default:{}break;
          case DF_EntityKind_Process:
          {
            UI_NamedTableVectorF("process_%p", entity)
            {
              UI_TableCellSized(ui_pct(1, 0)) UI_FocusHot((row_is_selected) ? UI_FocusKind_On : UI_FocusKind_Off)
              {
                df_entity_desc_button(ws, entity, &items.v[idx].matches, query);
              }
            }
            idx_in_process = 0;
          }break;
          case DF_EntityKind_Module:
          UI_NamedTableVectorF("module_%p", entity)
          {
            UI_TableCell UI_TextAlignment(UI_TextAlign_Center) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            {
              ui_labelf("%I64u", idx_in_process);
            }
            UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 0) ? UI_FocusKind_On : UI_FocusKind_Off)
            {
              df_entity_desc_button(ws, entity, &items.v[idx].matches, query);
            }
            UI_TableCell UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_FocusHot((row_is_selected && cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
            {
              UI_Box *range_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|UI_BoxFlag_DrawText, "[0x%I64x, 0x%I64x)###vaddr_range_%p", entity->vaddr_rng.min, entity->vaddr_rng.max, entity);
              UI_Signal sig = ui_signal_from_box(range_box);
              if(ui_pressed(sig))
              {
                next_cursor = v2s64(1, (S64)idx+1);
                DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
              }
            }
            UI_TableCell
            {
              B32 txt_is_selected = (row_is_selected && cursor.x == 2);
              B32 brw_is_selected = (row_is_selected && cursor.x == 3);
              
              // rjf: unpack module info
              DF_Entity *binary = df_binary_file_from_module(entity);
              DBGI_Parse *dbgi = df_dbgi_parse_from_binary_file(scope, binary);
              B32 dbgi_is_valid = (dbgi->dbg_props.modified != 0);
              
              // rjf: begin editing
              if(txt_is_selected && edit_begin)
              {
                mv->txt_editing = 1;
                mv->txt_size = Min(sizeof(mv->txt_buffer), dbgi->dbg_path.size);
                MemoryCopy(mv->txt_buffer, dbgi->dbg_path.str, mv->txt_size);
                mv->txt_cursor = txt_pt(1, 1+mv->txt_size);
                mv->txt_mark = txt_pt(1, 1);
              }
              
              // rjf: build
              UI_Signal sig = {0};
              UI_FocusHot(txt_is_selected ? UI_FocusKind_On : UI_FocusKind_Off)
                UI_FocusActive((txt_is_selected && mv->txt_editing) ? UI_FocusKind_On : UI_FocusKind_Off)
                UI_WidthFill
              {
                UI_TextColor(!dbgi_is_valid ? df_rgba_from_theme_color(DF_ThemeColor_FailureBackground) : ui_top_text_color())
                  sig = df_line_editf(DF_LineEditFlag_NoBackground, 0, 0, &mv->txt_cursor, &mv->txt_mark, mv->txt_buffer, sizeof(mv->txt_buffer), &mv->txt_size, 0, dbgi->dbg_path, "###dbg_path_%p", entity);
                edit_commit = (edit_commit || ui_committed(sig));
              }
              
              // rjf: press -> focus
              if(ui_pressed(sig))
              {
                edit_commit = (mv->txt_editing && !txt_is_selected);
                next_cursor = v2s64(2, (S64)idx+1);
                DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
              }
              
              // rjf: double-click -> begin editing
              if(ui_double_clicked(sig) && !mv->txt_editing)
              {
                ui_kill_action();
                mv->txt_editing = 1;
                mv->txt_size = Min(sizeof(mv->txt_buffer), dbgi->dbg_path.size);
                MemoryCopy(mv->txt_buffer, dbgi->dbg_path.str, mv->txt_size);
                mv->txt_cursor = txt_pt(1, 1+mv->txt_size);
                mv->txt_mark = txt_pt(1, 1);
              }
              
              // rjf: store commit info
              if(txt_is_selected && edit_commit)
              {
                commit_module = entity;
              }
              
              // rjf: build browse button
              UI_FocusHot(brw_is_selected ? UI_FocusKind_On : UI_FocusKind_Off) UI_PrefWidth(ui_text_dim(10, 1))
              {
                if(ui_clicked(ui_buttonf("Browse...")) || (brw_is_selected && edit_begin))
                {
                  DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
                  params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PickFile);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
                  df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
                  mv->pick_file_dst_entity = df_handle_from_entity(entity);
                }
              }
            }
          }break;
        }
      }
    }
    cursor = next_cursor;
  }
  
  //- rjf: apply commits
  if(edit_commit)
  {
    mv->txt_editing = 0;
    if(!df_entity_is_nil(commit_module))
    {
      String8 exe_path = commit_module->name;
      String8 dbg_path = str8(mv->txt_buffer, mv->txt_size);
      dbgi_force_exe_path_dbg_path(exe_path, dbg_path);
    }
    if(edit_submit)
    {
      cursor.y += 1;
    }
  }
  
  //- rjf: apply edit state changes
  if(edit_end)
  {
    mv->txt_editing = 0;
  }
  
  //- rjf: selected num -> selected entity
  mv->selected_column = cursor.x;
  mv->selected_entity = (1 <= cursor.y && cursor.y <= items.count) ? df_handle_from_entity(items.v[cursor.y-1].entity) : df_handle_zero();
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: PendingEntity @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(PendingEntity)
{
  DF_PendingEntityViewState *pves = df_view_user_state(view, DF_PendingEntityViewState);
  pves->deferred_cmd_arena = df_view_push_arena_ext(view);
  pves->complete_cfg_arena = df_view_push_arena_ext(view);
  pves->complete_cfg_root = df_cfg_tree_copy(pves->complete_cfg_arena, cfg_root);
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(PendingEntity)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(PendingEntity)
{
  Temp scratch = scratch_begin(0, 0);
  DF_PendingEntityViewState *pves = df_view_user_state(view, DF_PendingEntityViewState);
  
  //- rjf: process commands
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    // rjf: process
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default:break;
      
      // rjf: pick file
      case DF_CoreCmdKind_PickFile:
      {
        DF_Entity *missing_file = df_entity_from_handle(pves->pick_file_override_target);
        String8 pick_string = cmd->params.file_path;
        if(!df_entity_is_nil(missing_file) && pick_string.size != 0)
        {
          DF_Entity *replacement = df_entity_from_path(pick_string, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
          view->entity = df_handle_from_entity(replacement);
          DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
          p.entity = df_handle_from_entity(missing_file);
          p.file_path = pick_string;
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetFileReplacementPath));
        }
      }break;
      
      // rjf: gather deferred commands to redispatch when entity is ready
      case DF_CoreCmdKind_GoToLine:
      case DF_CoreCmdKind_GoToAddress:
      case DF_CoreCmdKind_CenterCursor:
      case DF_CoreCmdKind_ContainCursor:
      {
        df_cmd_list_push(pves->deferred_cmd_arena, &pves->deferred_cmds, &cmd->params, cmd->spec);
      }break;
    }
  }
  
  //- rjf: determine if entity is ready, and which viewer to use
  DF_Entity *entity = df_entity_from_handle(view->entity);
  DF_GfxViewKind viewer_kind = DF_GfxViewKind_Null;
  B32 entity_is_ready = 0;
  switch(entity->kind)
  {
    default:{}break;
    case DF_EntityKind_File:
    {
      entity_is_ready = 1;
      viewer_kind = DF_GfxViewKind_Code;
    }break;
  }
  
  //- rjf: if entity is ready, dispatch all deferred commands
  if(entity_is_ready)
  {
    for(DF_CmdNode *cmd_node = pves->deferred_cmds.first; cmd_node != 0; cmd_node = cmd_node->next)
    {
      DF_Cmd *cmd = &cmd_node->cmd;
      df_push_cmd__root(&cmd->params, cmd->spec);
    }
    arena_clear(pves->deferred_cmd_arena);
    MemoryZeroStruct(&pves->deferred_cmds);
  }
  
  //- rjf: if entity is ready, move cfg tree to scratch for new command
  DF_CfgNode *cfg_root = &df_g_nil_cfg_node;
  if(entity_is_ready)
  {
    cfg_root = df_cfg_tree_copy(scratch.arena, pves->complete_cfg_root);
  }
  
  //- rjf: if entity is ready, replace this view with the correct one, if any viewer is specified
  if(entity_is_ready && viewer_kind != DF_GfxViewKind_Null)
  {
    DF_ViewSpec *view_spec = df_view_spec_from_string(cfg_root->string);
    if(view_spec == &df_g_nil_view_spec)
    {
      view_spec = df_view_spec_from_gfx_view_kind(viewer_kind);
    }
    df_view_equip_spec(ws, view, view_spec, entity, str8_lit(""), cfg_root);
    df_panel_notify_mutation(ws, panel);
  }
  
  //- rjf: if entity is ready, but we have no viewer for it, then just close this tab
  if(entity_is_ready && viewer_kind == DF_GfxViewKind_Null)
  {
    DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseTab));
  }
  
  scratch_end(scratch);
}

DF_VIEW_UI_FUNCTION_DEF(PendingEntity)
{
  // rjf: grab state
  DF_PendingEntityViewState *pves = df_view_user_state(view, DF_PendingEntityViewState);
  DF_Entity *entity = df_entity_from_handle(view->entity);
  
  // rjf: entity is missing -> notify user
  if(entity->flags & DF_EntityFlag_IsMissing)
  {
    UI_WidthFill UI_HeightFill UI_Column UI_Padding(ui_pct(1, 0))
    {
      Temp scratch = scratch_begin(0, 0);
      String8 full_path = df_full_path_from_entity(scratch.arena, entity);
      UI_PrefWidth(ui_children_sum(1)) UI_PrefHeight(ui_em(3, 1))
        UI_Row UI_Padding(ui_pct(1, 0))
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
      {
        UI_Font(ui_icon_font()) ui_label(df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
        ui_labelf("Could not find \"%S\".", full_path);
      }
      UI_PrefHeight(ui_em(3, 1))
        UI_Row UI_Padding(ui_pct(1, 0))
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_ActionText))
        UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBackground))
        UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBorder))
        UI_CornerRadius(ui_top_font_size()/3)
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_Focus(UI_FocusKind_On)
        if(ui_clicked(ui_buttonf("Find alternative...")))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PickFile);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
        pves->pick_file_override_target = df_handle_from_entity(entity);
      }
      scratch_end(scratch);
    }
  }
  
  // rjf: entity is still loading -> loading animation
  else
  {
    view->loading_t = view->loading_t_target = 1.f;
    df_gfx_request_frame();
  }
}

////////////////////////////////
//~ rjf: Code @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Code)
{
  // rjf: set up state
  DF_CodeViewState *tv = df_view_user_state(view, DF_CodeViewState);
  if(tv->initialized == 0)
  {
    tv->initialized = 1;
    tv->cursor = tv->mark = txt_pt(1, 1);
    tv->preferred_column = 1;
    tv->find_text_arena = df_view_push_arena_ext(view);
  }
  
  // rjf: deserialize cursor
  DF_CfgNode *cursor_cfg = df_cfg_node_child_from_string(cfg_root, str8_lit("cursor"), StringMatchFlag_CaseInsensitive);
  if(cursor_cfg != &df_g_nil_cfg_node)
  {
    TxtPt cursor = txt_pt(1, 1);
    cursor.line = s64_from_str8(cursor_cfg->first->string, 10);
    cursor.column = s64_from_str8(cursor_cfg->first->first->string, 10);
    if(cursor.line == 0) { cursor.line = 1; }
    if(cursor.column == 0) { cursor.column = 1; }
    tv->cursor = tv->mark = cursor;
    tv->center_cursor = 1;
  }
  
  // rjf: default to loading
  df_view_equip_loading_info(view, 1, 0, 0);
  view->loading_t = view->loading_t_target = 1.f;
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Code)
{
  DF_CodeViewState *tvs = df_view_user_state(view, DF_CodeViewState);
  String8 string = push_str8f(arena, " cursor:%I64d:%I64d", tvs->cursor.line, tvs->cursor.column);
  return string;
}

DF_VIEW_CMD_FUNCTION_DEF(Code)
{
  DF_CodeViewState *tv = df_view_user_state(view, DF_CodeViewState);
  DF_Entity *entity = df_entity_from_handle(view->entity);
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    // rjf: process
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default: break;
      case DF_CoreCmdKind_GoToLine:
      {
        tv->goto_line_num = cmd->params.text_point.line;
      }break;
      case DF_CoreCmdKind_CenterCursor:
      {
        tv->center_cursor = 1;
      }break;
      case DF_CoreCmdKind_ContainCursor:
      {
        tv->contain_cursor = 1;
      }break;
      case DF_CoreCmdKind_FindTextForward:
      {
        arena_clear(tv->find_text_arena);
        tv->find_text_fwd = push_str8_copy(tv->find_text_arena, cmd->params.string);
      }break;
      case DF_CoreCmdKind_FindTextBackward:
      {
        arena_clear(tv->find_text_arena);
        tv->find_text_bwd = push_str8_copy(tv->find_text_arena, cmd->params.string);
      }break;
      case DF_CoreCmdKind_ToggleBreakpointAtCursor:
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.entity = view->entity;
        params.text_point = tv->cursor;
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_TextBreakpoint));
        tv->contain_cursor = 1;
      }break;
      case DF_CoreCmdKind_ToggleWatchPinAtCursor:
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.entity = view->entity;
        params.text_point = tv->cursor;
        params.string = cmd->params.string;
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchPin));
        tv->contain_cursor = 1;
      }break;
      case DF_CoreCmdKind_RunToCursor:
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.entity = view->entity;
        params.text_point = tv->cursor;
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToLine));
      }break;
      case DF_CoreCmdKind_SetNextStatement:
      {
        Temp scratch = scratch_begin(0, 0);
        DF_Entity *thread = df_entity_from_handle(cmd->params.entity);
        S64 line_num = (cmd->params.text_point.line == 0 ? tv->cursor.line : cmd->params.text_point.line);
        DF_TextLineSrc2DasmInfoListArray src2dasm = df_text_line_src2dasm_info_list_array_from_src_line_range(scratch.arena, entity, r1s64(line_num, line_num));
        if(!df_entity_is_nil(thread) && src2dasm.count != 0)
        {
          DF_TextLineSrc2DasmInfoList *src2dasm_list = &src2dasm.v[0];
          if(src2dasm_list->first != 0)
          {
            Rng1U64 voff_rng = src2dasm_list->first->v.voff_range;
            DF_Entity *binary = src2dasm_list->first->v.binary;
            DF_EntityList possible_modules = df_modules_from_binary_file(scratch.arena, binary);
            DF_Entity *thread_dst_module = df_module_from_thread_candidates(thread, &possible_modules);
            U64 thread_dst_voff = voff_rng.min;
            if(!df_entity_is_nil(thread_dst_module) && thread_dst_voff != 0)
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(thread);
              params.vaddr = df_vaddr_from_voff(thread_dst_module, thread_dst_voff);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetThreadIP));
            }
          }
        }
        scratch_end(scratch);
      }break;
      case DF_CoreCmdKind_GoToNameAtCursor:
      {
        Temp scratch = scratch_begin(0, 0);
        TXT_Scope *txt_scope = txt_scope_open();
        HS_Scope *hs_scope = hs_scope_open();
        {
          // rjf: unpack entity info
          String8 path = df_full_path_from_entity(scratch.arena, entity);
          U128 key = fs_key_from_path(path);
          TXT_LangKind lang_kind = txt_lang_kind_from_extension(str8_skip_last_dot(path));
          U128 hash = {0};
          TXT_TextInfo text_info = txt_text_info_from_key_lang(txt_scope, key, lang_kind, &hash);
          String8 data = hs_data_from_hash(hs_scope, hash);
          
          // rjf: determine expression range
          Rng1U64 expr_range = {0};
          {
            TxtRng selection_range = txt_rng(tv->cursor, tv->mark);
            if(txt_pt_match(selection_range.min, selection_range.max))
            {
              expr_range = txt_expr_off_range_from_info_data_pt(&text_info, data, tv->cursor);
            }
            else
            {
              expr_range = r1u64(txt_off_from_info_pt(&text_info, selection_range.min), txt_off_from_info_pt(&text_info, selection_range.max));
            }
          }
          
          // rjf: expression range -> text
          String8 expr_text = str8_substr(data, expr_range);
          
          // rjf: go to name
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
          params.entity = df_handle_from_entity(entity);
          params.string = expr_text;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_GoToName));
        }
        hs_scope_close(hs_scope);
        txt_scope_close(txt_scope);
        scratch_end(scratch);
      }break;
      case DF_CoreCmdKind_ToggleWatchExpressionAtCursor:
      {
        Temp scratch = scratch_begin(0, 0);
        TXT_Scope *txt_scope = txt_scope_open();
        HS_Scope *hs_scope = hs_scope_open();
        {
          // rjf: unpack entity info
          String8 path = df_full_path_from_entity(scratch.arena, entity);
          U128 key = fs_key_from_path(path);
          TXT_LangKind lang_kind = txt_lang_kind_from_extension(str8_skip_last_dot(path));
          U128 hash = {0};
          TXT_TextInfo text_info = txt_text_info_from_key_lang(txt_scope, key, lang_kind, &hash);
          String8 data = hs_data_from_hash(hs_scope, hash);
          
          // rjf: determine expression range
          Rng1U64 expr_range = {0};
          {
            TxtRng selection_range = txt_rng(tv->cursor, tv->mark);
            if(txt_pt_match(selection_range.min, selection_range.max))
            {
              expr_range = txt_expr_off_range_from_info_data_pt(&text_info, data, tv->cursor);
            }
            else
            {
              expr_range = r1u64(txt_off_from_info_pt(&text_info, selection_range.min), txt_off_from_info_pt(&text_info, selection_range.max));
            }
          }
          
          // rjf: expression range -> text
          String8 expr_text = str8_substr(data, expr_range);
          
          // rjf: toggle watch expr
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
          params.string = expr_text;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchExpression));
          scratch_end(scratch);
        }
        hs_scope_close(hs_scope);
        txt_scope_close(txt_scope);
        scratch_end(scratch);
      }break;
      case DF_CoreCmdKind_PickFile:
      {
        DF_Entity *missing_file = df_entity_from_handle(tv->pick_file_override_target);
        String8 pick_string = cmd->params.file_path;
        if(!df_entity_is_nil(missing_file) && pick_string.size != 0)
        {
          DF_Entity *replacement = df_entity_from_path(pick_string, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
          view->entity = df_handle_from_entity(replacement);
          DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
          p.entity = df_handle_from_entity(missing_file);
          p.file_path = pick_string;
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetFileReplacementPath));
        }
      }break;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(Code)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  DBGI_Scope *dbgi_scope = dbgi_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  DF_CodeViewState *tv = df_view_user_state(view, DF_CodeViewState);
  
  //////////////////////////////
  //- rjf: extract invariants
  //
  DF_Entity *entity = df_entity_from_handle(view->entity);
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  F_Tag code_font = df_font_from_slot(DF_FontSlot_Code);
  F32 code_font_size = df_font_size_from_slot(ws, DF_FontSlot_Code);
  F_Metrics code_font_metrics = f_metrics_from_tag_size(code_font, code_font_size);
  F32 code_line_height = ceil_f32(f_line_height_from_metrics(&code_font_metrics) * 1.5f);
  F32 big_glyph_advance = f_dim_from_tag_size_string(code_font, code_font_size, str8_lit("H")).x;
  Vec2F32 panel_box_dim = dim_2f32(rect);
  Vec2F32 bottom_bar_dim = {panel_box_dim.x, ui_em(1.8f, 0).value};
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  Vec2F32 code_area_dim = v2f32(panel_box_dim.x - scroll_bar_dim, panel_box_dim.y - scroll_bar_dim - bottom_bar_dim.y);
  S64 num_possible_visible_lines = (S64)(code_area_dim.y/code_line_height)+1;
  
  //////////////////////////////
  //- rjf: unpack ctrl ctx & make parse ctx
  //
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  U64 unwind_count = ctrl_ctx.unwind_count;
  U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(dbgi_scope, process, rip_vaddr);
  
  //////////////////////////////
  //- rjf: unpack file/text entity info
  //
  B32 entity_is_missing = !!(entity->flags & DF_EntityFlag_IsMissing && !(entity->flags & DF_EntityFlag_Output));
  String8 path = df_full_path_from_entity(scratch.arena, entity);
  U128 key = fs_key_from_path(path);
  TXT_LangKind lang_kind = txt_lang_kind_from_extension(str8_skip_last_dot(path));
  U128 hash = {0};
  TXT_TextInfo text_info = txt_text_info_from_key_lang(txt_scope, key, lang_kind, &hash);
  String8 data = hs_data_from_hash(hs_scope, hash);
  B32 text_info_is_ready = (text_info.lines_count != 0);
  
  //////////////////////////////
  //- rjf: buffer is pending -> equip view with loading information
  //
  if(!entity_is_missing && !text_info_is_ready)
  {
    df_view_equip_loading_info(view, 1, text_info.bytes_processed, text_info.bytes_to_process);
  }
  
  //////////////////////////////
  //- rjf: determine visible line range / count
  //
  Rng1S64 visible_line_num_range = r1s64(view->scroll_pos.y.idx + (S64)(view->scroll_pos.y.off) + 1 - !!(view->scroll_pos.y.off < 0),
                                         view->scroll_pos.y.idx + (S64)(view->scroll_pos.y.off) + 1 + num_possible_visible_lines);
  Rng1S64 target_visible_line_num_range = r1s64(view->scroll_pos.y.idx + 1,
                                                view->scroll_pos.y.idx + 1 + num_possible_visible_lines);
  U64 visible_line_count = 0;
  {
    visible_line_num_range.min = Clamp(1, visible_line_num_range.min, (S64)text_info.lines_count);
    visible_line_num_range.max = Clamp(1, visible_line_num_range.max, (S64)text_info.lines_count);
    visible_line_num_range.min = Max(1, visible_line_num_range.min);
    visible_line_num_range.max = Max(1, visible_line_num_range.max);
    target_visible_line_num_range.min = Clamp(1, target_visible_line_num_range.min, (S64)text_info.lines_count);
    target_visible_line_num_range.max = Clamp(1, target_visible_line_num_range.max, (S64)text_info.lines_count);
    target_visible_line_num_range.min = Max(1, target_visible_line_num_range.min);
    target_visible_line_num_range.max = Max(1, target_visible_line_num_range.max);
    visible_line_count = (U64)dim_1s64(visible_line_num_range)+1;
  }
  
  //////////////////////////////
  //- rjf: calculate scroll bounds
  //
  S64 line_size_x = 0;
  Rng1S64 scroll_idx_rng[Axis2_COUNT] = {0};
  {
    line_size_x = (text_info.lines_max_size*big_glyph_advance*3)/2;
    line_size_x = ClampBot(line_size_x, (S64)big_glyph_advance*120);
    line_size_x = ClampBot(line_size_x, (S64)code_area_dim.x);
    scroll_idx_rng[Axis2_X] = r1s64(0, line_size_x-(S64)code_area_dim.x);
    scroll_idx_rng[Axis2_Y] = r1s64(0, (S64)text_info.lines_count-1);
  }
  
  //////////////////////////////
  //- rjf: calculate line-range-dependent info
  //
  F32 margin_width_px = big_glyph_advance*3.5f;
  F32 line_num_width_px = big_glyph_advance * (log10(visible_line_num_range.max) + 3);
  TXT_LineTokensSlice slice = txt_line_tokens_slice_from_info_data_line_range(scratch.arena, &text_info, data, visible_line_num_range);
  
  //////////////////////////////
  //- rjf: get active search query
  //
  String8 search_query = {0};
  Side search_query_side = Side_Invalid;
  B32 search_query_is_active = 0;
  {
    DF_CoreCmdKind query_cmd_kind = df_core_cmd_kind_from_string(ws->query_cmd_spec->info.string);
    if(query_cmd_kind == DF_CoreCmdKind_FindTextForward ||
       query_cmd_kind == DF_CoreCmdKind_FindTextBackward)
    {
      search_query = str8(ws->query_view_stack_top->query_buffer, ws->query_view_stack_top->query_string_size);
      search_query_is_active = 1;
      search_query_side = (query_cmd_kind == DF_CoreCmdKind_FindTextForward) ? Side_Max : Side_Min;
    }
  }
  
  //////////////////////////////
  //- rjf: prepare code slice info bundle, for the viewable region of text
  //
  DF_CodeSliceParams code_slice_params = {0};
  if(text_info_is_ready)
  {
    // rjf: fill basics
    code_slice_params.flags                     = DF_CodeSliceFlag_Margin|DF_CodeSliceFlag_LineNums|DF_CodeSliceFlag_Clickable;
    code_slice_params.line_num_range            = visible_line_num_range;
    code_slice_params.line_text                 = push_array(scratch.arena, String8, visible_line_count);
    code_slice_params.line_ranges               = push_array(scratch.arena, Rng1U64, visible_line_count);
    code_slice_params.line_tokens               = push_array(scratch.arena, TXT_TokenArray, visible_line_count);
    code_slice_params.line_bps                  = push_array(scratch.arena, DF_EntityList, visible_line_count);
    code_slice_params.line_ips                  = push_array(scratch.arena, DF_EntityList, visible_line_count);
    code_slice_params.line_pins                 = push_array(scratch.arena, DF_EntityList, visible_line_count);
    code_slice_params.line_dasm2src             = push_array(scratch.arena, DF_TextLineDasm2SrcInfoList, visible_line_count);
    code_slice_params.line_src2dasm             = push_array(scratch.arena, DF_TextLineSrc2DasmInfoList, visible_line_count);
    code_slice_params.font                      = code_font;
    code_slice_params.font_size                 = code_font_size;
    code_slice_params.line_height_px            = code_line_height;
    code_slice_params.search_query              = search_query;
    code_slice_params.margin_width_px           = margin_width_px;
    code_slice_params.line_num_width_px         = line_num_width_px;
    code_slice_params.line_text_max_width_px    = (F32)line_size_x;
    code_slice_params.flash_ranges              = df_push_entity_child_list_with_kind(scratch.arena, entity, DF_EntityKind_FlashMarker);
    code_slice_params.margin_float_off_px       = view->scroll_pos.x.idx + view->scroll_pos.x.off;
    if(code_slice_params.margin_float_off_px < 1)
    {
      code_slice_params.margin_float_off_px = 0;
    }
    
    // rjf: fill text info
    {
      S64 line_num = visible_line_num_range.min;
      U64 line_idx = visible_line_num_range.min-1;
      for(U64 visible_line_idx = 0; visible_line_idx < visible_line_count; visible_line_idx += 1, line_idx += 1, line_num += 1)
      {
        code_slice_params.line_text[visible_line_idx]   = str8_substr(data, text_info.lines_ranges[line_idx]);
        code_slice_params.line_ranges[visible_line_idx] = text_info.lines_ranges[line_idx];
        code_slice_params.line_tokens[visible_line_idx] = slice.line_tokens[visible_line_idx];
      }
    }
    
    // rjf: find visible breakpoints
    ProfScope("find visible breakpoints")
    {
      for(DF_Entity *bp = entity->first; !df_entity_is_nil(bp); bp = bp->next)
      {
        if(bp->deleted || bp->kind != DF_EntityKind_Breakpoint) { continue; }
        if(visible_line_num_range.min <= bp->text_point.line && bp->text_point.line <= visible_line_num_range.max)
        {
          U64 slice_line_idx = (bp->text_point.line-visible_line_num_range.min);
          df_entity_list_push(scratch.arena, &code_slice_params.line_bps[slice_line_idx], bp);
        }
      }
    }
    
    // rjf: find live threads mapping to this file
    ProfScope("find live threads mapping to this file")
    {
      DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx.thread);
      DF_EntityList threads = df_query_cached_entity_list_with_kind(DF_EntityKind_Thread);
      for(DF_EntityNode *thread_n = threads.first; thread_n != 0; thread_n = thread_n->next)
      {
        DF_Entity *thread = thread_n->entity;
        DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
        U64 unwind_count = (thread == selected_thread) ? ctrl_ctx.unwind_count : 0;
        U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
        U64 last_inst_on_unwound_rip_vaddr = rip_vaddr - !!unwind_count;
        DF_Entity *module = df_module_from_process_vaddr(process, last_inst_on_unwound_rip_vaddr);
        U64 rip_voff = df_voff_from_vaddr(module, last_inst_on_unwound_rip_vaddr);
        DF_Entity *binary = df_binary_file_from_module(module);
        DF_TextLineDasm2SrcInfo dasm2src_info = df_text_line_dasm2src_info_from_binary_voff(binary, rip_voff);
        if(dasm2src_info.file == entity && visible_line_num_range.min <= dasm2src_info.pt.line && dasm2src_info.pt.line <= visible_line_num_range.max)
        {
          U64 slice_line_idx = dasm2src_info.pt.line-visible_line_num_range.min;
          df_entity_list_push(scratch.arena, &code_slice_params.line_ips[slice_line_idx], thread);
        }
      }
    }
    
    // rjf: find visible watch pins
    ProfScope("find visible watch pins")
    {
      for(DF_Entity *wp = entity->first; !df_entity_is_nil(wp); wp = wp->next)
      {
        if(wp->deleted || wp->kind != DF_EntityKind_WatchPin) { continue; }
        if(visible_line_num_range.min <= wp->text_point.line && wp->text_point.line <= visible_line_num_range.max)
        {
          U64 slice_line_idx = (wp->text_point.line-visible_line_num_range.min);
          df_entity_list_push(scratch.arena, &code_slice_params.line_pins[slice_line_idx], wp);
        }
      }
    }
    
    // rjf: find all src -> dasm info
    ProfScope("find all src -> dasm info")
    {
      DF_TextLineSrc2DasmInfoListArray src2dasm = df_text_line_src2dasm_info_list_array_from_src_line_range(scratch.arena, entity, visible_line_num_range);
      if(src2dasm.count != 0)
      {
        MemoryCopy(code_slice_params.line_src2dasm, src2dasm.v, sizeof(DF_TextLineSrc2DasmInfoList)*src2dasm.count);
      }
      code_slice_params.relevant_binaries = src2dasm.binaries;
    }
  }
  
  //////////////////////////////
  //- rjf: build missing & override interface
  //
  if(entity_is_missing)
  {
    UI_WidthFill UI_HeightFill UI_Column UI_Padding(ui_pct(1, 0))
    {
      Temp scratch = scratch_begin(0, 0);
      String8 full_path = df_full_path_from_entity(scratch.arena, entity);
      UI_PrefWidth(ui_children_sum(1)) UI_PrefHeight(ui_em(3, 1))
        UI_Row UI_Padding(ui_pct(1, 0))
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
      {
        UI_Font(ui_icon_font()) ui_label(df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
        ui_labelf("Could not find \"%S\".", full_path);
      }
      UI_PrefHeight(ui_em(3, 1))
        UI_Row UI_Padding(ui_pct(1, 0))
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_ActionText))
        UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBackground))
        UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBorder))
        UI_CornerRadius(ui_top_font_size()/3)
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_Focus(UI_FocusKind_On)
        if(ui_clicked(ui_buttonf("Find alternative...")))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PickFile);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
        tv->pick_file_override_target = df_handle_from_entity(entity);
      }
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: build container
  //
  UI_Box *container_box = &ui_g_nil_box;
  if(text_info_is_ready)
  {
    ui_set_next_pref_width(ui_px(code_area_dim.x, 1));
    ui_set_next_pref_height(ui_px(code_area_dim.y, 1));
    ui_set_next_child_layout_axis(Axis2_Y);
    container_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|
                                              UI_BoxFlag_Scroll|
                                              UI_BoxFlag_AllowOverflowX|
                                              UI_BoxFlag_AllowOverflowY,
                                              "###code_area_%p", view);
  }
  
  //////////////////////////////
  //- rjf: cancelled search query -> center cursor
  //
  if(!search_query_is_active && tv->drifted_for_search)
  {
    tv->drifted_for_search = 0;
    tv->center_cursor = 1;
  }
  
  //////////////////////////////
  //- rjf: do searching operations
  //
  if(text_info_is_ready)
  {
    //- rjf: find text (forward)
    if(tv->find_text_fwd.size != 0)
    {
      Temp scratch = scratch_begin(0, 0);
      B32 found = 0;
      B32 first = 1;
      S64 line_num_start = tv->cursor.line;
      S64 line_num_last = (S64)text_info.lines_count;
      for(S64 line_num = line_num_start;; first = 0)
      {
        // rjf: pop scratch
        temp_end(scratch);
        
        // rjf: gather line info
        String8 line_string = str8_substr(data, text_info.lines_ranges[line_num-1]);
        U64 search_start = 0;
        if(tv->cursor.line == line_num && first)
        {
          search_start = tv->cursor.column;
        }
        
        // rjf: search string
        U64 needle_pos = str8_find_needle(line_string, search_start, tv->find_text_fwd, StringMatchFlag_CaseInsensitive);
        if(needle_pos < line_string.size)
        {
          tv->cursor.line = line_num;
          tv->cursor.column = needle_pos+1;
          tv->mark = tv->cursor;
          found = 1;
          break;
        }
        
        // rjf: break if circled back around to cursor
        else if(line_num == line_num_start && !first)
        {
          break;
        }
        
        // rjf: increment
        line_num += 1;
        if(line_num > line_num_last)
        {
          line_num = 1;
        }
      }
      tv->center_cursor = found;
      if(found == 0)
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.string = push_str8f(scratch.arena, "Could not find \"%S\"", tv->find_text_fwd);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
      }
      scratch_end(scratch);
    }
    
    //- rjf: find text (backward)
    if(tv->find_text_bwd.size != 0)
    {
      Temp scratch = scratch_begin(0, 0);
      B32 found = 0;
      B32 first = 1;
      S64 line_num_start = tv->cursor.line;
      S64 line_num_last = (S64)text_info.lines_count;
      for(S64 line_num = line_num_start;; first = 0)
      {
        // rjf: pop scratch
        temp_end(scratch);
        
        // rjf: gather line info
        String8 line_string = str8_substr(data, text_info.lines_ranges[line_num-1]);
        if(tv->cursor.line == line_num && first)
        {
          line_string = str8_prefix(line_string, tv->cursor.column-1);
        }
        
        // rjf: search string
        U64 next_needle_pos = line_string.size;
        for(U64 needle_pos = 0; needle_pos < line_string.size;)
        {
          needle_pos = str8_find_needle(line_string, needle_pos, tv->find_text_bwd, StringMatchFlag_CaseInsensitive);
          if(needle_pos < line_string.size)
          {
            next_needle_pos = needle_pos;
            needle_pos += 1;
          }
        }
        if(next_needle_pos < line_string.size)
        {
          tv->cursor.line = line_num;
          tv->cursor.column = next_needle_pos+1;
          tv->mark = tv->cursor;
          found = 1;
          break;
        }
        
        // rjf: break if circled back around to cursor line
        else if(line_num == line_num_start && !first)
        {
          break;
        }
        
        // rjf: increment
        line_num -= 1;
        if(line_num == 0)
        {
          line_num = line_num_last;
        }
      }
      tv->center_cursor = found;
      if(found == 0)
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.string = push_str8f(scratch.arena, "Could not find \"%S\"", tv->find_text_bwd);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
      }
      scratch_end(scratch);
    }
    
    MemoryZeroStruct(&tv->find_text_fwd);
    MemoryZeroStruct(&tv->find_text_bwd);
    arena_clear(tv->find_text_arena);
  }
  
  //////////////////////////////
  //- rjf: do goto line
  //
  if(text_info_is_ready) if(tv->goto_line_num != 0)
  {
    S64 line_num = tv->goto_line_num;
    tv->goto_line_num = 0;
    line_num = Clamp(1, line_num, text_info.lines_count);
    tv->cursor = tv->mark = txt_pt(line_num, 1);
    tv->center_cursor = !tv->contain_cursor || (line_num < target_visible_line_num_range.min+4 || target_visible_line_num_range.max-4 < line_num);
  }
  
  //////////////////////////////
  //- rjf: do keyboard interaction
  //
  B32 snap[Axis2_COUNT] = {0};
  UI_Focus(UI_FocusKind_On)
  {
    if(ui_is_focus_active() && text_info_is_ready && visible_line_num_range.max >= visible_line_num_range.min)
    {
      snap[Axis2_X] = snap[Axis2_Y] = df_do_txt_controls(&text_info, data, ClampBot(num_possible_visible_lines, 10) - 10, &tv->cursor, &tv->mark, &tv->preferred_column);
    }
  }
  
  //////////////////////////////
  //- rjf: build container contents
  //
  if(text_info_is_ready) UI_Parent(container_box)
  {
    //- rjf: build fractional space
    container_box->view_off.x = container_box->view_off_target.x = view->scroll_pos.x.idx + view->scroll_pos.x.off;
    container_box->view_off.y = container_box->view_off_target.y = code_line_height*mod_f32(view->scroll_pos.y.off, 1.f) + code_line_height*(view->scroll_pos.y.off < 0) - code_line_height*(view->scroll_pos.y.off == -1.f && view->scroll_pos.y.idx == 1);
    
    //- rjf: build code slice
    DF_CodeSliceSignal sig = {0};
    UI_Focus(UI_FocusKind_On)
    {
      sig = df_code_slicef(ws, &ctrl_ctx, &parse_ctx, &code_slice_params, &tv->cursor, &tv->mark, &tv->preferred_column, "txt_view_%p", view);
    }
    
    //- rjf: hover eval
    if(!ui_dragging(sig.base) && sig.mouse_expr_rng.min.line != 0 && sig.base.event_flags == 0)
    {
      TxtRng expr_rng = sig.mouse_expr_rng;
      String8 expr = txt_string_from_info_data_txt_rng(&text_info, data, expr_rng);
      if(expr.size != 0)
      {
        DF_Eval eval = df_eval_from_string(scratch.arena, dbgi_scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, expr);
        if(eval.mode != EVAL_EvalMode_NULL)
        {
          df_set_hover_eval(ws, sig.mouse_expr_baseline_pos, ctrl_ctx, entity, sig.mouse_pt, 0, expr);
        }
      }
    }
    
    //- rjf: press code slice? -> focus panel
    if(ui_pressed(sig.base))
    {
      DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
    }
    
    //- rjf: dragging? -> contain cursor
    if(ui_dragging(sig.base) && sig.base.event_flags == 0)
    {
      tv->contain_cursor = 1;
    }
    
    //- rjf: ctrl+pressed? -> go to name
    if(ui_pressed(sig.base) && sig.base.event_flags & OS_EventFlag_Ctrl)
    {
      ui_kill_action();
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.entity = df_handle_from_entity(entity);
      params.string = txt_string_from_info_data_txt_rng(&text_info, data, sig.mouse_expr_rng);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_GoToName));
    }
    
    //- rjf: clicked margin? -> place breakpoint
    if(sig.clicked_margin_line_num != 0)
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.text_point = txt_pt(sig.clicked_margin_line_num, 1);
      params.entity = df_handle_from_entity(entity);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_TextBreakpoint));
    }
    
    //- rjf: dropped entity onto line? -> do drop
    if(sig.dropped_entity_line_num != 0 && !df_entity_is_nil(sig.dropped_entity))
    {
      DF_Entity *dropped_entity = sig.dropped_entity;
      switch(dropped_entity->kind)
      {
        default:{}break;
        case DF_EntityKind_Breakpoint:
        case DF_EntityKind_WatchPin:
        {
          DF_StateDeltaHistory *hist = df_state_delta_history();
          df_state_delta_history_push_batch(hist, &dropped_entity->generation);
          df_state_delta_history_push_struct_delta(hist, &dropped_entity->text_point);
          df_entity_change_parent(hist, dropped_entity, dropped_entity->parent, entity);
          df_entity_equip_txt_pt(dropped_entity, txt_pt(sig.dropped_entity_line_num, 1));
          if(dropped_entity->flags & DF_EntityFlag_HasVAddr)
          {
            df_state_delta_history_push_struct_delta(hist, &dropped_entity->vaddr);
            df_entity_equip_vaddr(dropped_entity, 0);
          }
        }break;
        case DF_EntityKind_Thread:
        if(contains_1s64(visible_line_num_range, sig.dropped_entity_line_num))
        {
          U64 line_idx = (sig.dropped_entity_line_num-visible_line_num_range.min);
          DF_TextLineSrc2DasmInfoList *src2dasm_list = &code_slice_params.line_src2dasm[line_idx];
          if(src2dasm_list->first != 0)
          {
            Rng1U64 voff_rng = src2dasm_list->first->v.voff_range;
            DF_Entity *binary = src2dasm_list->first->v.binary;
            DF_EntityList possible_modules = df_modules_from_binary_file(scratch.arena, binary);
            DF_Entity *thread_dst_module = df_module_from_thread_candidates(dropped_entity, &possible_modules);
            U64 thread_dst_voff = voff_rng.min;
            if(!df_entity_is_nil(thread_dst_module) && thread_dst_voff != 0)
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(dropped_entity);
              params.vaddr = df_vaddr_from_voff(thread_dst_module, thread_dst_voff);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetThreadIP));
            }
          }
        }break;
      }
    }
    
    //- rjf: copy text
    if(!txt_pt_match(sig.copy_range.min, sig.copy_range.max))
    {
      String8 text = txt_string_from_info_data_txt_rng(&text_info, data, sig.copy_range);
      os_set_clipboard_text(text);
    }
    
    //- rjf: toggle cursor watch
    if(sig.toggle_cursor_watch)
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchExpressionAtCursor));
    }
    
    //- rjf: set next statement
    if(sig.set_next_statement_line_num != 0 && contains_1s64(visible_line_num_range, sig.set_next_statement_line_num))
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.text_point = txt_pt(sig.set_next_statement_line_num, 1);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetNextStatement));
    }
    
    //- rjf: run-to-line
    if(sig.run_to_line_num != 0 && contains_1s64(visible_line_num_range, sig.run_to_line_num))
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.entity = df_handle_from_entity(entity);
      params.text_point = txt_pt(sig.run_to_line_num, 1);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToLine));
    }
    
    //- rjf: go to disasm
    if(sig.goto_disasm_line_num != 0 && contains_1s64(visible_line_num_range, sig.goto_disasm_line_num))
    {
      U64 line_idx = (sig.goto_disasm_line_num-visible_line_num_range.min);
      DF_TextLineSrc2DasmInfoList *src2dasm_list = &code_slice_params.line_src2dasm[line_idx];
      if(src2dasm_list->first != 0)
      {
        Rng1U64 voff_rng = src2dasm_list->first->v.voff_range;
        DF_Entity *binary = src2dasm_list->first->v.binary;
        DF_EntityList possible_modules = df_modules_from_binary_file(scratch.arena, binary);
        DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
        DF_Entity *thread_dst_module = df_module_from_thread_candidates(thread, &possible_modules);
        DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
        DF_Entity *module = thread_dst_module;
        if(df_entity_is_nil(module))
        {
          module = df_first_entity_from_list(&possible_modules);
        }
        U64 voff = voff_rng.min;
        if(!df_entity_is_nil(module) && voff != 0)
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(process);
          params.vaddr = df_vaddr_from_voff(module, voff);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: apply post-build view snapping rules
  //
  if(text_info_is_ready)
  {
    // rjf: contain => snap
    if(tv->contain_cursor)
    {
      tv->contain_cursor = 0;
      snap[Axis2_X] = 1;
      snap[Axis2_Y] = 1;
    }
    
    // rjf: center cursor
    if(tv->center_cursor)
    {
      tv->center_cursor = 0;
      String8 cursor_line = str8_substr(data, text_info.lines_ranges[tv->cursor.line-1]);
      F32 cursor_advance = f_dim_from_tag_size_string(code_font, code_font_size, str8_prefix(cursor_line, tv->cursor.column-1)).x;
      
      // rjf: scroll x
      {
        S64 new_idx = (S64)(cursor_advance - code_area_dim.x/2);
        new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
        ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
        snap[Axis2_X] = 0;
      }
      
      // rjf: scroll y
      {
        S64 new_idx = (tv->cursor.line-1) - num_possible_visible_lines/2 + 2;
        new_idx = Clamp(scroll_idx_rng[Axis2_Y].min, new_idx, scroll_idx_rng[Axis2_Y].max);
        ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
        snap[Axis2_Y] = 0;
      }
    }
    
    // rjf: snap in X
    if(snap[Axis2_X])
    {
      String8 cursor_line = str8_substr(data, text_info.lines_ranges[tv->cursor.line-1]);
      S64 cursor_off = (S64)(f_dim_from_tag_size_string(code_font, code_font_size, str8_prefix(cursor_line, tv->cursor.column-1)).x + margin_width_px + line_num_width_px);
      Rng1S64 visible_pixel_range =
      {
        view->scroll_pos.x.idx,
        view->scroll_pos.x.idx + (S64)code_area_dim.x,
      };
      Rng1S64 cursor_pixel_range =
      {
        cursor_off - (S64)(big_glyph_advance*4) - (S64)(margin_width_px + line_num_width_px),
        cursor_off + (S64)(big_glyph_advance*4),
      };
      S64 min_delta = Min(0, cursor_pixel_range.min - visible_pixel_range.min);
      S64 max_delta = Max(0, cursor_pixel_range.max - visible_pixel_range.max);
      S64 new_idx = view->scroll_pos.x.idx+min_delta+max_delta;
      new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
      ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
    }
    
    // rjf: snap in Y
    if(snap[Axis2_Y])
    {
      Rng1S64 cursor_visibility_range = r1s64(tv->cursor.line-4, tv->cursor.line+4);
      cursor_visibility_range.min = ClampBot(0, cursor_visibility_range.min);
      cursor_visibility_range.max = ClampBot(0, cursor_visibility_range.max);
      S64 min_delta = Min(0, cursor_visibility_range.min-(target_visible_line_num_range.min));
      S64 max_delta = Max(0, cursor_visibility_range.max-(target_visible_line_num_range.min+num_possible_visible_lines));
      S64 new_idx = view->scroll_pos.y.idx+min_delta+max_delta;
      new_idx = Clamp(0, new_idx, (S64)text_info.lines_count-1);
      ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: build horizontal scroll bar
  //
  if(text_info_is_ready)
  {
    ui_set_next_fixed_x(0);
    ui_set_next_fixed_y(code_area_dim.y);
    ui_set_next_fixed_width(panel_box_dim.x - scroll_bar_dim);
    ui_set_next_fixed_height(scroll_bar_dim);
    {
      view->scroll_pos.x = ui_scroll_bar(Axis2_X,
                                         ui_px(scroll_bar_dim, 1.f),
                                         view->scroll_pos.x,
                                         scroll_idx_rng[Axis2_X],
                                         (S64)code_area_dim.x);
    }
  }
  
  //////////////////////////////
  //- rjf: build vertical scroll bar
  //
  if(text_info_is_ready)
  {
    ui_set_next_fixed_x(code_area_dim.x);
    ui_set_next_fixed_y(0);
    ui_set_next_fixed_width(scroll_bar_dim);
    ui_set_next_fixed_height(panel_box_dim.y - bottom_bar_dim.y - scroll_bar_dim);
    {
      view->scroll_pos.y = ui_scroll_bar(Axis2_Y,
                                         ui_px(scroll_bar_dim, 1.f),
                                         view->scroll_pos.y,
                                         scroll_idx_rng[Axis2_Y],
                                         num_possible_visible_lines);
    }
  }
  
  //////////////////////////////
  //- rjf: top-level container interaction (scrolling)
  //
  if(text_info_is_ready)
  {
    UI_Signal sig = ui_signal_from_box(container_box);
    if(sig.scroll.x != 0)
    {
      S64 new_idx = view->scroll_pos.x.idx+sig.scroll.x*big_glyph_advance;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_X], new_idx);
      ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
    }
    if(sig.scroll.y != 0)
    {
      S64 new_idx = view->scroll_pos.y.idx + sig.scroll.y;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_Y], new_idx);
      ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
    }
    ui_scroll_pt_clamp_idx(&view->scroll_pos.x, scroll_idx_rng[Axis2_X]);
    ui_scroll_pt_clamp_idx(&view->scroll_pos.y, scroll_idx_rng[Axis2_Y]);
  }
  
  //////////////////////////////
  //- rjf: determine up-to-dateness of source file
  //
  B32 file_is_out_of_date = 0;
  String8 out_of_date_binary_name = {0};
  for(DF_EntityNode *n = code_slice_params.relevant_binaries.first; n != 0; n = n->next)
  {
    DF_Entity *binary = n->entity;
    if(!df_entity_is_nil(binary))
    {
      String8 full_path = df_full_path_from_entity(scratch.arena, entity);
      TXTI_Handle handle = txti_handle_from_path(full_path);
      TXTI_BufferInfo info = txti_buffer_info_from_handle(scratch.arena, handle);
      DBGI_Parse *parse = df_dbgi_parse_from_binary_file(dbgi_scope, binary);
      if(parse->exe_props.modified < info.timestamp)
      {
        file_is_out_of_date = 1;
        out_of_date_binary_name = binary->name;
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build bottom info bar
  //
  if(text_info_is_ready)
  {
    ui_set_next_fixed_x(0);
    ui_set_next_fixed_y(code_area_dim.y + scroll_bar_dim);
    ui_set_next_pref_width(ui_px(bottom_bar_dim.x, 1));
    ui_set_next_pref_height(ui_px(bottom_bar_dim.y, 1));
    ui_set_next_background_color(df_rgba_from_theme_color(file_is_out_of_date ? DF_ThemeColor_FailureBackground : DF_ThemeColor_AltBackground));
    ui_set_next_flags(UI_BoxFlag_DrawBackground);
    UI_Row
      UI_TextAlignment(UI_TextAlign_Center)
      UI_PrefWidth(ui_text_dim(10, 1))
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      String8 full_path = df_full_path_from_entity(scratch.arena, entity);
      TXTI_Handle handle = txti_handle_from_path(full_path);
      TXTI_BufferInfo info = txti_buffer_info_from_handle(scratch.arena, handle);
      if(file_is_out_of_date)
      {
        UI_Box *box = &ui_g_nil_box;
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureText))
          UI_Font(df_font_from_slot(DF_FontSlot_Icons))
        {
          box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_Clickable, "%S###file_ood_warning", df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
        }
        UI_Signal sig = ui_signal_from_box(box);
        if(ui_hovering(sig)) UI_Tooltip
        {
          UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(1, 1))
          {
            ui_labelf("This file has changed since ", out_of_date_binary_name);
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_Highlight0)) ui_label(out_of_date_binary_name);
            ui_labelf(" was built.");
          }
        }
      }
      UI_Font(code_font)
      {
        ui_label(full_path);
        ui_spacer(ui_em(1.5f, 1));
        ui_labelf("Row: %I64d, Col: %I64d", tv->cursor.line, tv->cursor.column);
        ui_spacer(ui_pct(1, 0));
        ui_labelf("(read only)");
        ui_labelf("%s",
                  info.line_end_kind == TXT_LineEndKind_LF   ? "lf" :
                  info.line_end_kind == TXT_LineEndKind_CRLF ? "crlf" :
                  "bin");
      }
    }
  }
  
  txt_scope_close(txt_scope);
  dbgi_scope_close(dbgi_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Disassembly @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Disassembly)
{
  DF_DisasmViewState *dv = df_view_user_state(view, DF_DisasmViewState);
  if(dv->initialized == 0)
  {
    dv->initialized = 1;
    dv->cursor = txt_pt(1, 1);
    dv->mark = txt_pt(1, 1);
    dv->preferred_column = 1;
    dv->find_text_arena = df_view_push_arena_ext(view);
    dv->style_flags = DASM_StyleFlag_Addresses|DASM_StyleFlag_SourceFilesNames|DASM_StyleFlag_SourceLines|DASM_StyleFlag_SymbolNames;
  }
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Disassembly)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(Disassembly)
{
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  DASM_Scope *dasm_scope = dasm_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  DF_DisasmViewState *dv = df_view_user_state(view, DF_DisasmViewState);
  
  //////////////////////////////
  //- rjf: unpack disassembly info
  //
  DF_Entity *process = df_entity_from_handle(dv->process);
  Architecture arch = df_architecture_from_entity(process);
  U64 dasm_base_vaddr = AlignDownPow2(dv->base_vaddr, KB(64));
  DF_Entity *dasm_module = df_module_from_process_vaddr(process,  dasm_base_vaddr);
  DF_Entity *dasm_binary = df_binary_file_from_module(dasm_module);
  Rng1U64 dasm_vaddr_range = r1u64(dasm_base_vaddr, dasm_base_vaddr+KB(64));
  U128 dasm_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, dasm_vaddr_range, 0);
  U128 dasm_data_hash = {0};
  DASM_Params dasm_params = {0};
  {
    dasm_params.vaddr = dasm_vaddr_range.min;
    dasm_params.arch = arch;
    dasm_params.style_flags = dv->style_flags;
    dasm_params.syntax = DASM_Syntax_Intel;
    dasm_params.base_vaddr = dasm_module->vaddr_rng.min;
    dasm_params.exe_path = df_full_path_from_entity(scratch.arena, dasm_binary);
  }
  DASM_Info dasm_info = dasm_info_from_key_params(dasm_scope, dasm_key, &dasm_params, &dasm_data_hash);
  U128 dasm_text_hash = {0};
  TXT_TextInfo dasm_text_info = txt_text_info_from_key_lang(txt_scope, dasm_info.text_key, txt_lang_kind_from_architecture(arch), &dasm_text_hash);
  String8 dasm_text_data = hs_data_from_hash(hs_scope, dasm_text_hash);
  
  //////////////////////////////
  //- rjf: process commands
  //
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    DF_CmdParams params = cmd->params;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    // rjf: process
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default: break;
      case DF_CoreCmdKind_GoToAddress:
      {
        DF_Entity *entity = df_entity_from_handle(params.entity);
        if(!df_entity_is_nil(entity) &&
           (entity->kind == DF_EntityKind_Process ||
            entity->kind == DF_EntityKind_Thread ||
            entity->kind == DF_EntityKind_Module))
        {
          DF_Entity *process = entity;
          if(entity->kind == DF_EntityKind_Thread ||
             entity->kind == DF_EntityKind_Module)
          {
            process = df_entity_ancestor_from_kind(process, DF_EntityKind_Process);
          }
          dv->process = df_handle_from_entity(process);
        }
        dv->base_vaddr = params.vaddr;
        dv->goto_vaddr = params.vaddr;
        dv->cursor = dv->mark = txt_pt(1, 1);
      }break;
      case DF_CoreCmdKind_GoToLine:
      {
        dv->goto_line_num = cmd->params.text_point.line;
      }break;
      case DF_CoreCmdKind_CenterCursor:
      {
        dv->center_cursor = 1;
      }break;
      case DF_CoreCmdKind_ContainCursor:
      {
        dv->contain_cursor = 1;
      }break;
      case DF_CoreCmdKind_FindTextForward:
      {
        arena_clear(dv->find_text_arena);
        dv->find_text_fwd = push_str8_copy(dv->find_text_arena, cmd->params.string);
      }break;
      case DF_CoreCmdKind_FindTextBackward:
      {
        arena_clear(dv->find_text_arena);
        dv->find_text_bwd = push_str8_copy(dv->find_text_arena, cmd->params.string);
      }break;
      case DF_CoreCmdKind_ToggleBreakpointAtCursor:
      {
        if(1 <= dv->cursor.line && dv->cursor.line <= dasm_info.insts.count)
        {
          U64 off = dasm_inst_array_code_off_from_idx(&dasm_info.insts, dv->cursor.line-1);
          U64 vaddr = dasm_vaddr_range.min+off;
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
          params.vaddr = vaddr;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddressBreakpoint));
          dv->contain_cursor = 1;
        }
      }break;
      case DF_CoreCmdKind_ToggleWatchPinAtCursor:
      {
        if(1 <= dv->cursor.line && dv->cursor.line <= dasm_info.insts.count)
        {
          U64 off = dasm_inst_array_code_off_from_idx(&dasm_info.insts, dv->cursor.line-1);
          U64 vaddr = dasm_vaddr_range.min+off;
          DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
          p.vaddr = vaddr;
          p.string = params.string;
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_VirtualAddr);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchPin));
          dv->contain_cursor = 1;
        }
      }break;
      case DF_CoreCmdKind_RunToCursor:
      {
        if(1 <= dv->cursor.line && dv->cursor.line <= dasm_info.insts.count)
        {
          U64 off = dasm_inst_array_code_off_from_idx(&dasm_info.insts, dv->cursor.line-1);
          U64 vaddr = dasm_vaddr_range.min+off;
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
          params.vaddr = vaddr;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToAddress));
        }
      }break;
      case DF_CoreCmdKind_SetNextStatement:
      {
        S64 line_num = (cmd->params.text_point.line == 0 ? dv->cursor.line : cmd->params.text_point.line);
        DF_Entity *thread = df_entity_from_handle(params.entity);
        if(1 <= line_num && line_num <= dasm_info.insts.count)
        {
          U64 off = dasm_inst_array_code_off_from_idx(&dasm_info.insts, line_num-1);
          U64 vaddr = dasm_vaddr_range.min+off;
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
          params.vaddr = vaddr;
          params.entity = df_handle_from_entity(thread);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetThreadIP));
        }
      }break;
      case DF_CoreCmdKind_ToggleCodeBytesVisibility:
      {
        dv->style_flags ^= DASM_StyleFlag_CodeBytes;
      }break;
      case DF_CoreCmdKind_ToggleAddressVisibility:
      {
        dv->style_flags ^= DASM_StyleFlag_Addresses;
      }break;
      case DF_CoreCmdKind_GoToNameAtCursor:
      {
        // rjf: determine expression range
        Rng1U64 expr_range = {0};
        {
          TxtRng selection_range = txt_rng(dv->cursor, dv->mark);
          if(txt_pt_match(selection_range.min, selection_range.max))
          {
            expr_range = txt_expr_off_range_from_info_data_pt(&dasm_text_info, dasm_text_data, dv->cursor);
          }
          else
          {
            expr_range = r1u64(txt_off_from_info_pt(&dasm_text_info, selection_range.min), txt_off_from_info_pt(&dasm_text_info, selection_range.max));
          }
        }
        
        // rjf: expression range -> text
        String8 expr_text = str8_substr(dasm_text_data, expr_range);
        
        // rjf: go to name
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.string = expr_text;
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_GoToName));
      }break;
      case DF_CoreCmdKind_ToggleWatchExpressionAtCursor:
      {
        // rjf: determine expression range
        Rng1U64 expr_range = {0};
        {
          TxtRng selection_range = txt_rng(dv->cursor, dv->mark);
          if(txt_pt_match(selection_range.min, selection_range.max))
          {
            expr_range = txt_expr_off_range_from_info_data_pt(&dasm_text_info, dasm_text_data, dv->cursor);
          }
          else
          {
            expr_range = r1u64(txt_off_from_info_pt(&dasm_text_info, selection_range.min), txt_off_from_info_pt(&dasm_text_info, selection_range.max));
          }
        }
        
        // rjf: expression range -> text
        String8 expr_text = str8_substr(dasm_text_data, expr_range);
        
        // rjf: toggle watch expr
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.string = expr_text;
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchExpression));
      }break;
    }
  }
  
  hs_scope_close(hs_scope);
  dasm_scope_close(dasm_scope);
  txt_scope_close(txt_scope);
  scratch_end(scratch);
}

DF_VIEW_UI_FUNCTION_DEF(Disassembly)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  DASM_Scope *dasm_scope = dasm_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  DBGI_Scope *dbgi_scope = dbgi_scope_open();
  DF_DisasmViewState *dv = df_view_user_state(view, DF_DisasmViewState);
  
  //////////////////////////////
  //- rjf: extract invariants
  //
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  F_Tag code_font = df_font_from_slot(DF_FontSlot_Code);
  F32 code_font_size = df_font_size_from_slot(ws, DF_FontSlot_Code);
  F_Metrics code_font_metrics = f_metrics_from_tag_size(code_font, code_font_size);
  F32 code_line_height = ceil_f32(f_line_height_from_metrics(&code_font_metrics) * 1.5f);
  F32 big_glyph_advance = f_dim_from_tag_size_string(code_font, code_font_size, str8_lit("H")).x;
  Vec2F32 panel_box_dim = dim_2f32(rect);
  Vec2F32 bottom_bar_dim = {panel_box_dim.x, ui_top_font_size()*1.8f};
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  Vec2F32 code_area_dim = v2f32(panel_box_dim.x - scroll_bar_dim, panel_box_dim.y - scroll_bar_dim - bottom_bar_dim.y);
  S64 num_possible_visible_lines = (S64)(code_area_dim.y/code_line_height)+1;
  
  //////////////////////////////
  //- rjf: unpack ctrl ctx & make parse ctx
  //
  DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx.thread);
  DF_Entity *selected_process = df_entity_ancestor_from_kind(selected_thread, DF_EntityKind_Process);
  U64 unwind_count = ctrl_ctx.unwind_count;
  U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(selected_thread, unwind_count);
  EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(dbgi_scope, selected_process, rip_vaddr);
  
  //////////////////////////////
  //- rjf: no disasm process open? -> snap to selected thread
  //
  if(df_entity_is_nil(df_entity_from_handle(dv->process)))
  {
    dv->process = df_handle_from_entity(selected_process);
    dv->base_vaddr = rip_vaddr;
    dv->goto_vaddr = rip_vaddr;
  }
  
  //////////////////////////////
  //- rjf: unpack disassembly info
  //
  DF_Entity *process = df_entity_from_handle(dv->process);
  Architecture arch = df_architecture_from_entity(process);
  U64 dasm_base_vaddr = AlignDownPow2(dv->base_vaddr, KB(64));
  DF_Entity *dasm_module = df_module_from_process_vaddr(process,  dasm_base_vaddr);
  DF_Entity *dasm_binary = df_binary_file_from_module(dasm_module);
  Rng1U64 dasm_vaddr_range = r1u64(dasm_base_vaddr, dasm_base_vaddr+KB(64));
  U128 dasm_key = ctrl_hash_store_key_from_process_vaddr_range(process->ctrl_machine_id, process->ctrl_handle, dasm_vaddr_range, 0);
  U128 dasm_data_hash = {0};
  DASM_Params dasm_params = {0};
  {
    dasm_params.vaddr = dasm_vaddr_range.min;
    dasm_params.arch = arch;
    dasm_params.style_flags = dv->style_flags;
    dasm_params.syntax = DASM_Syntax_Intel;
    dasm_params.base_vaddr = dasm_module->vaddr_rng.min;
    dasm_params.exe_path = df_full_path_from_entity(scratch.arena, dasm_binary);
  }
  DASM_Info dasm_info = dasm_info_from_key_params(dasm_scope, dasm_key, &dasm_params, &dasm_data_hash);
  U128 dasm_text_hash = {0};
  TXT_TextInfo dasm_text_info = txt_text_info_from_key_lang(txt_scope, dasm_info.text_key, txt_lang_kind_from_architecture(arch), &dasm_text_hash);
  String8 dasm_text_data = hs_data_from_hash(hs_scope, dasm_text_hash);
  B32 has_disasm = (dasm_info.insts.count != 0 && dasm_text_info.lines_count != 0);
  B32 is_loading = (!has_disasm && !df_entity_is_nil(process) && dim_1u64(dasm_vaddr_range) != 0);
  
  //////////////////////////////
  //- rjf: unpack module info for this region
  //
  DF_Entity *module = df_module_from_process_vaddr(process, dasm_vaddr_range.min);
  DF_Entity *binary = df_binary_file_from_module(module);
  
  //////////////////////////////
  //- rjf: is loading -> equip view with loading information
  //
  if(is_loading)
  {
    df_view_equip_loading_info(view, is_loading, 0, 0);
  }
  
  //////////////////////////////
  //- rjf: determine visible line range / count
  //
  Rng1S64 visible_line_num_range = r1s64(view->scroll_pos.y.idx + (S64)(view->scroll_pos.y.off) + 1 - !!(view->scroll_pos.y.off < 0),
                                         view->scroll_pos.y.idx + (S64)(view->scroll_pos.y.off) + 1 + num_possible_visible_lines);
  U64 visible_line_count = 0;
  {
    visible_line_num_range.min = Clamp(1, visible_line_num_range.min, (S64)dasm_info.insts.count);
    visible_line_num_range.max = Clamp(1, visible_line_num_range.max, (S64)dasm_info.insts.count);
    visible_line_count = (U64)dim_1s64(visible_line_num_range)+1;
  }
  
  //////////////////////////////
  //- rjf: calculate line-range-dependent info
  //
  F32 margin_width_px = big_glyph_advance*3.5f;
  F32 line_num_width_px = big_glyph_advance * (log10(visible_line_num_range.max) + 3);
  TXT_LineTokensSlice slice = txt_line_tokens_slice_from_info_data_line_range(scratch.arena, &dasm_text_info, dasm_text_data, visible_line_num_range);
  
  //////////////////////////////
  //- rjf: calculate scroll bounds
  //
  S64 line_size_x = 0;
  Rng1S64 scroll_idx_rng[Axis2_COUNT] = {0};
  {
    line_size_x = (200*big_glyph_advance);
    line_size_x = ClampBot(line_size_x, (S64)big_glyph_advance*120);
    line_size_x = ClampBot(line_size_x, (S64)code_area_dim.x);
    scroll_idx_rng[Axis2_X] = r1s64(0, line_size_x-(S64)code_area_dim.x);
    scroll_idx_rng[Axis2_Y] = r1s64(0, (S64)dasm_info.insts.count-1);
  }
  
  //////////////////////////////
  //- rjf: get active search query
  //
  String8 search_query = {0};
  Side search_query_side = Side_Invalid;
  B32 search_query_is_active = 0;
  {
    DF_CoreCmdKind query_cmd_kind = df_core_cmd_kind_from_string(ws->query_cmd_spec->info.string);
    if(query_cmd_kind == DF_CoreCmdKind_FindTextForward ||
       query_cmd_kind == DF_CoreCmdKind_FindTextBackward)
    {
      search_query = str8(ws->query_view_stack_top->query_buffer, ws->query_view_stack_top->query_string_size);
      search_query_is_active = 1;
      search_query_side = (query_cmd_kind == DF_CoreCmdKind_FindTextForward) ? Side_Max : Side_Min;
    }
  }
  
  //////////////////////////////
  //- rjf: prepare code slice info bundle, for the viewable region of text
  //
  DF_CodeSliceParams code_slice_params = {0};
  if(has_disasm)
  {
    // rjf: fill basics
    code_slice_params.flags                     = DF_CodeSliceFlag_Margin|DF_CodeSliceFlag_LineNums|DF_CodeSliceFlag_Clickable;
    code_slice_params.line_num_range            = visible_line_num_range;
    code_slice_params.line_text                 = push_array(scratch.arena, String8, visible_line_count);
    code_slice_params.line_ranges               = push_array(scratch.arena, Rng1U64, visible_line_count);
    code_slice_params.line_tokens               = push_array(scratch.arena, TXT_TokenArray, visible_line_count);
    code_slice_params.line_bps                  = push_array(scratch.arena, DF_EntityList, visible_line_count);
    code_slice_params.line_ips                  = push_array(scratch.arena, DF_EntityList, visible_line_count);
    code_slice_params.line_pins                 = push_array(scratch.arena, DF_EntityList, visible_line_count);
    code_slice_params.line_dasm2src             = push_array(scratch.arena, DF_TextLineDasm2SrcInfoList, visible_line_count);
    code_slice_params.line_src2dasm             = push_array(scratch.arena, DF_TextLineSrc2DasmInfoList, visible_line_count);
    code_slice_params.font                      = code_font;
    code_slice_params.font_size                 = code_font_size;
    code_slice_params.line_height_px            = code_line_height;
    code_slice_params.search_query              = search_query;
    code_slice_params.margin_width_px           = margin_width_px;
    code_slice_params.line_num_width_px         = line_num_width_px;
    code_slice_params.line_text_max_width_px    = (F32)line_size_x;
    code_slice_params.flash_ranges              = df_push_entity_child_list_with_kind(scratch.arena, process, DF_EntityKind_FlashMarker);
    code_slice_params.margin_float_off_px       = view->scroll_pos.x.idx + view->scroll_pos.x.off;
    if(code_slice_params.margin_float_off_px < 1)
    {
      code_slice_params.margin_float_off_px = 0;
    }
    df_entity_list_push(scratch.arena, &code_slice_params.relevant_binaries, binary);
    
    // rjf: fill text info
    {
      S64 line_num = visible_line_num_range.min;
      U64 line_idx = visible_line_num_range.min-1;
      for(U64 visible_line_idx = 0; visible_line_idx < visible_line_count; visible_line_idx += 1, line_idx += 1, line_num += 1)
      {
        code_slice_params.line_text[visible_line_idx]   = str8_substr(dasm_text_data, dasm_text_info.lines_ranges[line_idx]);
        code_slice_params.line_ranges[visible_line_idx] = dasm_text_info.lines_ranges[line_idx];
        code_slice_params.line_tokens[visible_line_idx] = slice.line_tokens[visible_line_idx];
      }
    }
    
    // rjf: find live threads mapping to this disassembly
    ProfScope("find live threads mapping to this disassembly")
    {
      DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx.thread);
      DF_EntityList threads = df_query_cached_entity_list_with_kind(DF_EntityKind_Thread);
      for(DF_EntityNode *thread_n = threads.first; thread_n != 0; thread_n = thread_n->next)
      {
        DF_Entity *thread = thread_n->entity;
        U64 unwind_count = (thread == selected_thread) ? ctrl_ctx.unwind_count : 0;
        U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
        if(df_entity_ancestor_from_kind(thread, DF_EntityKind_Process) == process && contains_1u64(dasm_vaddr_range, rip_vaddr))
        {
          U64 rip_off = rip_vaddr - dasm_vaddr_range.min;
          S64 line_num = dasm_inst_array_idx_from_code_off__linear_scan(&dasm_info.insts, rip_off)+1;
          if(contains_1s64(visible_line_num_range, line_num))
          {
            U64 slice_line_idx = (line_num-visible_line_num_range.min);
            df_entity_list_push(scratch.arena, &code_slice_params.line_ips[slice_line_idx], thread);
          }
        }
      }
    }
    
    // rjf: find breakpoints mapping to this disassembly
    ProfScope("find breakpoints mapping to this disassembly")
    {
      DF_EntityList bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
      for(DF_EntityNode *n = bps.first; n != 0; n = n->next)
      {
        DF_Entity *bp = n->entity;
        if(bp->flags & DF_EntityFlag_HasVAddr && contains_1u64(dasm_vaddr_range, bp->vaddr))
        {
          U64 off = bp->vaddr-dasm_vaddr_range.min;
          U64 idx = dasm_inst_array_idx_from_code_off__linear_scan(&dasm_info.insts, off);
          S64 line_num = (S64)(idx+1);
          if(contains_1s64(visible_line_num_range, line_num))
          {
            U64 slice_line_idx = (line_num-visible_line_num_range.min);
            df_entity_list_push(scratch.arena, &code_slice_params.line_bps[slice_line_idx], bp);
          }
        }
      }
    }
    
    // rjf: find watch pins mapping to this disassembly
    ProfScope("find watch pins mapping to this disassembly")
    {
      DF_EntityList pins = df_query_cached_entity_list_with_kind(DF_EntityKind_WatchPin);
      for(DF_EntityNode *n = pins.first; n != 0; n = n->next)
      {
        DF_Entity *pin = n->entity;
        if(pin->flags & DF_EntityFlag_HasVAddr && contains_1u64(dasm_vaddr_range, pin->vaddr))
        {
          U64 off = pin->vaddr-dasm_vaddr_range.min;
          U64 idx = dasm_inst_array_idx_from_code_off__linear_scan(&dasm_info.insts, off);
          S64 line_num = (S64)(idx+1);
          if(contains_1s64(visible_line_num_range, line_num))
          {
            U64 slice_line_idx = (line_num-visible_line_num_range.min);
            df_entity_list_push(scratch.arena, &code_slice_params.line_pins[slice_line_idx], pin);
          }
        }
      }
    }
    
    // rjf: fill dasm -> src info
    {
      DF_Entity *module = df_module_from_process_vaddr(process, dasm_vaddr_range.min);
      DF_Entity *binary = df_binary_file_from_module(module);
      for(S64 line_num = visible_line_num_range.min; line_num < visible_line_num_range.max; line_num += 1)
      {
        U64 vaddr = dasm_vaddr_range.min + dasm_inst_array_code_off_from_idx(&dasm_info.insts, line_num-1);
        U64 voff = df_voff_from_vaddr(module, vaddr);
        U64 slice_idx = line_num-visible_line_num_range.min;
        DF_TextLineDasm2SrcInfoNode *dasm2src_n = push_array(scratch.arena, DF_TextLineDasm2SrcInfoNode, 1);
        SLLQueuePush(code_slice_params.line_dasm2src[slice_idx].first, code_slice_params.line_dasm2src[slice_idx].last, dasm2src_n);
        code_slice_params.line_dasm2src[slice_idx].count += 1;
        dasm2src_n->v = df_text_line_dasm2src_info_from_binary_voff(binary, voff);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: do keyboard interaction
  //
  B32 snap[Axis2_COUNT] = {0};
  UI_Focus(UI_FocusKind_On)
  {
    if(ui_is_focus_active() && !is_loading && visible_line_num_range.max >= visible_line_num_range.min)
    {
      snap[Axis2_X] = snap[Axis2_Y] = df_do_txt_controls(&dasm_text_info, dasm_text_data, ClampBot(visible_line_count, 10) - 10, &dv->cursor, &dv->mark, &dv->preferred_column);
    }
  }
  
  //////////////////////////////
  //- rjf: do goto vaddr
  //
  if(!is_loading && dv->goto_vaddr != 0)
  {
    U64 vaddr = dv->goto_vaddr;
    dv->goto_vaddr = 0;
    U64 line_idx = dasm_inst_array_idx_from_code_off__linear_scan(&dasm_info.insts, vaddr-dasm_vaddr_range.min);
    S64 line_num = (S64)(line_idx+1);
    dv->cursor = dv->mark = txt_pt(line_num, 1);
    dv->center_cursor = !dv->contain_cursor || (line_num < visible_line_num_range.min+8 || visible_line_num_range.max-8 < line_num);
  }
  
  //////////////////////////////
  //- rjf: build container
  //
  UI_Box *container_box = &ui_g_nil_box;
  if(has_disasm)
  {
    ui_set_next_pref_width(ui_px(code_area_dim.x, 1));
    ui_set_next_pref_height(ui_px(code_area_dim.y, 1));
    ui_set_next_child_layout_axis(Axis2_Y);
    container_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|
                                              UI_BoxFlag_Scroll|
                                              UI_BoxFlag_DrawBorder|
                                              UI_BoxFlag_AllowOverflowX|
                                              UI_BoxFlag_AllowOverflowY,
                                              "###code_area_%p", view);
  }
  
  //////////////////////////////
  //- rjf: build container contents
  //
  if(has_disasm) UI_Parent(container_box)
  {
    //- rjf: build fractional space
    container_box->view_off.x = container_box->view_off_target.x = view->scroll_pos.x.idx + view->scroll_pos.x.off;
    container_box->view_off.y = container_box->view_off_target.y = code_line_height*mod_f32(view->scroll_pos.y.off, 1.f) + code_line_height*(view->scroll_pos.y.off < 0) - code_line_height*(view->scroll_pos.y.off == -1.f && view->scroll_pos.y.idx == 1);
    
    //- rjf: build code slice
    DF_CodeSliceSignal sig = {0};
    UI_Focus(UI_FocusKind_On)
    {
      sig = df_code_slicef(ws, &ctrl_ctx, &parse_ctx, &code_slice_params, &dv->cursor, &dv->mark, &dv->preferred_column, "dasm_slice_%p", view);
    }
    
    //- rjf: hover eval
    if(!ui_dragging(sig.base) && sig.mouse_expr_rng.min.line != 0 && contains_1s64(visible_line_num_range, sig.mouse_expr_rng.min.line) && sig.mouse_expr_rng.max.line == sig.mouse_expr_rng.min.line && sig.base.event_flags == 0)
    {
      U64 line_idx = sig.mouse_expr_rng.min.line-visible_line_num_range.min;
      String8 expr = str8_substr(code_slice_params.line_text[line_idx], r1u64(sig.mouse_expr_rng.min.column-1, sig.mouse_expr_rng.max.column-1));
      if(expr.size != 0)
      {
        DF_Eval eval = df_eval_from_string(scratch.arena, dbgi_scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, expr);
        if(eval.mode != EVAL_EvalMode_NULL)
        {
          U64 off = dasm_inst_array_code_off_from_idx(&dasm_info.insts, sig.mouse_expr_rng.min.line-1);
          U64 vaddr = dasm_vaddr_range.min+off;
          df_set_hover_eval(ws, sig.mouse_expr_baseline_pos, ctrl_ctx, process, sig.mouse_pt, vaddr, expr);
        }
      }
    }
    
    //- rjf: press code slice? -> focus panel
    if(ui_pressed(sig.base))
    {
      DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
    }
    
    //- rjf: dragging? -> contain cursor
    if(ui_dragging(sig.base) && sig.base.event_flags == 0)
    {
      dv->contain_cursor = 1;
    }
    
    //- rjf: clicked margin? -> place breakpoint
    if(sig.clicked_margin_line_num != 0)
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.vaddr = dasm_vaddr_range.min+dasm_inst_array_code_off_from_idx(&dasm_info.insts, sig.clicked_margin_line_num-1);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddressBreakpoint));
    }
    
    //- rjf: dropped entity onto line? -> do drop
    if(sig.dropped_entity_line_num != 0 && !df_entity_is_nil(sig.dropped_entity))
    {
      U64 drop_vaddr = dasm_vaddr_range.min+dasm_inst_array_code_off_from_idx(&dasm_info.insts, sig.dropped_entity_line_num-1);
      DF_Entity *dropped_entity = sig.dropped_entity;
      switch(dropped_entity->kind)
      {
        default:{}break;
        case DF_EntityKind_Breakpoint:
        case DF_EntityKind_WatchPin:
        {
          DF_StateDeltaHistory *hist = df_state_delta_history();
          df_state_delta_history_push_batch(hist, &dropped_entity->generation);
          df_state_delta_history_push_struct_delta(hist, &dropped_entity->vaddr);
          df_entity_change_parent(hist, dropped_entity, dropped_entity->parent, df_entity_root());
          df_entity_equip_vaddr(dropped_entity, drop_vaddr);
        }break;
        case DF_EntityKind_Thread:
        if(contains_1s64(visible_line_num_range, sig.dropped_entity_line_num))
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(dropped_entity);
          params.vaddr = drop_vaddr;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetThreadIP));
        }break;
      }
    }
    
    //- rjf: copy text
    if(!txt_pt_match(sig.copy_range.min, sig.copy_range.max))
    {
      Temp temp = temp_begin(scratch.arena);
      String8 text = txt_string_from_info_data_txt_rng(&dasm_text_info, dasm_text_data, sig.copy_range);
      os_set_clipboard_text(text);
      temp_end(temp);
    }
    
    //- rjf: toggle cursor watch
    if(sig.toggle_cursor_watch)
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchExpressionAtCursor));
    }
    
    //- rjf: set next statement
    if(sig.set_next_statement_line_num != 0 && contains_1s64(visible_line_num_range, sig.set_next_statement_line_num))
    {
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.text_point = txt_pt(sig.set_next_statement_line_num, 1);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetNextStatement));
    }
    
    //- rjf: run-to-line
    if(sig.run_to_line_num != 0 && contains_1s64(visible_line_num_range, sig.run_to_line_num))
    {
      DF_CmdParams params = df_cmd_params_from_window(ws);
      params.vaddr = dasm_vaddr_range.min+dasm_inst_array_code_off_from_idx(&dasm_info.insts, sig.run_to_line_num-1);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToAddress));
    }
    
    //- rjf: go to source
    if(sig.goto_src_line_num != 0 && contains_1s64(visible_line_num_range, sig.goto_src_line_num))
    {
      U64 vaddr = dasm_vaddr_range.min+dasm_inst_array_code_off_from_idx(&dasm_info.insts, sig.goto_src_line_num-1);
      DF_Entity *module = df_module_from_process_vaddr(process, vaddr);
      DF_Entity *binary = df_binary_file_from_module(module);
      U64 voff = df_voff_from_vaddr(module, vaddr);
      DF_TextLineDasm2SrcInfo dasm2src = df_text_line_dasm2src_info_from_binary_voff(binary, voff);
      String8 file_path = df_full_path_from_entity(scratch.arena, dasm2src.file);
      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
      params.text_point = dasm2src.pt;
      params.file_path = file_path;
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
    }
  }
  
  //////////////////////////////
  //- rjf: apply post-build view snapping rules
  //
  if(!is_loading)
  {
    // rjf: contain => snap
    if(dv->contain_cursor)
    {
      dv->contain_cursor = 0;
      snap[Axis2_X] = 1;
      snap[Axis2_Y] = 1;
    }
    
    // rjf: center cursor
    if(dv->center_cursor)
    {
      dv->center_cursor = 0;
      
      // rjf: scroll x
#if 0
      {
        String8 cursor_line = txti_string_from_handle_line_num(scratch.arena, txti_handle, tv->cursor.line);
        F32 cursor_advance = f_dim_from_tag_size_string(code_font, code_font_size, str8_prefix(cursor_line, tv->cursor.column-1)).x;
        S64 new_idx = (S64)(cursor_advance - code_area_dim.x/2);
        new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
        ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
        snap[Axis2_X] = 0;
      }
#endif
      
      // rjf: scroll y
      {
        S64 new_idx = (dv->cursor.line-1) - num_possible_visible_lines/2 + 2;
        new_idx = Clamp(scroll_idx_rng[Axis2_Y].min, new_idx, scroll_idx_rng[Axis2_Y].max);
        ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
        snap[Axis2_Y] = 0;
      }
    }
    
    // rjf: snap in X
    if(snap[Axis2_X])
    {
#if 0
      String8 cursor_line = txti_string_from_handle_line_num(scratch.arena, txti_handle, tv->cursor.line);
      S64 cursor_off = (S64)(f_dim_from_tag_size_string(code_font, code_font_size, str8_prefix(cursor_line, tv->cursor.column-1)).x + margin_width_px + line_num_width_px);
      Rng1S64 visible_pixel_range =
      {
        view->scroll_pos.x.idx,
        view->scroll_pos.x.idx + (S64)code_area_dim.x,
      };
      Rng1S64 cursor_pixel_range =
      {
        cursor_off - (S64)(big_glyph_advance*4) - (S64)(margin_width_px + line_num_width_px),
        cursor_off + (S64)(big_glyph_advance*4),
      };
      S64 min_delta = Min(0, cursor_pixel_range.min - visible_pixel_range.min);
      S64 max_delta = Max(0, cursor_pixel_range.max - visible_pixel_range.max);
      S64 new_idx = view->scroll_pos.x.idx+min_delta+max_delta;
      new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
      ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
#endif
    }
    
    // rjf: snap in Y
    if(snap[Axis2_Y])
    {
      Rng1S64 cursor_visibility_range = r1s64(dv->cursor.line-4, dv->cursor.line+4);
      cursor_visibility_range.min = Clamp(0, cursor_visibility_range.min, (S64)dasm_info.insts.count);
      cursor_visibility_range.max = Clamp(0, cursor_visibility_range.max, (S64)dasm_info.insts.count);
      S64 min_delta = Min(0, cursor_visibility_range.min-visible_line_num_range.min);
      S64 max_delta = Max(0, cursor_visibility_range.max-visible_line_num_range.max);
      S64 new_idx = view->scroll_pos.y.idx+min_delta+max_delta;
      new_idx = Clamp(0, new_idx, (S64)dasm_info.insts.count-1);
      ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: build horizontal scroll bar
  //
  if(has_disasm)
  {
    ui_set_next_fixed_x(0);
    ui_set_next_fixed_y(code_area_dim.y);
    ui_set_next_fixed_width(panel_box_dim.x - scroll_bar_dim);
    ui_set_next_fixed_height(scroll_bar_dim);
    {
      view->scroll_pos.x = ui_scroll_bar(Axis2_X,
                                         ui_px(scroll_bar_dim, 1.f),
                                         view->scroll_pos.x,
                                         scroll_idx_rng[Axis2_X],
                                         (S64)code_area_dim.x);
    }
  }
  
  //////////////////////////////
  //- rjf: build vertical scroll bar
  //
  if(has_disasm)
  {
    ui_set_next_fixed_x(code_area_dim.x);
    ui_set_next_fixed_y(0);
    ui_set_next_fixed_width(scroll_bar_dim);
    ui_set_next_fixed_height(panel_box_dim.y - bottom_bar_dim.y - scroll_bar_dim);
    {
      view->scroll_pos.y = ui_scroll_bar(Axis2_Y,
                                         ui_px(scroll_bar_dim, 1.f),
                                         view->scroll_pos.y,
                                         scroll_idx_rng[Axis2_Y],
                                         num_possible_visible_lines);
    }
  }
  
  //////////////////////////////
  //- rjf: top-level container interaction (scrolling)
  //
  if(!is_loading)
  {
    UI_Signal sig = ui_signal_from_box(container_box);
    if(sig.scroll.x != 0)
    {
      S64 new_idx = view->scroll_pos.x.idx+sig.scroll.x*big_glyph_advance;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_X], new_idx);
      ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
    }
    if(sig.scroll.y != 0)
    {
      S64 new_idx = view->scroll_pos.y.idx + sig.scroll.y;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_Y], new_idx);
      ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
    }
    ui_scroll_pt_clamp_idx(&view->scroll_pos.x, scroll_idx_rng[Axis2_X]);
    ui_scroll_pt_clamp_idx(&view->scroll_pos.y, scroll_idx_rng[Axis2_Y]);
  }
  
  //////////////////////////////
  //- rjf: build bottom info bar
  //
  if(has_disasm)
  {
    ui_set_next_fixed_x(0);
    ui_set_next_fixed_y(code_area_dim.y + scroll_bar_dim);
    ui_set_next_pref_width(ui_px(bottom_bar_dim.x, 1));
    ui_set_next_pref_height(ui_px(bottom_bar_dim.y, 1));
    ui_set_next_background_color(df_rgba_from_theme_color(DF_ThemeColor_AltBackground));
    ui_set_next_flags(UI_BoxFlag_DrawBackground);
    UI_Row
      UI_TextAlignment(UI_TextAlign_Center)
      UI_PrefWidth(ui_text_dim(10, 1))
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      UI_Font(code_font)
    {
      DF_Entity *module = df_module_from_process_vaddr(process, dasm_vaddr_range.min);
      U64 cursor_vaddr = (1 <= dv->cursor.line && dv->cursor.line <= dasm_info.insts.count) ? (dasm_vaddr_range.min+dasm_info.insts.v[dv->cursor.line-1].code_off) : 0;
      ui_labelf("%S", path_normalized_from_string(scratch.arena, module->name));
      ui_spacer(ui_em(1.5f, 1));
      ui_labelf("Address: 0x%I64x, Row: %I64d, Col: %I64d", cursor_vaddr, dv->cursor.line, dv->cursor.column);
      ui_spacer(ui_pct(1, 0));
      ui_labelf("(read only)");
      ui_labelf("bin");
    }
  }
  
  dbgi_scope_close(dbgi_scope);
  txt_scope_close(txt_scope);
  dasm_scope_close(dasm_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Watch @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Watch)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_init(ewv, view, DF_WatchViewFillKind_Mutable);
  
  // rjf: add roots for watches
  {
    Temp scratch = scratch_begin(0, 0);
    DF_EvalViewKey eval_view_key = df_eval_view_key_from_eval_watch_view(ewv);
    DF_EvalView *eval_view = df_eval_view_from_key(eval_view_key);
    for(DF_CfgNode *expr = cfg_root->first; expr != &df_g_nil_cfg_node; expr = expr->next)
    {
      if(expr->flags & DF_CfgNodeFlag_StringLiteral)
      {
        DF_EvalRoot *root = df_eval_root_alloc(view, ewv);
        DF_ExpandKey key = df_expand_key_from_eval_root(root);
        String8 expr_raw = df_cfg_raw_from_escaped_string(scratch.arena, expr->string);
        df_eval_root_equip_string(root, expr_raw);
        if(expr->first != &df_g_nil_cfg_node)
        {
          String8 view_rule_raw = df_cfg_raw_from_escaped_string(scratch.arena, expr->first->string);
          df_eval_view_set_key_rule(eval_view, key, view_rule_raw);
        }
      }
    }
    scratch_end(scratch);
  }
  
  ProfEnd();
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Watch)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strs = {0};
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  DF_EvalViewKey eval_view_key = df_eval_view_key_from_eval_watch_view(ewv);
  DF_EvalView *eval_view = df_eval_view_from_key(eval_view_key);
  {
    for(DF_EvalRoot *root = ewv->first_root; root != 0; root = root->next)
    {
      DF_ExpandKey key = df_expand_key_from_eval_root(root);
      String8 string = df_string_from_eval_root(root);
      String8 string_escaped = df_cfg_escaped_from_raw_string(scratch.arena, string);
      str8_list_pushf(arena, &strs, "\"%S\"", string_escaped);
      String8 view_rule = df_eval_view_rule_from_key(eval_view, key);
      String8 view_rule_escaped = df_cfg_escaped_from_raw_string(scratch.arena, view_rule);
      if(view_rule_escaped.size != 0)
      {
        str8_list_pushf(arena, &strs, ":{\"%S\"}", view_rule_escaped);
      }
      if(root->next != 0)
      {
        str8_list_pushf(arena, &strs, " ");
      }
    }
  }
  String8 string = str8_list_join(arena, &strs, 0);
  scratch_end(scratch);
  return string;
}

DF_VIEW_CMD_FUNCTION_DEF(Watch)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_cmds(ws, panel, view, ewv, cmds);
  ProfEnd();
}

DF_VIEW_UI_FUNCTION_DEF(Watch)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_build(ws, panel, view, ewv, 1*(view->query_string_size == 0), 10, rect);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Locals @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Locals) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Locals) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Locals) {}
DF_VIEW_UI_FUNCTION_DEF(Locals)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_init(ewv, view, DF_WatchViewFillKind_Locals);
  df_watch_view_build(ws, panel, view, ewv, 0, 10, rect);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Registers @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Registers) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Registers) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Registers) {}
DF_VIEW_UI_FUNCTION_DEF(Registers)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_init(ewv, view, DF_WatchViewFillKind_Registers);
  df_watch_view_build(ws, panel, view, ewv, 0, 16, rect);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Globals @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Globals) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Globals) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Globals) {}
DF_VIEW_UI_FUNCTION_DEF(Globals)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_init(ewv, view, DF_WatchViewFillKind_Globals);
  df_watch_view_build(ws, panel, view, ewv, 0, 10, rect);
  ProfEnd();
}

////////////////////////////////
//~ rjf: ThreadLocals @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(ThreadLocals) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(ThreadLocals) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(ThreadLocals) {}
DF_VIEW_UI_FUNCTION_DEF(ThreadLocals)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_init(ewv, view, DF_WatchViewFillKind_ThreadLocals);
  df_watch_view_build(ws, panel, view, ewv, 0, 10, rect);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Types @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Types) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Types) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Types) {}
DF_VIEW_UI_FUNCTION_DEF(Types)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_init(ewv, view, DF_WatchViewFillKind_Types);
  df_watch_view_build(ws, panel, view, ewv, 0, 10, rect);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Procedures @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Procedures) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Procedures) { return str8_lit(""); }
DF_VIEW_CMD_FUNCTION_DEF(Procedures) {}
DF_VIEW_UI_FUNCTION_DEF(Procedures)
{
  ProfBeginFunction();
  DF_WatchViewState *ewv = df_view_user_state(view, DF_WatchViewState);
  df_watch_view_init(ewv, view, DF_WatchViewFillKind_Procedures);
  df_watch_view_build(ws, panel, view, ewv, 0, 10, rect);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Output @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Output)
{
  // rjf: set up state
  DF_CodeViewState *tv = df_view_user_state(view, DF_CodeViewState);
  if(tv->initialized == 0)
  {
    tv->initialized = 1;
    tv->cursor = tv->mark = txt_pt(1, 1);
    tv->preferred_column = 1;
    tv->find_text_arena = df_view_push_arena_ext(view);
  }
  
  // rjf: default to loading
  df_view_equip_loading_info(view, 1, 0, 0);
  view->loading_t = view->loading_t_target = 1.f;
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Output)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(Output)
{
  DF_CodeViewState *tv = df_view_user_state(view, DF_CodeViewState);
  DF_Entity *entity = df_entity_from_handle(view->entity);
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    // rjf: process
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default: break;
      case DF_CoreCmdKind_GoToLine:
      {
        tv->goto_line_num = cmd->params.text_point.line;
      }break;
      case DF_CoreCmdKind_CenterCursor:
      {
        tv->center_cursor = 1;
      }break;
      case DF_CoreCmdKind_ContainCursor:
      {
        tv->contain_cursor = 1;
      }break;
      case DF_CoreCmdKind_FindTextForward:
      {
        arena_clear(tv->find_text_arena);
        tv->find_text_fwd = push_str8_copy(tv->find_text_arena, cmd->params.string);
      }break;
      case DF_CoreCmdKind_FindTextBackward:
      {
        arena_clear(tv->find_text_arena);
        tv->find_text_bwd = push_str8_copy(tv->find_text_arena, cmd->params.string);
      }break;
      case DF_CoreCmdKind_ToggleBreakpointAtCursor:
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.entity = view->entity;
        params.text_point = tv->cursor;
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_TextBreakpoint));
        tv->contain_cursor = 1;
      }break;
      case DF_CoreCmdKind_ToggleWatchPinAtCursor:
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.entity = view->entity;
        params.text_point = tv->cursor;
        params.string = cmd->params.string;
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchPin));
        tv->contain_cursor = 1;
      }break;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(Output)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  DF_CodeViewState *tv = df_view_user_state(view, DF_CodeViewState);
  
  //////////////////////////////
  //- rjf: extract invariants
  //
  DF_Entity *entity = df_log_from_entity(df_entity_root());
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  F_Tag code_font = df_font_from_slot(DF_FontSlot_Code);
  F32 code_font_size = df_font_size_from_slot(ws, DF_FontSlot_Code);
  F_Metrics code_font_metrics = f_metrics_from_tag_size(code_font, code_font_size);
  F32 code_line_height = ceil_f32(f_line_height_from_metrics(&code_font_metrics) * 1.5f);
  F32 big_glyph_advance = f_dim_from_tag_size_string(code_font, code_font_size, str8_lit("H")).x;
  Vec2F32 panel_box_dim = dim_2f32(rect);
  Vec2F32 bottom_bar_dim = {panel_box_dim.x, ui_top_font_size()*1.8f};
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  Vec2F32 code_area_dim = v2f32(panel_box_dim.x - scroll_bar_dim, panel_box_dim.y - scroll_bar_dim - bottom_bar_dim.y);
  S64 num_possible_visible_lines = (S64)(code_area_dim.y/code_line_height)+1;
  
  //////////////////////////////
  //- rjf: unpack ctrl ctx & make parse ctx
  //
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  U64 unwind_count = ctrl_ctx.unwind_count;
  U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(scope, process, rip_vaddr);
  
  //////////////////////////////
  //- rjf: unpack entity info
  //
  TXTI_Handle txti_handle = df_txti_handle_from_entity(entity);
  TXTI_BufferInfo txti_buffer_info = txti_buffer_info_from_handle(scratch.arena, txti_handle);
  B32 txti_buffer_is_ready = ((txti_buffer_info.timestamp != 0 || entity->flags & DF_EntityFlag_Output) &&
                              (txti_buffer_info.bytes_processed == txti_buffer_info.bytes_to_process || txti_buffer_info.buffer_apply_gen != 0));
  
  //////////////////////////////
  //- rjf: buffer is pending -> equip view with loading information
  //
  if(!txti_buffer_is_ready)
  {
    df_view_equip_loading_info(view, 1, txti_buffer_info.bytes_processed, txti_buffer_info.bytes_to_process);
  }
  
  //////////////////////////////
  //- rjf: determine visible line range / count
  //
  Rng1S64 visible_line_num_range = r1s64(view->scroll_pos.y.idx + (S64)(view->scroll_pos.y.off) + 1 - !!(view->scroll_pos.y.off < 0),
                                         view->scroll_pos.y.idx + (S64)(view->scroll_pos.y.off) + 1 + num_possible_visible_lines);
  Rng1S64 target_visible_line_num_range = r1s64(view->scroll_pos.y.idx + 1,
                                                view->scroll_pos.y.idx + 1 + num_possible_visible_lines);
  U64 visible_line_count = 0;
  {
    visible_line_num_range.min = Clamp(1, visible_line_num_range.min, (S64)txti_buffer_info.total_line_count);
    visible_line_num_range.max = Clamp(1, visible_line_num_range.max, (S64)txti_buffer_info.total_line_count);
    visible_line_num_range.min = Max(1, visible_line_num_range.min);
    visible_line_num_range.max = Max(1, visible_line_num_range.max);
    target_visible_line_num_range.min = Clamp(1, target_visible_line_num_range.min, (S64)txti_buffer_info.total_line_count);
    target_visible_line_num_range.max = Clamp(1, target_visible_line_num_range.max, (S64)txti_buffer_info.total_line_count);
    target_visible_line_num_range.min = Max(1, target_visible_line_num_range.min);
    target_visible_line_num_range.max = Max(1, target_visible_line_num_range.max);
    visible_line_count = (U64)dim_1s64(visible_line_num_range)+1;
  }
  
  //////////////////////////////
  //- rjf: calculate scroll bounds
  //
  S64 line_size_x = 0;
  Rng1S64 scroll_idx_rng[Axis2_COUNT] = {0};
  {
    line_size_x = (txti_buffer_info.max_line_size*big_glyph_advance*3)/2;
    line_size_x = ClampBot(line_size_x, (S64)big_glyph_advance*120);
    line_size_x = ClampBot(line_size_x, (S64)code_area_dim.x);
    scroll_idx_rng[Axis2_X] = r1s64(0, line_size_x-(S64)code_area_dim.x);
    scroll_idx_rng[Axis2_Y] = r1s64(0, (S64)txti_buffer_info.total_line_count-1);
  }
  
  //////////////////////////////
  //- rjf: calculate line-range-dependent info
  //
  F32 margin_width_px = big_glyph_advance*3.5f;
  F32 line_num_width_px = big_glyph_advance * (log10(visible_line_num_range.max) + 3);
  TXTI_Slice slice = txti_slice_from_handle_line_range(scratch.arena, txti_handle, visible_line_num_range);
  
  //////////////////////////////
  //- rjf: get active search query
  //
  String8 search_query = {0};
  Side search_query_side = Side_Invalid;
  B32 search_query_is_active = 0;
  {
    DF_CoreCmdKind query_cmd_kind = df_core_cmd_kind_from_string(ws->query_cmd_spec->info.string);
    if(query_cmd_kind == DF_CoreCmdKind_FindTextForward ||
       query_cmd_kind == DF_CoreCmdKind_FindTextBackward)
    {
      search_query = str8(ws->query_view_stack_top->query_buffer, ws->query_view_stack_top->query_string_size);
      search_query_is_active = 1;
      search_query_side = (query_cmd_kind == DF_CoreCmdKind_FindTextForward) ? Side_Max : Side_Min;
    }
  }
  
  //////////////////////////////
  //- rjf: prepare code slice info bundle, for the viewable region of text
  //
  DF_CodeSliceParams code_slice_params = {0};
  if(txti_buffer_is_ready)
  {
    // rjf: fill basics
    code_slice_params.flags                     = DF_CodeSliceFlag_LineNums|DF_CodeSliceFlag_Clickable;
    code_slice_params.line_num_range            = visible_line_num_range;
    code_slice_params.line_text                 = slice.line_text;
    code_slice_params.line_ranges               = slice.line_ranges;
    code_slice_params.line_tokens               = slice.line_tokens;
    code_slice_params.line_bps                  = push_array(scratch.arena, DF_EntityList, slice.line_count);
    code_slice_params.line_ips                  = push_array(scratch.arena, DF_EntityList, slice.line_count);
    code_slice_params.line_pins                 = push_array(scratch.arena, DF_EntityList, slice.line_count);
    code_slice_params.line_dasm2src             = push_array(scratch.arena, DF_TextLineDasm2SrcInfoList, slice.line_count);
    code_slice_params.line_src2dasm             = push_array(scratch.arena, DF_TextLineSrc2DasmInfoList, slice.line_count);
    code_slice_params.font                      = code_font;
    code_slice_params.font_size                 = code_font_size;
    code_slice_params.line_height_px            = code_line_height;
    code_slice_params.search_query              = search_query;
    code_slice_params.margin_width_px           = margin_width_px;
    code_slice_params.line_num_width_px         = line_num_width_px;
    code_slice_params.line_text_max_width_px    = (F32)line_size_x;
    code_slice_params.flash_ranges              = df_push_entity_child_list_with_kind(scratch.arena, entity, DF_EntityKind_FlashMarker);
    code_slice_params.margin_float_off_px       = view->scroll_pos.x.idx + view->scroll_pos.x.off;
    if(code_slice_params.margin_float_off_px < 1)
    {
      code_slice_params.margin_float_off_px = 0;
    }
  }
  
  //////////////////////////////
  //- rjf: build container
  //
  UI_Box *container_box = &ui_g_nil_box;
  if(txti_buffer_is_ready)
  {
    ui_set_next_pref_width(ui_px(code_area_dim.x, 1));
    ui_set_next_pref_height(ui_px(code_area_dim.y, 1));
    ui_set_next_child_layout_axis(Axis2_Y);
    container_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|
                                              UI_BoxFlag_Scroll|
                                              UI_BoxFlag_DrawBorder|
                                              UI_BoxFlag_AllowOverflowX|
                                              UI_BoxFlag_AllowOverflowY,
                                              "###code_area_%p", view);
  }
  
  //////////////////////////////
  //- rjf: cancelled search query -> center cursor
  //
  if(!search_query_is_active && tv->drifted_for_search)
  {
    tv->drifted_for_search = 0;
    tv->center_cursor = 1;
  }
  
  //////////////////////////////
  //- rjf: do searching operations
  //
  if(txti_buffer_is_ready)
  {
    //- rjf: find text (forward)
    if(tv->find_text_fwd.size != 0)
    {
      Temp scratch = scratch_begin(0, 0);
      B32 found = 0;
      B32 first = 1;
      S64 line_num_start = tv->cursor.line;
      S64 line_num_last = (S64)txti_buffer_info.total_line_count;
      for(S64 line_num = line_num_start;; first = 0)
      {
        // rjf: pop scratch
        temp_end(scratch);
        
        // rjf: gather line info
        String8 line_string = txti_string_from_handle_line_num(scratch.arena, txti_handle, line_num);
        U64 search_start = 0;
        if(tv->cursor.line == line_num && first)
        {
          search_start = tv->cursor.column;
        }
        
        // rjf: search string
        U64 needle_pos = str8_find_needle(line_string, search_start, tv->find_text_fwd, StringMatchFlag_CaseInsensitive);
        if(needle_pos < line_string.size)
        {
          tv->cursor.line = line_num;
          tv->cursor.column = needle_pos+1;
          tv->mark = tv->cursor;
          found = 1;
          break;
        }
        
        // rjf: break if circled back around to cursor
        else if(line_num == line_num_start && !first)
        {
          break;
        }
        
        // rjf: increment
        line_num += 1;
        if(line_num > line_num_last)
        {
          line_num = 1;
        }
      }
      tv->center_cursor = found;
      if(found == 0)
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.string = push_str8f(scratch.arena, "Could not find \"%S\"", tv->find_text_fwd);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
      }
      scratch_end(scratch);
    }
    
    //- rjf: find text (backward)
    if(tv->find_text_bwd.size != 0)
    {
      Temp scratch = scratch_begin(0, 0);
      B32 found = 0;
      B32 first = 1;
      S64 line_num_start = tv->cursor.line;
      S64 line_num_last = (S64)txti_buffer_info.total_line_count;
      for(S64 line_num = line_num_start;; first = 0)
      {
        // rjf: pop scratch
        temp_end(scratch);
        
        // rjf: gather line info
        String8 line_string = txti_string_from_handle_line_num(scratch.arena, txti_handle, line_num);
        if(tv->cursor.line == line_num && first)
        {
          line_string = str8_prefix(line_string, tv->cursor.column-1);
        }
        
        // rjf: search string
        U64 next_needle_pos = line_string.size;
        for(U64 needle_pos = 0; needle_pos < line_string.size;)
        {
          needle_pos = str8_find_needle(line_string, needle_pos, tv->find_text_bwd, StringMatchFlag_CaseInsensitive);
          if(needle_pos < line_string.size)
          {
            next_needle_pos = needle_pos;
            needle_pos += 1;
          }
        }
        if(next_needle_pos < line_string.size)
        {
          tv->cursor.line = line_num;
          tv->cursor.column = next_needle_pos+1;
          tv->mark = tv->cursor;
          found = 1;
          break;
        }
        
        // rjf: break if circled back around to cursor line
        else if(line_num == line_num_start && !first)
        {
          break;
        }
        
        // rjf: increment
        line_num -= 1;
        if(line_num == 0)
        {
          line_num = line_num_last;
        }
      }
      tv->center_cursor = found;
      if(found == 0)
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.string = push_str8f(scratch.arena, "Could not find \"%S\"", tv->find_text_bwd);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
      }
      scratch_end(scratch);
    }
    
    MemoryZeroStruct(&tv->find_text_fwd);
    MemoryZeroStruct(&tv->find_text_bwd);
    arena_clear(tv->find_text_arena);
  }
  
  //////////////////////////////
  //- rjf: do goto line
  //
  if(txti_buffer_is_ready) if(tv->goto_line_num != 0)
  {
    S64 line_num = tv->goto_line_num;
    tv->goto_line_num = 0;
    line_num = Clamp(1, line_num, txti_buffer_info.total_line_count);
    tv->cursor = tv->mark = txt_pt(line_num, 1);
    tv->center_cursor = !tv->contain_cursor || (line_num < target_visible_line_num_range.min+4 || target_visible_line_num_range.max-4 < line_num);
  }
  
  //////////////////////////////
  //- rjf: do keyboard interaction
  //
  B32 snap[Axis2_COUNT] = {0};
  UI_Focus(UI_FocusKind_On)
  {
    if(txti_buffer_is_ready && visible_line_num_range.max >= visible_line_num_range.min && ui_is_focus_active())
    {
      snap[Axis2_X] = snap[Axis2_Y] = df_do_txti_controls(txti_handle, ClampBot(num_possible_visible_lines, 10) - 10, &tv->cursor, &tv->mark, &tv->preferred_column);
    }
  }
  
  //////////////////////////////
  //- rjf: build container contents
  //
  if(txti_buffer_is_ready) UI_Parent(container_box)
  {
    //- rjf: build fractional space
    container_box->view_off.x = container_box->view_off_target.x = view->scroll_pos.x.idx + view->scroll_pos.x.off;
    container_box->view_off.y = container_box->view_off_target.y = code_line_height*mod_f32(view->scroll_pos.y.off, 1.f) + code_line_height*(view->scroll_pos.y.off < 0) - code_line_height*(view->scroll_pos.y.off == -1.f && view->scroll_pos.y.idx == 1);
    
    //- rjf: build code slice
    DF_CodeSliceSignal sig = {0};
    UI_Focus(UI_FocusKind_On)
    {
      sig = df_code_slicef(ws, &ctrl_ctx, &parse_ctx, &code_slice_params, &tv->cursor, &tv->mark, &tv->preferred_column, "txt_view_%p", view);
    }
    
    //- rjf: hover eval
    if(!ui_dragging(sig.base) && sig.mouse_expr_rng.min.line != 0)
    {
      TxtRng expr_rng = sig.mouse_expr_rng;
      String8 expr = txti_string_from_handle_txt_rng(scratch.arena, txti_handle, expr_rng);
      if(expr.size != 0)
      {
        DF_Eval eval = df_eval_from_string(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, expr);
        if(eval.mode != EVAL_EvalMode_NULL)
        {
          df_set_hover_eval(ws, sig.mouse_expr_baseline_pos, ctrl_ctx, entity, sig.mouse_pt, 0, expr);
        }
      }
    }
    
    //- rjf: press code slice? -> focus panel
    if(ui_pressed(sig.base))
    {
      DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
    }
    
    //- rjf: dragging? -> contain cursor
    if(ui_dragging(sig.base))
    {
      tv->contain_cursor = 1;
    }
    
    //- rjf: copy text
    if(!txt_pt_match(sig.copy_range.min, sig.copy_range.max))
    {
      Temp temp = temp_begin(scratch.arena);
      String8 text = txti_string_from_handle_txt_rng(temp.arena, txti_handle, sig.copy_range);
      os_set_clipboard_text(text);
      temp_end(temp);
    }
  }
  
  //////////////////////////////
  //- rjf: apply post-build view snapping rules
  //
  if(txti_buffer_is_ready)
  {
    // rjf: contain => snap
    if(tv->contain_cursor)
    {
      tv->contain_cursor = 0;
      snap[Axis2_X] = 1;
      snap[Axis2_Y] = 1;
    }
    
    // rjf: center cursor
    if(tv->center_cursor)
    {
      tv->center_cursor = 0;
      String8 cursor_line = txti_string_from_handle_line_num(scratch.arena, txti_handle, tv->cursor.line);
      F32 cursor_advance = f_dim_from_tag_size_string(code_font, code_font_size, str8_prefix(cursor_line, tv->cursor.column-1)).x;
      
      // rjf: scroll x
      {
        S64 new_idx = (S64)(cursor_advance - code_area_dim.x/2);
        new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
        ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
        snap[Axis2_X] = 0;
      }
      
      // rjf: scroll y
      {
        S64 new_idx = (tv->cursor.line-1) - num_possible_visible_lines/2 + 2;
        new_idx = Clamp(scroll_idx_rng[Axis2_Y].min, new_idx, scroll_idx_rng[Axis2_Y].max);
        ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
        snap[Axis2_Y] = 0;
      }
    }
    
    // rjf: snap in X
    if(snap[Axis2_X])
    {
      String8 cursor_line = txti_string_from_handle_line_num(scratch.arena, txti_handle, tv->cursor.line);
      S64 cursor_off = (S64)(f_dim_from_tag_size_string(code_font, code_font_size, str8_prefix(cursor_line, tv->cursor.column-1)).x + margin_width_px + line_num_width_px);
      Rng1S64 visible_pixel_range =
      {
        view->scroll_pos.x.idx,
        view->scroll_pos.x.idx + (S64)code_area_dim.x,
      };
      Rng1S64 cursor_pixel_range =
      {
        cursor_off - (S64)(big_glyph_advance*4) - (S64)(margin_width_px + line_num_width_px),
        cursor_off + (S64)(big_glyph_advance*4),
      };
      S64 min_delta = Min(0, cursor_pixel_range.min - visible_pixel_range.min);
      S64 max_delta = Max(0, cursor_pixel_range.max - visible_pixel_range.max);
      S64 new_idx = view->scroll_pos.x.idx+min_delta+max_delta;
      new_idx = Clamp(scroll_idx_rng[Axis2_X].min, new_idx, scroll_idx_rng[Axis2_X].max);
      ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
    }
    
    // rjf: snap in Y
    if(snap[Axis2_Y])
    {
      Rng1S64 cursor_visibility_range = r1s64(tv->cursor.line-4, tv->cursor.line+4);
      cursor_visibility_range.min = ClampBot(0, cursor_visibility_range.min);
      cursor_visibility_range.max = ClampBot(0, cursor_visibility_range.max);
      S64 min_delta = Min(0, cursor_visibility_range.min-(target_visible_line_num_range.min));
      S64 max_delta = Max(0, cursor_visibility_range.max-(target_visible_line_num_range.min+num_possible_visible_lines));
      S64 new_idx = view->scroll_pos.y.idx+min_delta+max_delta;
      new_idx = Clamp(0, new_idx, (S64)txti_buffer_info.total_line_count-1);
      ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: build horizontal scroll bar
  //
  if(txti_buffer_is_ready)
  {
    ui_set_next_fixed_x(0);
    ui_set_next_fixed_y(code_area_dim.y);
    ui_set_next_fixed_width(panel_box_dim.x - scroll_bar_dim);
    ui_set_next_fixed_height(scroll_bar_dim);
    {
      view->scroll_pos.x = ui_scroll_bar(Axis2_X,
                                         ui_px(scroll_bar_dim, 1.f),
                                         view->scroll_pos.x,
                                         scroll_idx_rng[Axis2_X],
                                         (S64)code_area_dim.x);
    }
  }
  
  //////////////////////////////
  //- rjf: build vertical scroll bar
  //
  if(txti_buffer_is_ready)
  {
    ui_set_next_fixed_x(code_area_dim.x);
    ui_set_next_fixed_y(0);
    ui_set_next_fixed_width(scroll_bar_dim);
    ui_set_next_fixed_height(panel_box_dim.y - bottom_bar_dim.y - scroll_bar_dim);
    {
      view->scroll_pos.y = ui_scroll_bar(Axis2_Y,
                                         ui_px(scroll_bar_dim, 1.f),
                                         view->scroll_pos.y,
                                         scroll_idx_rng[Axis2_Y],
                                         num_possible_visible_lines);
    }
  }
  
  //////////////////////////////
  //- rjf: top-level container interaction (scrolling)
  //
  if(txti_buffer_is_ready)
  {
    UI_Signal sig = ui_signal_from_box(container_box);
    if(sig.scroll.x != 0)
    {
      S64 new_idx = view->scroll_pos.x.idx+sig.scroll.x*big_glyph_advance;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_X], new_idx);
      ui_scroll_pt_target_idx(&view->scroll_pos.x, new_idx);
    }
    if(sig.scroll.y != 0)
    {
      S64 new_idx = view->scroll_pos.y.idx + sig.scroll.y;
      new_idx = clamp_1s64(scroll_idx_rng[Axis2_Y], new_idx);
      ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
    }
    ui_scroll_pt_clamp_idx(&view->scroll_pos.x, scroll_idx_rng[Axis2_X]);
    ui_scroll_pt_clamp_idx(&view->scroll_pos.y, scroll_idx_rng[Axis2_Y]);
  }
  
  //////////////////////////////
  //- rjf: build bottom info bar
  //
  if(txti_buffer_is_ready)
  {
    ui_set_next_fixed_x(0);
    ui_set_next_fixed_y(code_area_dim.y + scroll_bar_dim);
    ui_set_next_pref_width(ui_px(bottom_bar_dim.x, 1));
    ui_set_next_pref_height(ui_px(bottom_bar_dim.y, 1));
    ui_set_next_background_color(df_rgba_from_theme_color(DF_ThemeColor_AltBackground));
    ui_set_next_flags(UI_BoxFlag_DrawBackground);
    UI_Row
      UI_TextAlignment(UI_TextAlign_Center)
      UI_PrefWidth(ui_text_dim(10, 1))
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      UI_Font(code_font)
    {
      ui_labelf("Row: %I64d, Col: %I64d", tv->cursor.line, tv->cursor.column);
      ui_spacer(ui_pct(1, 0));
      ui_labelf("(read only)");
    }
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Memory @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Memory)
{
  DF_MemoryViewState *mv = df_view_user_state(view, DF_MemoryViewState);
  if(mv->initialized == 0)
  {
    mv->initialized = 1;
    mv->num_columns = 16;
    mv->bytes_per_cell = 1;
    mv->last_viewed_memory_cache_arena = df_view_push_arena_ext(view);
  }
}

DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Memory)
{
  return str8_lit("");
}

DF_VIEW_CMD_FUNCTION_DEF(Memory)
{
  DF_MemoryViewState *mv = df_view_user_state(view, DF_MemoryViewState);
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    DF_CmdParams *params = &cmd->params;
    switch(core_cmd_kind)
    {
      default: break;
      case DF_CoreCmdKind_CenterCursor:
      if(df_view_from_handle(params->view) == view)
      {
        mv->center_cursor = 1;
      }break;
      case DF_CoreCmdKind_ContainCursor:
      if(df_view_from_handle(params->view) == view)
      {
        mv->contain_cursor = 1;
      }break;
      case DF_CoreCmdKind_GoToAddress:
      {
        // NOTE(rjf): go-to-address occurs with disassembly snaps, and we don't
        // generally want to respond to those in thise view, so just skip any
        // go-to-address commands that haven't been *explicitly* parameterized
        // with this view.
        if(df_view_from_handle(params->view) == view)
        {
          mv->cursor = mv->mark = params->vaddr;
          mv->center_cursor = 1;
        }
      }break;
      case DF_CoreCmdKind_SetColumns:
      if(df_view_from_handle(params->view) == view)
      {
        U64 num_columns = params->index;
        mv->num_columns = Clamp(1, num_columns, 64);
        if(mv->num_columns % mv->bytes_per_cell != 0)
        {
          mv->bytes_per_cell = 1;
        }
        mv->center_cursor = 1;
      }break;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(Memory)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  
  //////////////////////////////
  //- rjf: unpack state
  //
  DF_MemoryViewState *mv = df_view_user_state(view, DF_MemoryViewState);
  
  //////////////////////////////
  //- rjf: unpack entity params
  //
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  
  //////////////////////////////
  //- rjf: unpack visual params
  //
  F_Tag font = df_font_from_slot(DF_FontSlot_Code);
  F32 font_size = df_font_size_from_slot(ws, DF_FontSlot_Code);
  F32 big_glyph_advance = f_dim_from_tag_size_string(font, font_size, str8_lit("H")).x;
  F32 row_height_px = floor_f32(font_size*2.f);
  F32 cell_width_px = floor_f32(font_size*2.f * mv->bytes_per_cell);
  F32 scroll_bar_dim = floor_f32(ui_top_font_size()*1.5f);
  Vec2F32 panel_dim = dim_2f32(rect);
  F32 footer_dim = font_size*10.f;
  Rng2F32 header_rect = r2f32p(0, 0, panel_dim.x, row_height_px);
  Rng2F32 footer_rect = r2f32p(0, panel_dim.y-footer_dim, panel_dim.x, panel_dim.y);
  Rng2F32 content_rect = r2f32p(0, row_height_px, panel_dim.x-scroll_bar_dim, footer_rect.y0);
  
  //////////////////////////////
  //- rjf: determine legal scroll range
  //
  Rng1S64 scroll_idx_rng = r1s64(0, 0x7FFFFFFFFFFFull/mv->num_columns);
  
  //////////////////////////////
  //- rjf: determine info about visible range of rows
  //
  Rng1S64 viz_range_rows = {0};
  Rng1U64 viz_range_bytes = {0};
  S64 num_possible_visible_rows = 0;
  {
    num_possible_visible_rows = dim_2f32(content_rect).y/row_height_px;
    viz_range_rows.min = view->scroll_pos.y.idx + (S64)view->scroll_pos.y.off - !!(view->scroll_pos.y.off<0);
    viz_range_rows.max = view->scroll_pos.y.idx + (S64)view->scroll_pos.y.off + num_possible_visible_rows,
    viz_range_rows.min = clamp_1s64(scroll_idx_rng, viz_range_rows.min);
    viz_range_rows.max = clamp_1s64(scroll_idx_rng, viz_range_rows.max);
    viz_range_bytes.min = viz_range_rows.min*mv->num_columns;
    viz_range_bytes.max = (viz_range_rows.max+1)*mv->num_columns+1;
    if(viz_range_bytes.min > viz_range_bytes.max)
    {
      Swap(U64, viz_range_bytes.min, viz_range_bytes.max);
    }
  }
  
  //////////////////////////////
  //- rjf: take keyboard controls
  //
  UI_Focus(UI_FocusKind_On) if(ui_is_focus_active())
  {
    U64 next_cursor = mv->cursor;
    U64 next_mark = mv->mark;
    UI_EventList *events = ui_events();
    for(UI_EventNode *n = events->first, *next = 0; n != 0; n = next)
    {
      next = n->next;
      UI_Event *evt = &n->v;
      Vec2S64 cell_delta = {0};
      switch(evt->delta_unit)
      {
        default:{}break;
        case UI_EventDeltaUnit_Char:
        {
          cell_delta.x = (S64)evt->delta_2s32.x;
          cell_delta.y = (S64)evt->delta_2s32.y;
        }break;
        case UI_EventDeltaUnit_Word:
        case UI_EventDeltaUnit_Page:
        {
          if(evt->delta_2s32.x < 0)
          {
            cell_delta.x = -(S64)(mv->cursor%mv->num_columns);
          }
          else if(evt->delta_2s32.x > 0)
          {
            cell_delta.x = (mv->num_columns-1) - (S64)(mv->cursor%mv->num_columns);
          }
          if(evt->delta_2s32.y < 0)
          {
            cell_delta.y = -4;
          }
          else if(evt->delta_2s32.y > 0)
          {
            cell_delta.y = +4;
          }
        }break;
      }
      B32 good_action = 0;
      if(evt->delta_2s32.x != 0 || evt->delta_2s32.y != 0)
      {
        good_action = 1;
      }
      if(good_action && evt->flags & UI_EventFlag_ZeroDeltaOnSelect && mv->cursor != mv->mark)
      {
        MemoryZeroStruct(&cell_delta);
      }
      if(good_action)
      {
        cell_delta.x = ClampBot(cell_delta.x, (S64)-next_cursor);
        cell_delta.y = ClampBot(cell_delta.y, (S64)-(next_cursor/mv->num_columns));
        next_cursor += cell_delta.x;
        next_cursor += cell_delta.y*mv->num_columns;
        next_cursor = ClampTop(0x7FFFFFFFFFFFull, next_cursor);
      }
      if(good_action && evt->flags & UI_EventFlag_PickSelectSide && mv->cursor != mv->mark)
      {
        if(evt->delta_2s32.x < 0 || evt->delta_2s32.y < 0)
        {
          next_cursor = Min(mv->cursor, mv->mark);
        }
        else
        {
          next_cursor = Max(mv->cursor, mv->mark);
        }
      }
      if(good_action && !(evt->flags & UI_EventFlag_KeepMark))
      {
        next_mark = next_cursor;
      }
      if(good_action)
      {
        mv->contain_cursor = 1;
        ui_eat_event(events, n);
      }
    }
    mv->cursor = next_cursor;
    mv->mark = next_mark;
  }
  
  //////////////////////////////
  //- rjf: clamp cursor
  //
  {
    Rng1U64 cursor_valid_rng = r1u64(0, 0x7FFFFFFFFFFFull);
    mv->cursor = clamp_1u64(cursor_valid_rng, mv->cursor);
    mv->mark = clamp_1u64(cursor_valid_rng, mv->mark);
  }
  
  //////////////////////////////
  //- rjf: center cursor
  //
  if(mv->center_cursor)
  {
    mv->center_cursor = 0;
    S64 cursor_row_idx = mv->cursor/mv->num_columns;
    S64 new_idx = (cursor_row_idx-num_possible_visible_rows/2+1);
    new_idx = clamp_1s64(scroll_idx_rng, new_idx);
    ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
  }
  
  //////////////////////////////
  //- rjf: contain cursor
  //
  if(mv->contain_cursor)
  {
    mv->contain_cursor = 0;
    S64 cursor_row_idx = mv->cursor/mv->num_columns;
    Rng1S64 cursor_viz_range = r1s64(clamp_1s64(scroll_idx_rng, cursor_row_idx-2), clamp_1s64(scroll_idx_rng, cursor_row_idx+3));
    S64 min_delta = Min(0, cursor_viz_range.min-viz_range_rows.min);
    S64 max_delta = Max(0, cursor_viz_range.max-viz_range_rows.max);
    S64 new_idx = view->scroll_pos.y.idx+min_delta+max_delta;
    new_idx = clamp_1s64(scroll_idx_rng, new_idx);
    ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
  }
  
  //////////////////////////////
  //- rjf: produce fancy string runs for all possible byte values in all cells
  //
  D_FancyStringList byte_fancy_strings[256] = {0};
  {
    Vec4F32 full_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight1);
    Vec4F32 zero_color = df_rgba_from_theme_color(DF_ThemeColor_WeakText);
    for(U64 idx = 0; idx < ArrayCount(byte_fancy_strings); idx += 1)
    {
      U8 byte = (U8)idx;
      F32 pct = (byte/255.f);
      Vec4F32 text_color = mix_4f32(zero_color, full_color, pct);
      if(byte == 0)
      {
        text_color.w *= 0.5f;
      }
      D_FancyString fstr = {font, push_str8f(scratch.arena, "%02x", byte), text_color, font_size, 0, 0};
      d_fancy_string_list_push(scratch.arena, &byte_fancy_strings[idx], &fstr);
    }
  }
  
  //////////////////////////////
  //- rjf: grab windowed memory
  //
  U64 visible_memory_size = dim_1u64(viz_range_bytes);
  U8 *visible_memory = 0;
  {
    Rng1U64 chunk_aligned_range_bytes = r1u64(AlignDownPow2(viz_range_bytes.min, KB(4)), AlignPow2(viz_range_bytes.max, KB(4)));
    U64 current_memgen_idx = ctrl_mem_gen();
    B32 range_changed = (chunk_aligned_range_bytes.min != mv->last_viewed_memory_cache_range.min ||
                         chunk_aligned_range_bytes.max != mv->last_viewed_memory_cache_range.max);
    B32 mem_changed = (current_memgen_idx != mv->last_viewed_memory_cache_memgen_idx);
    if(range_changed || mem_changed)
    {
      Temp scratch = scratch_begin(0, 0);
      
      // rjf: try to read new memory for this range
      U64 bytes_to_read = dim_1u64(chunk_aligned_range_bytes);
      U8 *buffer = push_array_no_zero(scratch.arena, U8, bytes_to_read);
      U64 half1_bytes_read = dmn_process_read(process->ctrl_handle, r1u64(chunk_aligned_range_bytes.min, chunk_aligned_range_bytes.min+bytes_to_read/2), buffer+0);
      U64 half2_bytes_read = dmn_process_read(process->ctrl_handle, r1u64(chunk_aligned_range_bytes.min+bytes_to_read/2, chunk_aligned_range_bytes.max), buffer+bytes_to_read/2);
      
      // rjf: worked? -> clear cache & store
      if(half1_bytes_read+half2_bytes_read >= bytes_to_read)
      {
        arena_clear(mv->last_viewed_memory_cache_arena);
        mv->last_viewed_memory_cache_buffer = push_array_no_zero(mv->last_viewed_memory_cache_arena, U8, bytes_to_read);
        MemoryCopy(mv->last_viewed_memory_cache_buffer, buffer, bytes_to_read);
      }
      
      // rjf: didn't work, but range didn't change? -> no-op
      if(half1_bytes_read == 0 && half2_bytes_read == 0 && !range_changed)
      {
        // NOTE(rjf): nothing - use stale memory from cache.
      }
      
      // rjf: didn't work, but range DID change? -> clear cache
      if(half1_bytes_read == 0 && half2_bytes_read == 0 && range_changed)
      {
        arena_clear(mv->last_viewed_memory_cache_arena);
        mv->last_viewed_memory_cache_buffer = push_array(mv->last_viewed_memory_cache_arena, U8, bytes_to_read);
      }
      
      // rjf: didn't fully work, but changed? -> clear cache memory, fill what we can, zero the rest.
      if(half1_bytes_read+half2_bytes_read < bytes_to_read && half1_bytes_read+half2_bytes_read != 0)
      {
        arena_clear(mv->last_viewed_memory_cache_arena);
        mv->last_viewed_memory_cache_buffer = push_array(mv->last_viewed_memory_cache_arena, U8, bytes_to_read);
        MemoryCopy(mv->last_viewed_memory_cache_buffer+0,               buffer+0,               half1_bytes_read);
        MemoryCopy(mv->last_viewed_memory_cache_buffer+bytes_to_read/2, buffer+bytes_to_read/2, half2_bytes_read);
      }
      
      // rjf: update cache stamps
      if(!df_ctrl_targets_running())
      {
        mv->last_viewed_memory_cache_range = chunk_aligned_range_bytes;
        mv->last_viewed_memory_cache_memgen_idx = current_memgen_idx;
      }
      
      scratch_end(scratch);
    }
    visible_memory = mv->last_viewed_memory_cache_buffer + viz_range_bytes.min-chunk_aligned_range_bytes.min;
  }
  
  //////////////////////////////
  //- rjf: grab annotations for windowed range of memory
  //
  typedef struct Annotation Annotation;
  struct Annotation
  {
    Annotation *next;
    String8 name_string;
    String8 kind_string;
    String8 type_string;
    Vec4F32 color;
    Rng1U64 vaddr_range;
  };
  typedef struct AnnotationList AnnotationList;
  struct AnnotationList
  {
    Annotation *first;
    Annotation *last;
  };
  AnnotationList *visible_memory_annotations = push_array(scratch.arena, AnnotationList, visible_memory_size);
  {
    CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
    
    //- rjf: fill unwind frame annotations
    if(unwind.first != 0)
    {
      U64 last_stack_top = regs_rsp_from_arch_block(thread->arch, unwind.first->regs);
      for(CTRL_UnwindFrame *f = unwind.first->next; f != 0; f = f->next)
      {
        U64 f_stack_top = regs_rsp_from_arch_block(thread->arch, f->regs);
        Rng1U64 frame_vaddr_range = r1u64(last_stack_top, f_stack_top);
        Rng1U64 frame_vaddr_range_in_viz = intersect_1u64(frame_vaddr_range, viz_range_bytes);
        last_stack_top = f_stack_top;
        if(dim_1u64(frame_vaddr_range_in_viz) != 0)
        {
          DF_Entity *module = df_module_from_process_vaddr(process, f->rip);
          DF_Entity *binary = df_binary_file_from_module(module);
          U64 rip_voff = df_voff_from_vaddr(module, f->rip);
          String8 symbol_name = df_symbol_name_from_binary_voff(scratch.arena, binary, rip_voff);
          Annotation *annotation = push_array(scratch.arena, Annotation, 1);
          annotation->name_string = symbol_name.size != 0 ? symbol_name : str8_lit("[external code]");
          annotation->kind_string = str8_lit("Call Stack Frame");
          annotation->color = symbol_name.size != 0 ? df_rgba_from_theme_color(DF_ThemeColor_CodeFunction) : df_rgba_from_theme_color(DF_ThemeColor_WeakText);
          annotation->vaddr_range = frame_vaddr_range;
          for(U64 vaddr = frame_vaddr_range_in_viz.min; vaddr < frame_vaddr_range_in_viz.max; vaddr += 1)
          {
            U64 visible_byte_idx = vaddr - viz_range_bytes.min;
            SLLQueuePush(visible_memory_annotations[visible_byte_idx].first, visible_memory_annotations[visible_byte_idx].last, annotation);
          }
        }
      }
    }
    
    //- rjf: fill selected thread stack range annotation
    if(unwind.first != 0)
    {
      U64 stack_base_vaddr = thread->stack_base;
      U64 stack_top_vaddr = regs_rsp_from_arch_block(thread->arch, unwind.first->regs);
      Rng1U64 stack_vaddr_range = r1u64(stack_base_vaddr, stack_top_vaddr);
      Rng1U64 stack_vaddr_range_in_viz = intersect_1u64(stack_vaddr_range, viz_range_bytes);
      if(dim_1u64(stack_vaddr_range_in_viz) != 0)
      {
        Annotation *annotation = push_array(scratch.arena, Annotation, 1);
        annotation->name_string = df_display_string_from_entity(scratch.arena, thread);
        annotation->kind_string = str8_lit("Stack");
        annotation->color = thread->flags & DF_EntityFlag_HasColor ? df_rgba_from_entity(thread) : df_rgba_from_theme_color(DF_ThemeColor_PlainText);
        annotation->vaddr_range = stack_vaddr_range;
        for(U64 vaddr = stack_vaddr_range_in_viz.min; vaddr < stack_vaddr_range_in_viz.max; vaddr += 1)
        {
          U64 visible_byte_idx = vaddr - viz_range_bytes.min;
          SLLQueuePush(visible_memory_annotations[visible_byte_idx].first, visible_memory_annotations[visible_byte_idx].last, annotation);
        }
      }
    }
    
    //- rjf: fill local variable annotations
    {
      Vec4F32 color_gen_table[] =
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
      DBGI_Scope *scope = dbgi_scope_open();
      U64 thread_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx.unwind_count);
      EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(scope, process, thread_rip_vaddr);
      RDI_Parsed *rdi = parse_ctx.rdi;
      for(EVAL_String2NumMapNode *n = parse_ctx.locals_map->first; n != 0; n = n->order_next)
      {
        String8 local_name = n->string;
        DF_Eval local_eval = df_eval_from_string(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, local_name);
        if(local_eval.mode == EVAL_EvalMode_Addr)
        {
          TG_Kind local_eval_type_kind = tg_kind_from_key(local_eval.type_key);
          U64 local_eval_type_size = tg_byte_size_from_graph_rdi_key(parse_ctx.type_graph, rdi, local_eval.type_key);
          Rng1U64 vaddr_rng = r1u64(local_eval.offset, local_eval.offset+local_eval_type_size);
          Rng1U64 vaddr_rng_in_visible = intersect_1u64(viz_range_bytes, vaddr_rng);
          if(vaddr_rng_in_visible.max != vaddr_rng_in_visible.min)
          {
            Annotation *annotation = push_array(scratch.arena, Annotation, 1);
            {
              annotation->name_string = push_str8_copy(scratch.arena, local_name);
              annotation->kind_string = str8_lit("Local");
              annotation->type_string = tg_string_from_key(scratch.arena, parse_ctx.type_graph, parse_ctx.rdi, local_eval.type_key);
              annotation->color = color_gen_table[(vaddr_rng.min/8)%ArrayCount(color_gen_table)];
              annotation->vaddr_range = vaddr_rng;
            }
            for(U64 vaddr = vaddr_rng_in_visible.min; vaddr < vaddr_rng_in_visible.max; vaddr += 1)
            {
              SLLQueuePushFront(visible_memory_annotations[vaddr-viz_range_bytes.min].first, visible_memory_annotations[vaddr-viz_range_bytes.min].last, annotation);
            }
          }
        }
      }
      dbgi_scope_close(scope);
    }
  }
  
  //////////////////////////////
  //- rjf: build main container
  //
  UI_Box *container_box = &ui_g_nil_box;
  {
    Vec2F32 dim = dim_2f32(rect);
    ui_set_next_fixed_width(dim.x);
    ui_set_next_fixed_height(dim.y);
    ui_set_next_child_layout_axis(Axis2_Y);
    container_box = ui_build_box_from_stringf(0, "memory_view_container_%p", view);
  }
  
  //////////////////////////////
  //- rjf: build header
  //
  UI_Box *header_box = &ui_g_nil_box;
  UI_Parent(container_box)
  {
    UI_WidthFill UI_PrefHeight(ui_px(row_height_px, 1.f)) UI_Row
      header_box = ui_build_box_from_stringf(UI_BoxFlag_DrawSideBottom, "table_header");
    UI_Parent(header_box)
      UI_Font(font)
      UI_FontSize(font_size)
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      UI_PrefWidth(ui_px(big_glyph_advance*18.f, 1.f)) ui_labelf("Address");
      UI_PrefWidth(ui_px(cell_width_px, 1.f))
        UI_TextAlignment(UI_TextAlign_Center)
      {
        Rng1U64 col_selection_rng = r1u64(mv->cursor%mv->num_columns, mv->mark%mv->num_columns);
        for(U64 row_off = 0; row_off < mv->num_columns*mv->bytes_per_cell; row_off += mv->bytes_per_cell)
        {
          if(col_selection_rng.min <= row_off && row_off <= col_selection_rng.max)
          {
            ui_set_next_text_color(df_rgba_from_theme_color(DF_ThemeColor_PlainText));
          }
          ui_labelf("%I64X", row_off);
        }
      }
      ui_spacer(ui_px(big_glyph_advance*1.5f, 1.f));
      UI_WidthFill ui_labelf("ASCII");
    }
  }
  
  //////////////////////////////
  //- rjf: build scroll bar
  //
  UI_Parent(container_box)
  {
    ui_set_next_fixed_x(content_rect.x1);
    ui_set_next_fixed_y(content_rect.y0);
    ui_set_next_fixed_width(scroll_bar_dim);
    ui_set_next_fixed_height(dim_2f32(content_rect).y);
    {
      view->scroll_pos.y = ui_scroll_bar(Axis2_Y,
                                         ui_px(scroll_bar_dim, 1.f),
                                         view->scroll_pos.y,
                                         scroll_idx_rng,
                                         num_possible_visible_rows);
    }
  }
  
  //////////////////////////////
  //- rjf: build scrollable box
  //
  UI_Box *scrollable_box = &ui_g_nil_box;
  UI_Parent(container_box)
  {
    ui_set_next_fixed_x(content_rect.x0);
    ui_set_next_fixed_y(content_rect.y0);
    ui_set_next_fixed_width(dim_2f32(content_rect).x);
    ui_set_next_fixed_height(dim_2f32(content_rect).y);
    ui_set_next_child_layout_axis(Axis2_Y);
    scrollable_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|
                                               UI_BoxFlag_Scroll|
                                               UI_BoxFlag_AllowOverflowX|
                                               UI_BoxFlag_AllowOverflowY,
                                               "scrollable_box");
    container_box->view_off.x = container_box->view_off_target.x = view->scroll_pos.x.idx + view->scroll_pos.x.off;
    scrollable_box->view_off.y = scrollable_box->view_off_target.y = floor_f32(row_height_px*mod_f32(view->scroll_pos.y.off, 1.f) + row_height_px*(view->scroll_pos.y.off < 0));
  }
  
  //////////////////////////////
  //- rjf: build row container/overlay
  //
  UI_Box *row_container_box = &ui_g_nil_box;
  UI_Box *row_overlay_box = &ui_g_nil_box;
  UI_Parent(scrollable_box) UI_WidthFill UI_HeightFill
  {
    ui_set_next_child_layout_axis(Axis2_Y);
    ui_set_next_hover_cursor(OS_Cursor_IBar);
    row_container_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "row_container");
    UI_Parent(row_container_box)
    {
      row_overlay_box = ui_build_box_from_stringf(UI_BoxFlag_Floating, "row_overlay");
    }
  }
  
  //////////////////////////////
  //- rjf: interact with row container
  //
  U64 mouse_hover_byte_num = 0;
  {
    UI_Signal sig = ui_signal_from_box(row_container_box);
    
    // rjf: calculate hovered byte
    if(ui_hovering(sig) || ui_dragging(sig))
    {
      Vec2F32 mouse_rel = sub_2f32(ui_mouse(), row_container_box->rect.p0);
      U64 row_idx = ClampBot(0, mouse_rel.y) / row_height_px;
      
      // rjf: try from cells
      if(mouse_hover_byte_num == 0)
      {
        U64 col_idx = ClampBot(mouse_rel.x-big_glyph_advance*18.f, 0)/cell_width_px;
        if(col_idx < mv->num_columns)
        {
          mouse_hover_byte_num = viz_range_bytes.min + row_idx*mv->num_columns + col_idx + 1;
        }
      }
      
      // rjf: try from ascii
      if(mouse_hover_byte_num == 0)
      {
        U64 col_idx = ClampBot(mouse_rel.x - (big_glyph_advance*18.f + cell_width_px*mv->num_columns + big_glyph_advance*1.5f), 0)/big_glyph_advance;
        col_idx = ClampTop(col_idx, mv->num_columns-1);
        mouse_hover_byte_num = viz_range_bytes.min + row_idx*mv->num_columns + col_idx + 1;
      }
      
      mouse_hover_byte_num = Clamp(1, mouse_hover_byte_num, 0x7FFFFFFFFFFFull+1);
    }
    
    // rjf: press -> focus panel
    if(ui_pressed(sig))
    {
      DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
    }
    
    // rjf: click & drag -> select
    if(ui_dragging(sig) && mouse_hover_byte_num != 0)
    {
      mv->contain_cursor = 1;
      mv->cursor = mouse_hover_byte_num-1;
      if(ui_pressed(sig))
      {
        mv->mark = mv->cursor;
      }
    }
    
    // rjf: ctrl+scroll -> change font size
    if(ui_hovering(sig))
    {
      UI_EventList *events = ui_events();
      for(UI_EventNode *n = events->first, *next = 0; n != 0; n = next)
      {
        next = n->next;
        UI_Event *event = &n->v;
        if(event->kind == UI_EventKind_Scroll && event->modifiers & OS_EventFlag_Ctrl)
        {
          ui_eat_event(events, n);
          if(event->delta_2f32.y < 0)
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_IncCodeFontScale));
          }
          else if(event->delta_2f32.y > 0)
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_DecCodeFontScale));
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build rows
  //
  UI_Parent(row_container_box) UI_Font(font) UI_FontSize(font_size)
  {
    Rng1U64 selection = r1u64(mv->cursor, mv->mark);
    U8 *row_ascii_buffer = push_array(scratch.arena, U8, mv->num_columns);
    UI_WidthFill UI_PrefHeight(ui_px(row_height_px, 1.f))
      for(S64 row_idx = viz_range_rows.min; row_idx <= viz_range_rows.max; row_idx += 1)
    {
      Rng1U64 row_range_bytes = r1u64(row_idx*mv->num_columns, (row_idx+1)*mv->num_columns);
      B32 row_is_boundary = 0;
      Vec4F32 row_boundary_color = {0};
      if(row_range_bytes.min%64 == 0)
      {
        row_is_boundary = 1;
        row_boundary_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
      }
      ui_set_next_border_color(row_boundary_color);
      UI_Box *row = ui_build_box_from_stringf(UI_BoxFlag_DrawSideTop*!!row_is_boundary, "row_%I64x", row_range_bytes.min);
      UI_Parent(row)
      {
        UI_PrefWidth(ui_px(big_glyph_advance*18.f, 1.f))
        {
          ui_set_next_text_color((selection.max >= row_range_bytes.min && selection.min < row_range_bytes.max)
                                 ? df_rgba_from_theme_color(DF_ThemeColor_PlainText)
                                 : df_rgba_from_theme_color(DF_ThemeColor_WeakText));
          ui_labelf("%016I64X", row_range_bytes.min);
        }
        UI_PrefWidth(ui_px(cell_width_px, 1.f))
          UI_TextAlignment(UI_TextAlign_Center)
          UI_CornerRadius(0)
        {
          Vec4F32 full_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight1);
          Vec4F32 zero_color = df_rgba_from_theme_color(DF_ThemeColor_WeakText);
          for(U64 col_idx = 0; col_idx < mv->num_columns; col_idx += 1)
          {
            U64 visible_byte_idx = (row_idx-viz_range_rows.min)*mv->num_columns + col_idx;
            U64 global_byte_idx = viz_range_bytes.min+visible_byte_idx;
            U64 global_byte_num = global_byte_idx+1;
            U8 byte_value = visible_memory[visible_byte_idx];
            Annotation *annotation = visible_memory_annotations[visible_byte_idx].first;
            UI_BoxFlags cell_flags = 0;
            Vec4F32 cell_border_rgba = {0};
            Vec4F32 cell_bg_rgba = {0};
            if(global_byte_num == mouse_hover_byte_num)
            {
              cell_flags |= UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideRight;
              cell_border_rgba = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
            }
            if(annotation != 0)
            {
              cell_flags |= UI_BoxFlag_DrawBackground;
              cell_bg_rgba = annotation->color;
              if(contains_1u64(annotation->vaddr_range, mouse_hover_byte_num-1))
              {
                cell_bg_rgba.w *= 0.15f;
              }
              else
              {
                cell_bg_rgba.w *= 0.08f;
              }
            }
            if(selection.min <= global_byte_idx && global_byte_idx <= selection.max)
            {
              cell_flags |= UI_BoxFlag_DrawBackground;
              cell_bg_rgba = df_rgba_from_theme_color(DF_ThemeColor_TextSelection);
            }
            if(cell_flags & (UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideRight|UI_BoxFlag_DrawSideBottom))
            {
              ui_set_next_border_color(cell_border_rgba);
            }
            if(cell_flags & UI_BoxFlag_DrawBackground)
            {
              ui_set_next_background_color(cell_bg_rgba);
            }
            UI_Box *cell_box = ui_build_box_from_key(UI_BoxFlag_DrawText|cell_flags, ui_key_zero());
            ui_box_equip_display_fancy_strings(cell_box, &byte_fancy_strings[byte_value]);
            {
              F32 off = 0;
              for(Annotation *a = annotation; a != 0; a = a->next)
              {
                if(global_byte_idx == a->vaddr_range.min) UI_Parent(row_overlay_box)
                {
                  ui_set_next_background_color(annotation->color);
                  ui_set_next_fixed_x(big_glyph_advance*18.f + col_idx*cell_width_px + -cell_width_px/8.f + off);
                  ui_set_next_fixed_y((row_idx-viz_range_rows.min)*row_height_px + -cell_width_px/8.f);
                  ui_set_next_fixed_width(cell_width_px/4.f);
                  ui_set_next_fixed_height(cell_width_px/4.f);
                  ui_set_next_corner_radius_00(cell_width_px/8.f);
                  ui_set_next_corner_radius_01(cell_width_px/8.f);
                  ui_set_next_corner_radius_10(cell_width_px/8.f);
                  ui_set_next_corner_radius_11(cell_width_px/8.f);
                  ui_build_box_from_key(UI_BoxFlag_Floating|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow, ui_key_zero());
                  off += cell_width_px/8.f + cell_width_px/16.f;
                }
              }
            }
            if(annotation != 0 && mouse_hover_byte_num == global_byte_num) UI_Tooltip UI_FontSize(ui_top_font_size()) UI_PrefHeight(ui_px(ui_top_font_size()*1.75f, 1.f))
            {
              for(Annotation *a = annotation; a != 0; a = a->next)
              {
                UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(10, 1))
                {
                  UI_TextColor(a->color) UI_Font(font) ui_label(a->name_string);
                  UI_Font(df_font_from_slot(DF_FontSlot_Main)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_label(a->kind_string);
                }
                if(a->type_string.size != 0)
                {
                  df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeType), a->type_string);
                }
                UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_label(str8_from_memory_size(scratch.arena, dim_1u64(a->vaddr_range)));
                if(a->next != 0)
                {
                  ui_spacer(ui_em(1.5f, 1.f));
                }
              }
            }
          }
        }
        ui_spacer(ui_px(big_glyph_advance*1.5f, 1.f));
        UI_WidthFill
        {
          MemoryZero(row_ascii_buffer, mv->num_columns);
          for(U64 col_idx = 0; col_idx < mv->num_columns; col_idx += 1)
          {
            U8 byte_value = visible_memory[(row_idx-viz_range_rows.min)*mv->num_columns + col_idx];
            row_ascii_buffer[col_idx] = byte_value;
            if(byte_value <= 32 || 127 < byte_value)
            {
              row_ascii_buffer[col_idx] = '.';
            }
          }
          String8 ascii_text = str8(row_ascii_buffer, mv->num_columns);
          UI_Box *ascii_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText, "%S###ascii_row_%I64x", ascii_text, row_range_bytes.min);
          if(selection.max >= row_range_bytes.min && selection.min < row_range_bytes.max)
          {
            Rng1U64 selection_in_row = intersect_1u64(row_range_bytes, selection);
            D_Bucket *bucket = d_bucket_make();
            D_BucketScope(bucket)
            {
              Vec2F32 text_pos = ui_box_text_position(ascii_box);
              d_rect(r2f32p(text_pos.x + f_dim_from_tag_size_string(font, font_size, str8_prefix(ascii_text, selection_in_row.min+0-row_range_bytes.min)).x - font_size/8.f,
                            ascii_box->rect.y0,
                            text_pos.x + f_dim_from_tag_size_string(font, font_size, str8_prefix(ascii_text, selection_in_row.max+1-row_range_bytes.min)).x + font_size/4.f,
                            ascii_box->rect.y1),
                     df_rgba_from_theme_color(DF_ThemeColor_TextSelection),
                     0, 0, 1.f);
            }
            ui_box_equip_draw_bucket(ascii_box, bucket);
          }
          if(mouse_hover_byte_num != 0 && contains_1u64(row_range_bytes, mouse_hover_byte_num-1))
          {
            D_Bucket *bucket = d_bucket_make();
            D_BucketScope(bucket)
            {
              Vec2F32 text_pos = ui_box_text_position(ascii_box);
              Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
              d_rect(r2f32p(text_pos.x + f_dim_from_tag_size_string(font, font_size, str8_prefix(ascii_text, mouse_hover_byte_num-1-row_range_bytes.min)).x - font_size/8.f,
                            ascii_box->rect.y0,
                            text_pos.x + f_dim_from_tag_size_string(font, font_size, str8_prefix(ascii_text, mouse_hover_byte_num+0-row_range_bytes.min)).x + font_size/4.f,
                            ascii_box->rect.y1),
                     color,
                     1.f, 3.f, 1.f);
            }
            ui_box_equip_draw_bucket(ascii_box, bucket);
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build footer
  //
  UI_Box *footer_box = &ui_g_nil_box;
  UI_Parent(container_box)
  {
    ui_set_next_fixed_x(footer_rect.x0);
    ui_set_next_fixed_y(footer_rect.y0);
    ui_set_next_fixed_width(dim_2f32(footer_rect).x);
    ui_set_next_fixed_height(dim_2f32(footer_rect).y);
    footer_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow, "footer");
    UI_Parent(footer_box) UI_Font(font) UI_FontSize(font_size)
    {
      UI_PrefWidth(ui_em(7.5f, 1.f)) UI_HeightFill UI_Column UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
        UI_PrefHeight(ui_px(row_height_px, 0.f))
      {
        ui_labelf("Address:");
        ui_labelf("U8:");
        ui_labelf("U16:");
        ui_labelf("U32:");
        ui_labelf("U64:");
      }
      UI_PrefWidth(ui_em(45.f, 1.f)) UI_HeightFill UI_Column UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_CodeNumeric))
        UI_PrefHeight(ui_px(row_height_px, 0.f))
      {
        B32 cursor_in_range = (viz_range_bytes.min <= mv->cursor && mv->cursor+8 <= viz_range_bytes.max);
        ui_labelf("%016I64X", mv->cursor);
        if(cursor_in_range)
        {
          U64 as_u8  = 0;
          U64 as_u16 = 0;
          U64 as_u32 = 0;
          U64 as_u64 = 0;
          U64 cursor_off = mv->cursor-viz_range_bytes.min;
          as_u8  = (U64)*(U8 *)(visible_memory + cursor_off);
          as_u16 = (U64)*(U16*)(visible_memory + cursor_off);
          as_u32 = (U64)*(U32*)(visible_memory + cursor_off);
          as_u64 = (U64)*(U64*)(visible_memory + cursor_off);
          ui_labelf("%02X (%I64u)",  as_u8,  as_u8);
          ui_labelf("%04X (%I64u)",  as_u16, as_u16);
          ui_labelf("%08X (%I64u)",  as_u32, as_u32);
          ui_labelf("%016I64X (%I64u)", as_u64, as_u64);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: scroll
  //
  {
    UI_Signal sig = ui_signal_from_box(scrollable_box);
    if(sig.scroll.y != 0)
    {
      S64 new_idx = view->scroll_pos.y.idx + sig.scroll.y;
      new_idx = clamp_1s64(scroll_idx_rng, new_idx);
      ui_scroll_pt_target_idx(&view->scroll_pos.y, new_idx);
    }
  }
  
  hs_scope_close(hs_scope);
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Breakpoints @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Breakpoints) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Breakpoints) {return str8_lit("");}
DF_VIEW_CMD_FUNCTION_DEF(Breakpoints) {}
DF_VIEW_UI_FUNCTION_DEF(Breakpoints)
{
  Temp scratch = scratch_begin(0, 0);
  String8 query = str8(view->query_buffer, view->query_string_size);
  
  //- rjf: get state
  typedef struct DF_BreakpointsViewState DF_BreakpointsViewState;
  struct DF_BreakpointsViewState
  {
    B32 initialized;
    DF_Handle selected_entity;
    S64 selected_column;
    F32 enabled_col_pct;
    F32 desc_col_pct;
    F32 loc_col_pct;
    F32 hit_col_pct;
    F32 del_col_pct;
  };
  DF_BreakpointsViewState *bv = df_view_user_state(view, DF_BreakpointsViewState);
  if(bv->initialized == 0)
  {
    bv->initialized = 1;
    bv->enabled_col_pct = 0.05f;
    bv->desc_col_pct    = 0.25f;
    bv->loc_col_pct     = 0.50f;
    bv->hit_col_pct     = 0.15f;
    bv->del_col_pct     = 0.05f;
  }
  F32 *col_pcts[] = {&bv->enabled_col_pct, &bv->desc_col_pct, &bv->loc_col_pct, &bv->hit_col_pct, &bv->del_col_pct};
  
  //- rjf: get entities
  DF_EntityList entities_list = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
  DF_EntityFuzzyItemArray entities = df_entity_fuzzy_item_array_from_entity_list_needle(scratch.arena, &entities_list, query);
  
  //- rjf: selected column/entity -> selected cursor
  Vec2S64 cursor = {bv->selected_column};
  for(U64 idx = 0; idx < entities.count; idx += 1)
  {
    if(entities.v[idx].entity == df_entity_from_handle(bv->selected_entity))
    {
      cursor.y = (S64)(idx+1);
      break;
    }
  }
  
  //- rjf: build table
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = floor_f32(ui_top_font_size()*2.5f);
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(4, entities.count));
    scroll_list_params.item_range    = r1s64(0, entities.count+1);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_TableF(ArrayCount(col_pcts), col_pcts, "breakpoints_table")
  {
    if(visible_row_range.min == 0) UI_TableVector UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      UI_TableCell{}
      UI_TableCell{ui_labelf("Name");}
      UI_TableCell{ui_labelf("Location");}
      UI_TableCell{ui_labelf("Hit Count");}
      UI_TableCell{}
    }
    Vec2S64 next_cursor = cursor;
    for(U64 idx = Max(1, visible_row_range.min); idx <= visible_row_range.max && idx <= entities.count; idx += 1)
    {
      DF_Entity *entity = entities.v[idx-1].entity;
      B32 row_is_selected = (cursor.y == (S64)(idx));
      UI_NamedTableVectorF("breakpoint_%p", entity)
      {
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 0) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          if(ui_clicked(df_icon_buttonf(entity->b32 ? DF_IconKind_CheckFilled : DF_IconKind_CheckHollow, 0, "###ebl_%p", entity)))
          {
            df_entity_equip_b32(entity, !entity->b32);
          }
        }
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          df_entity_desc_button(ws, entity, &entities.v[idx-1].matches, query);
        }
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 2) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          B32 loc_is_code = 0;
          String8 loc_string = {0};
          DF_Entity *file_parent = df_entity_ancestor_from_kind(entity, DF_EntityKind_File);
          DF_Entity *symbol_name = df_entity_child_from_kind(entity, DF_EntityKind_EntryPointName);
          if(!df_entity_is_nil(file_parent))
          {
            loc_string = push_str8f(scratch.arena, "%S:%I64u:%I64u", file_parent->name, entity->text_point.line, entity->text_point.column);
          }
          else if(!df_entity_is_nil(symbol_name))
          {
            loc_string = symbol_name->name;
            loc_is_code = 1;
          }
          else if(entity->flags & DF_EntityFlag_HasVAddr)
          {
            loc_string = push_str8f(scratch.arena, "0x%016I64x", entity->vaddr);
            loc_is_code = 1;
          }
          UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "loc_%p", entity);
          UI_Parent(box)
          {
            UI_Font(loc_is_code ? df_font_from_slot(DF_FontSlot_Code) : ui_top_font())
            {
              ui_label(loc_string);
            }
          }
          UI_Signal sig = ui_signal_from_box(box);
          if(ui_double_clicked(sig) || sig.f&UI_SignalFlag_KeyboardPressed)
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.file_path = df_full_path_from_entity(scratch.arena, file_parent);
            params.text_point = entity->text_point;
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
          }
          if(ui_pressed(sig))
          {
            next_cursor = v2s64(2, (S64)(idx));
            DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
        }
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 3) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###cnd_%p", entity);
          UI_Parent(box)
          {
            String8 hit_count_string = str8_from_u64(scratch.arena, entity->u64, 10, 0, 0);
            UI_Font(df_font_from_slot(DF_FontSlot_Code)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), hit_count_string);
          }
          UI_Signal sig = ui_signal_from_box(box);
          if(ui_pressed(sig))
          {
            next_cursor = v2s64(3, (S64)(idx));
            DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
        }
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 4) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          if(ui_clicked(df_icon_buttonf(DF_IconKind_Trash, 0, "###del_%p", entity)))
          {
            df_entity_mark_for_deletion(entity);
          }
        }
      }
    }
    cursor = next_cursor;
  }
  
  //- rjf: selected num -> selected entity
  bv->selected_column = cursor.x;
  bv->selected_entity = (1 <= cursor.y && cursor.y <= entities.count) ? df_handle_from_entity(entities.v[cursor.y-1].entity) : df_handle_zero();
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: WatchPins @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(WatchPins) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(WatchPins) {return str8_lit("");}
DF_VIEW_CMD_FUNCTION_DEF(WatchPins) {}
DF_VIEW_UI_FUNCTION_DEF(WatchPins)
{
  Temp scratch = scratch_begin(0, 0);
  String8 query = str8(view->query_buffer, view->query_string_size);
  
  //- rjf: get state
  typedef struct DF_WatchPinsViewState DF_WatchPinsViewState;
  struct DF_WatchPinsViewState
  {
    B32 initialized;
    DF_Handle selected_entity;
    S64 selected_column;
    F32 desc_col_pct;
    F32 loc_col_pct;
    F32 del_col_pct;
  };
  DF_WatchPinsViewState *pv = df_view_user_state(view, DF_WatchPinsViewState);
  if(pv->initialized == 0)
  {
    pv->initialized = 1;
    pv->desc_col_pct    = 0.35f;
    pv->loc_col_pct     = 0.60f;
    pv->del_col_pct     = 0.05f;
  }
  F32 *col_pcts[] = {&pv->desc_col_pct, &pv->loc_col_pct, &pv->del_col_pct};
  
  //- rjf: get entities
  DF_EntityList entities_list = df_query_cached_entity_list_with_kind(DF_EntityKind_WatchPin);
  DF_EntityFuzzyItemArray entities = df_entity_fuzzy_item_array_from_entity_list_needle(scratch.arena, &entities_list, query);
  
  //- rjf: selected column/entity -> selected cursor
  Vec2S64 cursor = {pv->selected_column};
  for(U64 idx = 0; idx < entities.count; idx += 1)
  {
    if(entities.v[idx].entity == df_entity_from_handle(pv->selected_entity))
    {
      cursor.y = (S64)(idx+1);
      break;
    }
  }
  
  //- rjf: build table
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = floor_f32(ui_top_font_size()*2.5f);
    scroll_list_params.dim_px        = dim_2f32(rect);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(2, entities.count));
    scroll_list_params.item_range    = r1s64(0, entities.count+1);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
    UI_TableF(ArrayCount(col_pcts), col_pcts, "pins_table")
  {
    if(visible_row_range.min == 0) UI_TableVector UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    {
      UI_TableCell{ui_labelf("Name");}
      UI_TableCell{ui_labelf("Location");}
      UI_TableCell{}
    }
    Vec2S64 next_cursor = cursor;
    for(U64 idx = Max(1, visible_row_range.min); idx <= visible_row_range.max && idx <= entities.count; idx += 1)
    {
      DF_Entity *entity = entities.v[idx-1].entity;
      B32 row_is_selected = (cursor.y == (S64)(idx));
      UI_NamedTableVectorF("pin_%p", entity)
      {
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 0) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          df_entity_desc_button(ws, entity, &entities.v[idx-1].matches, query);
        }
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          String8 loc_string = {0};
          DF_Entity *file_parent = df_entity_ancestor_from_kind(entity, DF_EntityKind_File);
          if(!df_entity_is_nil(file_parent))
          {
            loc_string = push_str8f(scratch.arena, "%S:%I64u:%I64u", file_parent->name, entity->text_point.line, entity->text_point.column);
          }
          else if(entity->flags & DF_EntityFlag_HasVAddr)
          {
            loc_string = push_str8f(scratch.arena, "0x%016I64x", entity->vaddr);
          }
          UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|UI_BoxFlag_DrawText, "%S###loc_%p", loc_string, entity);
          UI_Signal sig = ui_signal_from_box(box);
          if(ui_double_clicked(sig) || sig.f&UI_SignalFlag_KeyboardPressed)
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.file_path = df_full_path_from_entity(scratch.arena, file_parent);
            params.text_point = entity->text_point;
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
          }
          if(ui_pressed(sig))
          {
            next_cursor = v2s64(1, (S64)(idx));
            DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
        }
        UI_TableCell UI_FocusHot((row_is_selected && cursor.x == 2) ? UI_FocusKind_On : UI_FocusKind_Off)
        {
          if(ui_clicked(df_icon_buttonf(DF_IconKind_Trash, 0, "###del_%p", entity)))
          {
            df_entity_mark_for_deletion(entity);
          }
        }
      }
    }
    cursor = next_cursor;
  }
  
  //- rjf: selected num -> selected entity
  pv->selected_column = cursor.x;
  pv->selected_entity = (1 <= cursor.y && cursor.y <= entities.count) ? df_handle_from_entity(entities.v[cursor.y-1].entity) : df_handle_zero();
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: ExceptionFilters @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(ExceptionFilters) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(ExceptionFilters) {return str8_lit("");}
DF_VIEW_CMD_FUNCTION_DEF(ExceptionFilters) {}
DF_VIEW_UI_FUNCTION_DEF(ExceptionFilters)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  String8 query = str8(view->query_buffer, view->query_string_size);
  
  //- rjf: get state
  typedef struct DF_ExceptionFiltersViewState DF_ExceptionFiltersViewState;
  struct DF_ExceptionFiltersViewState
  {
    Vec2S64 cursor;
  };
  DF_ExceptionFiltersViewState *sv = df_view_user_state(view, DF_ExceptionFiltersViewState);
  
  //- rjf: get list of options
  typedef struct DF_ExceptionFiltersOption DF_ExceptionFiltersOption;
  struct DF_ExceptionFiltersOption
  {
    String8 name;
    FuzzyMatchRangeList matches;
    B32 is_enabled;
    CTRL_ExceptionCodeKind exception_code_kind;
  };
  typedef struct DF_ExceptionFiltersOptionChunkNode DF_ExceptionFiltersOptionChunkNode;
  struct DF_ExceptionFiltersOptionChunkNode
  {
    DF_ExceptionFiltersOptionChunkNode *next;
    DF_ExceptionFiltersOption *v;
    U64 cap;
    U64 count;
  };
  typedef struct DF_ExceptionFiltersOptionChunkList DF_ExceptionFiltersOptionChunkList;
  struct DF_ExceptionFiltersOptionChunkList
  {
    DF_ExceptionFiltersOptionChunkNode *first;
    DF_ExceptionFiltersOptionChunkNode *last;
    U64 option_count;
    U64 node_count;
  };
  typedef struct DF_ExceptionFiltersOptionArray DF_ExceptionFiltersOptionArray;
  struct DF_ExceptionFiltersOptionArray
  {
    DF_ExceptionFiltersOption *v;
    U64 count;
  };
  DF_ExceptionFiltersOptionChunkList opts_list = {0};
  for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)(CTRL_ExceptionCodeKind_Null+1);
      k < CTRL_ExceptionCodeKind_COUNT;
      k = (CTRL_ExceptionCodeKind)(k+1))
  {
    DF_ExceptionFiltersOptionChunkNode *node = opts_list.last;
    String8 name = push_str8f(scratch.arena, "0x%x %S", ctrl_exception_code_kind_code_table[k], ctrl_exception_code_kind_display_string_table[k]);
    FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, query, name);
    if(matches.count >= matches.needle_part_count)
    {
      if(node == 0 || node->count >= node->cap)
      {
        node = push_array(scratch.arena, DF_ExceptionFiltersOptionChunkNode, 1);
        node->cap = 256;
        node->v = push_array_no_zero(scratch.arena, DF_ExceptionFiltersOption, node->cap);
        SLLQueuePush(opts_list.first, opts_list.last, node);
        opts_list.node_count += 1;
      }
      node->v[node->count].name = name;
      node->v[node->count].matches = matches;
      node->v[node->count].is_enabled = !!(df_state->ctrl_exception_code_filters[k/64] & (1ull<<(k%64)));
      node->v[node->count].exception_code_kind = k;
      node->count += 1;
      opts_list.option_count += 1;
    }
  }
  DF_ExceptionFiltersOptionArray opts = {0};
  {
    opts.count = opts_list.option_count;
    opts.v = push_array_no_zero(scratch.arena, DF_ExceptionFiltersOption, opts.count);
    U64 idx = 0;
    for(DF_ExceptionFiltersOptionChunkNode *n = opts_list.first; n != 0; n = n->next)
    {
      MemoryCopy(opts.v+idx, n->v, n->count*sizeof(DF_ExceptionFiltersOption));
      idx += n->count;
    }
  }
  
  //- rjf: build option table
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    Vec2F32 rect_dim = dim_2f32(rect);
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = rect_dim;
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(0, opts.count));
    scroll_list_params.item_range    = r1s64(0, opts.count);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &sv->cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    for(S64 row = visible_row_range.min; row <= visible_row_range.max && row < opts.count; row += 1)
      UI_FocusHot(sv->cursor.y == row+1 ? UI_FocusKind_On : UI_FocusKind_Off)
    {
      DF_ExceptionFiltersOption *opt = &opts.v[row];
      UI_Signal sig = df_icon_buttonf(opt->is_enabled ? DF_IconKind_CheckFilled : DF_IconKind_CheckHollow, &opt->matches, "%S", opt->name);
      if(ui_clicked(sig))
      {
        if(opt->exception_code_kind != CTRL_ExceptionCodeKind_Null)
        {
          CTRL_ExceptionCodeKind k = opt->exception_code_kind;
          if(opt->is_enabled)
          {
            df_state->ctrl_exception_code_filters[k/64] &= ~(1ull<<(k%64));
          }
          else
          {
            df_state->ctrl_exception_code_filters[k/64] |= (1ull<<(k%64));
          }
        }
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////
//~ rjf: Theme @view_hook_impl

DF_VIEW_SETUP_FUNCTION_DEF(Theme) {}
DF_VIEW_STRING_FROM_STATE_FUNCTION_DEF(Theme) {return str8_lit("");}

DF_VIEW_CMD_FUNCTION_DEF(Theme)
{
  for(DF_CmdNode *n = cmds->first; n != 0; n = n->next)
  {
    DF_Cmd *cmd = &n->cmd;
    
    // rjf: mismatched window/panel => skip
    if(df_window_from_handle(cmd->params.window) != ws ||
       df_panel_from_handle(cmd->params.panel) != panel)
    {
      continue;
    }
    
    //rjf: process
    DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
    switch(core_cmd_kind)
    {
      default:break;
      case DF_CoreCmdKind_PickFile:
      {
        Temp scratch = scratch_begin(0, 0);
        String8 path = cmd->params.file_path;
        String8 data = os_data_from_file_path(scratch.arena, path);
        DF_CfgTable cfg_table = {0};
        df_cfg_table_push_unparsed_string(scratch.arena, &cfg_table, data, DF_CfgSrc_User);
        DF_CfgVal *colors = df_cfg_val_from_string(&cfg_table, str8_lit("colors"));
        for(DF_CfgNode *colors_set = colors->first;
            colors_set != &df_g_nil_cfg_node;
            colors_set = colors_set->next)
        {
          for(DF_CfgNode *color = colors_set->first;
              color != &df_g_nil_cfg_node;
              color = color->next)
          {
            String8 color_name = color->string;
            DF_ThemeColor color_code = DF_ThemeColor_Null;
            for(DF_ThemeColor c = DF_ThemeColor_Null; c < DF_ThemeColor_COUNT; c = (DF_ThemeColor)(c+1))
            {
              if(str8_match(df_g_theme_color_cfg_string_table[c], color_name, StringMatchFlag_CaseInsensitive))
              {
                color_code = c;
                break;
              }
            }
            if(color_code != DF_ThemeColor_Null)
            {
              DF_CfgNode *hex_cfg = color->first;
              String8 hex_string = hex_cfg->string;
              U64 hex_val = 0;
              try_u64_from_str8_c_rules(hex_string, &hex_val);
              Vec4F32 color_rgba = rgba_from_u32((U32)hex_val);
              df_gfx_state->cfg_theme_target.colors[color_code] = color_rgba;
            }
          }
        }
        scratch_end(scratch);
      }break;
    }
  }
}

DF_VIEW_UI_FUNCTION_DEF(Theme)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
  
  //- rjf: get state
  typedef struct DF_ThemeViewState DF_ThemeViewState;
  struct DF_ThemeViewState
  {
    Vec2S64 cursor;
    TxtPt txt_cursor;
    TxtPt txt_mark;
    U8 txt_buffer[1024];
    U64 txt_size;
    DF_ThemeColor color_ctx_menu_color;
    Vec4F32 color_ctx_menu_color_hsva;
  };
  DF_ThemeViewState *sv = df_view_user_state(view, DF_ThemeViewState);
  
  //- rjf: build preset ctx menu
  UI_Key preset_ctx_menu_key = ui_key_from_stringf(ui_key_zero(), "%p_preset_ctx_menu", view);
  UI_CtxMenu(preset_ctx_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
  {
    for(DF_ThemePreset preset = (DF_ThemePreset)0;
        preset < DF_ThemePreset_COUNT;
        preset = (DF_ThemePreset)(preset+1))
    {
      Vec4F32 *colors = df_g_theme_preset_colors_table[preset];
      Vec4F32 bg_color = colors[DF_ThemeColor_PlainBackground];
      Vec4F32 tx_color = colors[DF_ThemeColor_PlainText];
      Vec4F32 bd_color = colors[DF_ThemeColor_PlainBorder];
      ui_set_next_background_color(bg_color);
      ui_set_next_text_color(tx_color);
      ui_set_next_border_color(bd_color);
      if(ui_clicked(ui_buttonf("%S", df_g_theme_preset_display_string_table[preset])))
      {
        MemoryCopy(df_gfx_state->cfg_theme_target.colors, colors, sizeof(df_gfx_state->cfg_theme_target.colors));
      }
    }
  }
  
  //- rjf: produce per-color context menu keys
  UI_Key *color_ctx_menu_keys = push_array(scratch.arena, UI_Key, DF_ThemeColor_COUNT);
  {
    for(DF_ThemeColor color = (DF_ThemeColor)(DF_ThemeColor_Null+1);
        color < DF_ThemeColor_COUNT;
        color = (DF_ThemeColor)(color+1))
    {
      color_ctx_menu_keys[color] = ui_key_from_stringf(ui_key_zero(), "###settings_color_ctx_menu_%I64x", (U64)color);
    }
  }
  
  //- rjf: do color context menus
  for(DF_ThemeColor color = (DF_ThemeColor)(DF_ThemeColor_Null+1);
      color < DF_ThemeColor_COUNT;
      color = (DF_ThemeColor)(color+1))
  {
    UI_CtxMenu(color_ctx_menu_keys[color]) UI_Padding(ui_em(1.5f, 1.f))
      UI_PrefWidth(ui_em(28.5f, 1)) UI_PrefHeight(ui_children_sum(1.f))
    {
      // rjf: build title
      UI_Row
      {
        ui_spacer(ui_em(1.5f, 1.f));
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_label(df_g_theme_color_display_string_table[color]);
      }
      
      ui_spacer(ui_em(1.5f, 1.f));
      
      // rjf: build picker
      {
        ui_set_next_pref_height(ui_em(22.f, 1.f));
        UI_Row UI_Padding(ui_pct(1, 0))
        {
          UI_PrefWidth(ui_em(22.f, 1.f)) UI_PrefHeight(ui_em(22.f, 1.f)) UI_Flags(UI_BoxFlag_FocusNavSkip)
          {
            ui_sat_val_pickerf(sv->color_ctx_menu_color_hsva.x, &sv->color_ctx_menu_color_hsva.y, &sv->color_ctx_menu_color_hsva.z, "###settings_satval_picker");
          }
          
          ui_spacer(ui_em(0.75f, 1.f));
          
          UI_PrefWidth(ui_em(1.5f, 1.f)) UI_PrefHeight(ui_em(22.f, 1.f)) UI_Flags(UI_BoxFlag_FocusNavSkip)
            ui_hue_pickerf(&sv->color_ctx_menu_color_hsva.x, sv->color_ctx_menu_color_hsva.y, sv->color_ctx_menu_color_hsva.z, "###settings_hue_picker");
          
          UI_PrefWidth(ui_em(1.5f, 1.f)) UI_PrefHeight(ui_em(22.f, 1.f)) UI_Flags(UI_BoxFlag_FocusNavSkip)
            ui_alpha_pickerf(&sv->color_ctx_menu_color_hsva.w, "###settings_alpha_picker");
        }
      }
      
      ui_spacer(ui_em(1.5f, 1.f));
      
      // rjf: build line edits
      UI_Row
        UI_WidthFill
        UI_Padding(ui_em(1.5f, 1.f))
        UI_PrefHeight(ui_children_sum(1.f))
        UI_Column
        UI_PrefHeight(ui_em(2.25f, 1.f))
      {
        Vec4F32 hsva = sv->color_ctx_menu_color_hsva;
        Vec3F32 hsv = v3f32(hsva.x, hsva.y, hsva.z);
        Vec3F32 rgb = rgb_from_hsv(hsv);
        Vec4F32 rgba = v4f32(rgb.x, rgb.y, rgb.z, sv->color_ctx_menu_color_hsva.w);
        String8 hex_string = hex_string_from_rgba_4f32(scratch.arena, rgba);
        hex_string = push_str8f(scratch.arena, "#%S", hex_string);
        String8 r_string = push_str8f(scratch.arena, "%.2f", rgba.x);
        String8 g_string = push_str8f(scratch.arena, "%.2f", rgba.y);
        String8 b_string = push_str8f(scratch.arena, "%.2f", rgba.z);
        String8 h_string = push_str8f(scratch.arena, "%.2f", hsva.x);
        String8 s_string = push_str8f(scratch.arena, "%.2f", hsva.y);
        String8 v_string = push_str8f(scratch.arena, "%.2f", hsva.z);
        String8 a_string = push_str8f(scratch.arena, "%.2f", rgba.w);
        UI_Row UI_Font(df_font_from_slot(DF_FontSlot_Code))
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("Hex");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, hex_string, "###hex_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_rgba = rgba_from_hex_string_4f32(string);
            Vec4F32 new_hsva = hsva_from_rgba(new_rgba);
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
        ui_spacer(ui_em(0.75f, 1.f));
        UI_Row
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("R");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, r_string, "###r_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_rgba = v4f32((F32)f64_from_str8(string), rgba.y, rgba.z, rgba.w);
            Vec4F32 new_hsva = hsva_from_rgba(new_rgba);
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
        UI_Row
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("G");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, g_string, "###g_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_rgba = v4f32(rgba.x, (F32)f64_from_str8(string), rgba.z, rgba.w);
            Vec4F32 new_hsva = hsva_from_rgba(new_rgba);
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
        UI_Row
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("B");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, b_string, "###b_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_rgba = v4f32(rgba.x, rgba.y, (F32)f64_from_str8(string), rgba.w);
            Vec4F32 new_hsva = hsva_from_rgba(new_rgba);
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
        ui_spacer(ui_em(0.75f, 1.f));
        UI_Row
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("H");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, h_string, "###h_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_hsva = v4f32((F32)f64_from_str8(string), hsva.y, hsva.z, hsva.w);
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
        UI_Row
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("S");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, s_string, "###s_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_hsva = v4f32(hsva.x, (F32)f64_from_str8(string), hsva.z, hsva.w);
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
        UI_Row
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("V");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, v_string, "###v_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_hsva = v4f32(hsva.x, hsva.y, (F32)f64_from_str8(string), hsva.w);
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
        ui_spacer(ui_em(0.75f, 1.f));
        UI_Row
        {
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(4.5f, 1.f)) ui_labelf("A");
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &sv->txt_cursor, &sv->txt_mark, sv->txt_buffer, sizeof(sv->txt_buffer), &sv->txt_size, 0, a_string, "###a_edit");
          if(ui_committed(sig))
          {
            String8 string = str8(sv->txt_buffer, sv->txt_size);
            Vec4F32 new_hsva = v4f32(hsva.x, hsva.y, hsva.z, (F32)f64_from_str8(string));
            sv->color_ctx_menu_color_hsva = new_hsva;
          }
        }
      }
      
      // rjf: commit state to theme
      Vec4F32 hsva = sv->color_ctx_menu_color_hsva;
      Vec3F32 hsv = v3f32(hsva.x, hsva.y, hsva.z);
      Vec3F32 rgb = rgb_from_hsv(hsv);
      Vec4F32 rgba = v4f32(rgb.x, rgb.y, rgb.z, sv->color_ctx_menu_color_hsva.w);
      df_gfx_state->cfg_theme_target.colors[sv->color_ctx_menu_color] = rgba;
    }
  }
  
  //- rjf: build non-scrolled header
  UI_PrefHeight(ui_px(row_height_px, 1.f)) UI_Row
  {
    // rjf: preset selector
    UI_FocusHot((sv->cursor.y == 1 && sv->cursor.x == 0) ? UI_FocusKind_On : UI_FocusKind_Off)
    {
      UI_Signal preset_sig = df_icon_buttonf(DF_IconKind_Palette, 0, "Apply Preset");
      if(ui_clicked(preset_sig))
      {
        ui_ctx_menu_open(preset_ctx_menu_key, preset_sig.box->key, v2f32(0, dim_2f32(preset_sig.box->rect).y));
      }
    }
    
    // rjf: load-from-file
    UI_FocusHot((sv->cursor.y == 1 && sv->cursor.x == 1) ? UI_FocusKind_On : UI_FocusKind_Off)
    {
      if(ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Load From File")))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        params.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PickFile);
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
      }
    }
  }
  
  //- rjf: build palette table
  Rng1S64 visible_row_range = {0};
  UI_ScrollListParams scroll_list_params = {0};
  {
    Vec2F32 rect_dim = dim_2f32(rect);
    scroll_list_params.flags         = UI_ScrollListFlag_All;
    scroll_list_params.row_height_px = row_height_px;
    scroll_list_params.dim_px        = v2f32(rect_dim.x, rect_dim.y-row_height_px);
    scroll_list_params.cursor_range  = r2s64(v2s64(0, 0), v2s64(1, DF_ThemeColor_COUNT));
    scroll_list_params.item_range    = r1s64(0, DF_ThemeColor_COUNT-1);
    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 1;
  }
  UI_ScrollListSignal scroll_list_sig = {0};
  UI_Focus(UI_FocusKind_On)
    UI_ScrollList(&scroll_list_params,
                  &view->scroll_pos.y,
                  &sv->cursor,
                  0,
                  &visible_row_range,
                  &scroll_list_sig)
    UI_Focus(UI_FocusKind_Null)
  {
    for(S64 row = visible_row_range.min; row <= visible_row_range.max; row += 1)
    {
      DF_ThemeColor color = (DF_ThemeColor)(row+1);
      if(DF_ThemeColor_Null < color && color < DF_ThemeColor_COUNT)
        UI_FocusHot(sv->cursor.y == row+2 ? UI_FocusKind_On : UI_FocusKind_Off)
      {
        Vec4F32 rgba = df_rgba_from_theme_color(color);
        Vec3F32 rgb = v3f32(rgba.x, rgba.y, rgba.z);
        Vec3F32 hsv = hsv_from_rgb(rgb);
        Vec4F32 hsva = v4f32(hsv.x, hsv.y, hsv.z, rgba.w);
        ui_set_next_pref_width(ui_pct(1, 0));
        ui_set_next_background_color(v4f32(0, 0, 0, 0));
        ui_set_next_hover_cursor(OS_Cursor_HandPoint);
        UI_Box *color_row = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|
                                                      UI_BoxFlag_DrawBackground|
                                                      UI_BoxFlag_DrawHotEffects|
                                                      UI_BoxFlag_DrawActiveEffects|
                                                      UI_BoxFlag_Clickable,
                                                      "###color_%I64x", (U64)color);
        UI_Parent(color_row)
        {
          Vec4F32 bg_color = ui_top_background_color();
          Vec4F32 default_text_color = ui_top_text_color();
          F32 default_fallback_factor = clamp_1f32(r1f32(0.3f, 1), dot_4f32(normalize_4f32(rgba), normalize_4f32(bg_color))) - 0.3f;
          Vec4F32 text_rgba = mix_4f32(rgba, default_text_color, default_fallback_factor);
          UI_WidthFill UI_TextColor(text_rgba) ui_label(df_g_theme_color_display_string_table[color]);
          ui_set_next_pref_width(ui_top_pref_height());
          UI_HeightFill UI_Column UI_Padding(ui_em(0.3f, 1))
          {
            ui_set_next_background_color(rgba);
            ui_set_next_corner_radius_00(ui_top_font_size()/4.f);
            ui_set_next_corner_radius_01(ui_top_font_size()/4.f);
            ui_set_next_corner_radius_10(ui_top_font_size()/4.f);
            ui_set_next_corner_radius_11(ui_top_font_size()/4.f);
            UI_Box *color_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, "###color_box");
            UI_Signal color_sig = ui_signal_from_box(color_box);
            if(ui_hovering(color_sig))
            {
              ui_do_color_tooltip_hsva(hsva);
            }
          }
          ui_spacer(ui_em(0.3f, 1));
        }
        UI_Signal color_row_sig = ui_signal_from_box(color_row);
        if(ui_clicked(color_row_sig) || ui_right_clicked(color_row_sig))
        {
          ui_ctx_menu_open(color_ctx_menu_keys[color], color_row->key, v2f32(0, color_row->rect.y1-color_row->rect.y0));
          sv->color_ctx_menu_color = color;
          sv->color_ctx_menu_color_hsva = v4f32(hsv.x, hsv.y, hsv.z, rgba.w);
          DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
        }
        if(ui_hovering(color_row_sig)) UI_Tooltip
        {
          ui_label(df_g_theme_color_display_string_table[color]);
        }
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}
