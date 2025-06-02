// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Baking Stage Tasks

//- rjf: bake string map building

#define rdim_make_string_map_if_needed() do {if(in->maps[thread_idx] == 0) ProfScope("make map") {in->maps[thread_idx] = rdim_bake_string_map_loose_make(arena, in->top);}} while(0)

ASYNC_WORK_DEF(rdim_bake_src_files_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeSrcFilesStringsIn *in = (RDIM_BakeSrcFilesStringsIn *)input;
  rdim_make_string_map_if_needed();
  ProfScope("bake src file strings") rdim_bake_string_map_loose_push_src_files(arena, in->top, in->maps[thread_idx], in->list);
  ProfEnd();
  return 0;
}

ASYNC_WORK_DEF(rdim_bake_units_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeUnitsStringsIn *in = (RDIM_BakeUnitsStringsIn *)input;
  rdim_make_string_map_if_needed();
  ProfScope("bake unit strings") rdim_bake_string_map_loose_push_units(arena, in->top, in->maps[thread_idx], in->list);
  ProfEnd();
  return 0;
}

ASYNC_WORK_DEF(rdim_bake_types_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeTypesStringsIn *in = (RDIM_BakeTypesStringsIn *)input;
  rdim_make_string_map_if_needed();
  ProfScope("bake type strings")
  {
    for(RDIM_BakeTypesStringsInNode *n = in->first; n != 0; n = n->next)
    {
      rdim_bake_string_map_loose_push_type_slice(arena, in->top, in->maps[thread_idx], n->v, n->count);
    }
  }
  ProfEnd();
  return 0;
}

ASYNC_WORK_DEF(rdim_bake_udts_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeUDTsStringsIn *in = (RDIM_BakeUDTsStringsIn *)input;
  rdim_make_string_map_if_needed();
  ProfScope("bake udt strings")
  {
    for(RDIM_BakeUDTsStringsInNode *n = in->first; n != 0; n = n->next)
    {
      rdim_bake_string_map_loose_push_udt_slice(arena, in->top, in->maps[thread_idx], n->v, n->count);
    }
  }
  ProfEnd();
  return 0;
}

ASYNC_WORK_DEF(rdim_bake_symbols_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeSymbolsStringsIn *in = (RDIM_BakeSymbolsStringsIn *)input;
  rdim_make_string_map_if_needed();
  ProfScope("bake symbol strings")
  {
    for(RDIM_BakeSymbolsStringsInNode *n = in->first; n != 0; n = n->next)
    {
      rdim_bake_string_map_loose_push_symbol_slice(arena, in->top, in->maps[thread_idx], n->v, n->count);
    }
  }
  ProfEnd();
  return 0;
}

ASYNC_WORK_DEF(rdim_bake_inline_site_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeInlineSiteStringsIn *in = input;
  rdim_make_string_map_if_needed();
  ProfScope("bake inline site strings")
  {
    for(RDIM_BakeInlineSiteStringsInNode *n = in->first; n != 0; n = n->next)
    {
      rdim_bake_string_map_loose_push_inline_site_slice(arena, in->top, in->maps[thread_idx], n->v, n->count);
    }
  }
  ProfEnd();
  return 0;
}

ASYNC_WORK_DEF(rdim_bake_scopes_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeScopesStringsIn *in = (RDIM_BakeScopesStringsIn *)input;
  rdim_make_string_map_if_needed();
  ProfScope("bake scope strings")
  {
    for(RDIM_BakeScopesStringsInNode *n = in->first; n != 0; n = n->next)
    {
      rdim_bake_string_map_loose_push_scope_slice(arena, in->top, in->maps[thread_idx], n->v, n->count);
    }
  }
  ProfEnd();
  return 0;
}

ASYNC_WORK_DEF(rdim_bake_line_tables_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeLineTablesIn *in = (RDIM_BakeLineTablesIn *)input;
  RDIM_LineTableBakeResult *out = push_array(arena, RDIM_LineTableBakeResult, 1);
  ProfScope("bake line tables") *out = rdim_bake_line_tables(arena, in->line_tables);
  ProfEnd();
  return out;
}

#undef rdim_make_string_map_if_needed

//- rjf: bake string map joining

ASYNC_WORK_DEF(rdim_bake_string_map_join_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_JoinBakeStringMapSlotsIn *in = (RDIM_JoinBakeStringMapSlotsIn *)input;
  ProfScope("join bake string maps")
  {
    for(U64 src_map_idx = 0; src_map_idx < in->src_maps_count; src_map_idx += 1)
    {
      for(U64 slot_idx = in->slot_idx_range.min; slot_idx < in->slot_idx_range.max; slot_idx += 1)
      {
        B32 src_slots_good = (in->src_maps[src_map_idx] != 0 && in->src_maps[src_map_idx]->slots != 0);
        B32 dst_slot_is_zero = (in->dst_map->slots[slot_idx] == 0);
        if(src_slots_good && dst_slot_is_zero)
        {
          in->dst_map->slots[slot_idx] = in->src_maps[src_map_idx]->slots[slot_idx];
        }
        else if(src_slots_good && in->src_maps[src_map_idx]->slots[slot_idx] != 0)
        {
          rdim_bake_string_chunk_list_concat_in_place(in->dst_map->slots[slot_idx], in->src_maps[src_map_idx]->slots[slot_idx]);
        }
      }
    }
  }
  ProfEnd();
  return 0;
}

//- rjf: bake string map sorting

ASYNC_WORK_DEF(rdim_bake_string_map_sort_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_SortBakeStringMapSlotsIn *in = (RDIM_SortBakeStringMapSlotsIn *)input;
  ProfScope("sort bake string chunk list map range")
  {
    for(U64 slot_idx = in->slot_idx;
        slot_idx < in->slot_idx+in->slot_count;
        slot_idx += 1)
    {
      if(in->src_map->slots[slot_idx] != 0)
      {
        if(in->src_map->slots[slot_idx]->total_count > 1)
        {
          in->dst_map->slots[slot_idx] = push_array(arena, RDIM_BakeStringChunkList, 1);
          *in->dst_map->slots[slot_idx] = rdim_bake_string_chunk_list_sorted_from_unsorted(arena, in->src_map->slots[slot_idx]);
        }
        else
        {
          in->dst_map->slots[slot_idx] = in->src_map->slots[slot_idx];
        }
      }
    }
  }
  ProfEnd();
  return 0;
}

//- rjf: pass 1: interner/deduper map builds

ASYNC_WORK_DEF(rdim_build_bake_name_map_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BuildBakeNameMapIn *in = (RDIM_BuildBakeNameMapIn *)input;
  RDIM_BakeNameMap *name_map = 0;
  ProfScope("build name map %i", in->k) name_map = rdim_bake_name_map_from_kind_params(arena, in->k, in->type_indices, in->params);
  ProfEnd();
  return name_map;
}

//- rjf: pass 2: string-map-dependent debug info stream builds

ASYNC_WORK_DEF(rdim_bake_units_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeUnitsIn *in = (RDIM_BakeUnitsIn *)input;
  RDIM_UnitBakeResult *out = push_array(arena, RDIM_UnitBakeResult, 1);
  ProfScope("bake units") *out = rdim_bake_units(arena, in->strings, in->path_tree, in->units);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_unit_vmap_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeUnitVMapIn *in = (RDIM_BakeUnitVMapIn *)input;
  RDIM_UnitVMapBakeResult *out = push_array(arena, RDIM_UnitVMapBakeResult, 1);
  ProfScope("bake unit vmap") *out = rdim_bake_unit_vmap(arena, in->units);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_src_files_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeSrcFilesIn *in = (RDIM_BakeSrcFilesIn *)input;
  RDIM_SrcFileBakeResult *out = push_array(arena, RDIM_SrcFileBakeResult, 1);
  ProfScope("bake src files") *out = rdim_bake_src_files(arena, in->strings, in->path_tree, in->src_files);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_udts_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeUDTsIn *in = (RDIM_BakeUDTsIn *)input;
  RDIM_UDTBakeResult *out = push_array(arena, RDIM_UDTBakeResult, 1);
  ProfScope("bake udts") *out = rdim_bake_udts(arena, in->strings, in->type_indices, in->udts);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_global_variables_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeGlobalVariablesIn *in = (RDIM_BakeGlobalVariablesIn *)input;
  RDIM_GlobalVariableBakeResult *out = push_array(arena, RDIM_GlobalVariableBakeResult, 1);
  ProfScope("bake global variables") *out = rdim_bake_global_variables(arena, in->strings, in->type_indices, in->global_variables);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_global_vmap_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeGlobalVMapIn *in = (RDIM_BakeGlobalVMapIn *)input;
  RDIM_GlobalVMapBakeResult *out = push_array(arena, RDIM_GlobalVMapBakeResult, 1);
  ProfScope("bake global vmap") *out = rdim_bake_global_vmap(arena, in->global_variables);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_thread_variables_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeThreadVariablesIn *in = (RDIM_BakeThreadVariablesIn *)input;
  RDIM_ThreadVariableBakeResult *out = push_array(arena, RDIM_ThreadVariableBakeResult, 1);
  ProfScope("bake thread variables") *out = rdim_bake_thread_variables(arena, in->strings, in->type_indices, in->thread_variables);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_procedures_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeProceduresIn *in = (RDIM_BakeProceduresIn *)input;
  RDIM_ProcedureBakeResult *out = push_array(arena, RDIM_ProcedureBakeResult, 1);
  ProfScope("bake procedures") *out = rdim_bake_procedures(arena, in->strings, in->type_indices, in->location_blocks, in->location_data_blobs, in->procedures);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_scopes_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeScopesIn *in = (RDIM_BakeScopesIn *)input;
  RDIM_ScopeBakeResult *out = push_array(arena, RDIM_ScopeBakeResult, 1);
  ProfScope("bake scopes") *out = rdim_bake_scopes(arena, in->strings, in->type_indices, in->location_blocks, in->location_data_blobs, in->scopes);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_scope_vmap_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeScopeVMapIn *in = (RDIM_BakeScopeVMapIn *)input;
  RDIM_ScopeVMapBakeResult *out = push_array(arena, RDIM_ScopeVMapBakeResult, 1);
  ProfScope("bake scope vmap") *out = rdim_bake_scope_vmap(arena, in->scopes);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_inline_sites_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeInlineSitesIn *in = (RDIM_BakeInlineSitesIn *)input;
  RDIM_InlineSiteBakeResult *out = push_array(arena, RDIM_InlineSiteBakeResult, 1);
  ProfScope("bake inline sites") *out = rdim_bake_inline_sites(arena, in->strings, in->type_indices, in->inline_sites);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_file_paths_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeFilePathsIn *in = (RDIM_BakeFilePathsIn *)input;
  RDIM_FilePathBakeResult *out = push_array(arena, RDIM_FilePathBakeResult, 1);
  ProfScope("bake file paths") *out = rdim_bake_file_paths(arena, in->strings, in->path_tree);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_strings_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeStringsIn *in = (RDIM_BakeStringsIn *)input;
  RDIM_StringBakeResult *out = push_array(arena, RDIM_StringBakeResult, 1);
  ProfScope("bake strings") *out = rdim_bake_strings(arena, in->strings);
  ProfEnd();
  return out;
}

//- rjf: pass 3: idx-run-map-dependent debug info stream builds

ASYNC_WORK_DEF(rdim_bake_type_nodes_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeTypeNodesIn *in = (RDIM_BakeTypeNodesIn *)input;
  RDIM_TypeNodeBakeResult *out = push_array(arena, RDIM_TypeNodeBakeResult, 1);
  ProfScope("bake type nodes") *out = rdim_bake_types(arena, in->strings, in->idx_runs, in->type_indices, in->types);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_name_map_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeNameMapIn *in = (RDIM_BakeNameMapIn *)input;
  RDIM_NameMapBakeResult *out = push_array(arena, RDIM_NameMapBakeResult, 1);
  ProfScope("bake name map %i", in->kind) *out = rdim_bake_name_map(arena, in->strings, in->idx_runs, in->map);
  ProfEnd();
  return out;
}

ASYNC_WORK_DEF(rdim_bake_idx_runs_work)
{
  ProfBeginFunction();
  Arena *arena = rdim_help_state->work_thread_arenas[thread_idx];
  RDIM_BakeIdxRunsIn *in = (RDIM_BakeIdxRunsIn *)input;
  RDIM_IndexRunBakeResult *out = push_array(arena, RDIM_IndexRunBakeResult, 1);
  ProfScope("bake idx runs") *out = rdim_bake_index_runs(arena, in->idx_runs);
  ProfEnd();
  return out;
}

internal RDIM_HelpState *
rdim_help_init(void)
{
  Arena *arena = arena_alloc();
  RDIM_HelpState *state           = push_array(arena, RDIM_HelpState, 1);
  state->arena                    = arena;
  state->work_thread_arenas_count = async_thread_count();
  state->work_thread_arenas       = push_array(arena, Arena *, state->work_thread_arenas_count);
  for EachIndex(idx, state->work_thread_arenas_count)
  {
    state->work_thread_arenas[idx] = arena_alloc();
  }
  return state;
}

internal RDIM_BakeResults
rdim_bake(RDIM_HelpState *state, RDIM_BakeParams *in_params)
{
  Temp scratch = scratch_begin(0,0);
  RDIM_BakeResults out = {0};
  
  rdim_help_state = state;
  
  ////////////////////////////////
  // compute type indices
  
  RDI_U64 *type_indices = rdim_make_type_indices(scratch.arena, &in_params->types);
  
  //////////////////////////////
  //- rjf: kick off line tables baking
  //
  ASYNC_Task *bake_line_tables_task = 0;
  {
    RDIM_BakeLineTablesIn *in = push_array(scratch.arena, RDIM_BakeLineTablesIn, 1);
    in->line_tables = &in_params->line_tables;
    bake_line_tables_task = async_task_launch(scratch.arena, rdim_bake_line_tables_work, .input = in);
  }
  
  //////////////////////////////
  //- rjf: build interned path tree
  //
  RDIM_BakePathTree *path_tree = 0;
  ProfScope("build interned path tree")
  {
    path_tree = rdim_bake_path_tree_from_params(state->arena, in_params);
  }
  
  //////////////////////////////
  //- rjf: kick off string map building tasks
  //
  RDIM_BakeStringMapTopology bake_string_map_topology = {(64 +
                                                          in_params->procedures.total_count*1 +
                                                          in_params->global_variables.total_count*1 +
                                                          in_params->thread_variables.total_count*1 +
                                                          in_params->constants.total_count*1 +
                                                          in_params->types.total_count/2)};
  RDIM_BakeStringMapLoose **bake_string_maps__in_progress = push_array(scratch.arena, RDIM_BakeStringMapLoose *, async_thread_count());
  ASYNC_TaskList bake_string_map_build_tasks = {0};
  {
    // rjf: src files
    ProfScope("kick off src files string map build task")
    {
      RDIM_BakeSrcFilesStringsIn *in = push_array(scratch.arena, RDIM_BakeSrcFilesStringsIn, 1);
      in->top = &bake_string_map_topology;
      in->maps = bake_string_maps__in_progress;
      in->list = &in_params->src_files;
      async_task_list_push(scratch.arena, &bake_string_map_build_tasks, async_task_launch(scratch.arena, rdim_bake_src_files_strings_work, .input = in));
    }
    
    // rjf: units
    ProfScope("kick off units string map build task")
    {
      RDIM_BakeUnitsStringsIn *in = push_array(scratch.arena, RDIM_BakeUnitsStringsIn, 1);
      in->top = &bake_string_map_topology;
      in->maps = bake_string_maps__in_progress;
      in->list = &in_params->units;
      async_task_list_push(scratch.arena, &bake_string_map_build_tasks, async_task_launch(scratch.arena, rdim_bake_units_strings_work, .input = in));
    }
    
    // rjf: types
    ProfScope("kick off types string map build tasks")
    {
      U64 items_per_task = 4096;
      U64 num_tasks = (in_params->types.total_count+items_per_task-1)/items_per_task;
      RDIM_TypeChunkNode *chunk = in_params->types.first;
      U64 chunk_off = 0;
      for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
      {
        RDIM_BakeTypesStringsIn *in = push_array(scratch.arena, RDIM_BakeTypesStringsIn, 1);
        in->top = &bake_string_map_topology;
        in->maps = bake_string_maps__in_progress;
        U64 items_left = items_per_task;
        for(;chunk != 0 && items_left > 0;)
        {
          U64 items_in_this_chunk = Min(items_per_task, chunk->count-chunk_off);
          RDIM_BakeTypesStringsInNode *n = push_array(scratch.arena, RDIM_BakeTypesStringsInNode, 1);
          SLLQueuePush(in->first, in->last, n);
          n->v = chunk->v + chunk_off;
          n->count = items_in_this_chunk;
          chunk_off += items_in_this_chunk;
          items_left -= items_in_this_chunk;
          if(chunk_off >= chunk->count)
          {
            chunk = chunk->next;
            chunk_off = 0;
          }
        }
        async_task_list_push(scratch.arena, &bake_string_map_build_tasks, async_task_launch(scratch.arena, rdim_bake_types_strings_work, .input = in));
      }
    }
    
    // rjf: UDTs
    ProfScope("kick off udts string map build tasks")
    {
      U64 items_per_task = 4096;
      U64 num_tasks = (in_params->udts.total_count+items_per_task-1)/items_per_task;
      RDIM_UDTChunkNode *chunk = in_params->udts.first;
      U64 chunk_off = 0;
      for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
      {
        RDIM_BakeUDTsStringsIn *in = push_array(scratch.arena, RDIM_BakeUDTsStringsIn, 1);
        in->top = &bake_string_map_topology;
        in->maps = bake_string_maps__in_progress;
        U64 items_left = items_per_task;
        for(;chunk != 0 && items_left > 0;)
        {
          U64 items_in_this_chunk = Min(items_per_task, chunk->count-chunk_off);
          RDIM_BakeUDTsStringsInNode *n = push_array(scratch.arena, RDIM_BakeUDTsStringsInNode, 1);
          SLLQueuePush(in->first, in->last, n);
          n->v = chunk->v + chunk_off;
          n->count = items_in_this_chunk;
          chunk_off += items_in_this_chunk;
          items_left -= items_in_this_chunk;
          if(chunk_off >= chunk->count)
          {
            chunk = chunk->next;
            chunk_off = 0;
          }
        }
        async_task_list_push(scratch.arena, &bake_string_map_build_tasks, async_task_launch(scratch.arena, rdim_bake_udts_strings_work, .input = in));
      }
    }
    
    // rjf: symbols
    ProfScope("kick off symbols string map build tasks")
    {
      RDIM_SymbolChunkList *symbol_lists[] =
      {
        &in_params->global_variables,
        &in_params->thread_variables,
        &in_params->constants,
        &in_params->procedures,
      };
      for(U64 list_idx = 0; list_idx < ArrayCount(symbol_lists); list_idx += 1)
      {
        U64 items_per_task = 4096;
        U64 num_tasks = (symbol_lists[list_idx]->total_count+items_per_task-1)/items_per_task;
        RDIM_SymbolChunkNode *chunk = symbol_lists[list_idx]->first;
        U64 chunk_off = 0;
        for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
        {
          RDIM_BakeSymbolsStringsIn *in = push_array(scratch.arena, RDIM_BakeSymbolsStringsIn, 1);
          in->top = &bake_string_map_topology;
          in->maps = bake_string_maps__in_progress;
          U64 items_left = items_per_task;
          for(;chunk != 0 && items_left > 0;)
          {
            U64 items_in_this_chunk = Min(items_per_task, chunk->count-chunk_off);
            RDIM_BakeSymbolsStringsInNode *n = push_array(scratch.arena, RDIM_BakeSymbolsStringsInNode, 1);
            SLLQueuePush(in->first, in->last, n);
            n->v = chunk->v + chunk_off;
            n->count = items_in_this_chunk;
            chunk_off += items_in_this_chunk;
            items_left -= items_in_this_chunk;
            if(chunk_off >= chunk->count)
            {
              chunk = chunk->next;
              chunk_off = 0;
            }
          }
          async_task_list_push(scratch.arena, &bake_string_map_build_tasks, async_task_launch(scratch.arena, rdim_bake_symbols_strings_work, .input = in));
        }
      }
    }
    
    ProfScope("kick off inline site string map build task")
    {
      U64 items_per_task = 4096;
      U64 num_tasks = CeilIntegerDiv(in_params->inline_sites.total_count, items_per_task);
      RDIM_InlineSiteChunkNode *chunk = in_params->inline_sites.first;
      U64 chunk_off = 0;
      for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
      {
        RDIM_BakeInlineSiteStringsIn *in = push_array(scratch.arena, RDIM_BakeInlineSiteStringsIn, 1);
        in->top = &bake_string_map_topology;
        in->maps = bake_string_maps__in_progress;
        U64 items_left = items_per_task;
        for(;chunk != 0 && items_left > 0;)
        {
          U64 items_in_this_chunk = Min(items_per_task, chunk->count-chunk_off);
          RDIM_BakeInlineSiteStringsInNode *n = push_array(scratch.arena, RDIM_BakeInlineSiteStringsInNode, 1);
          SLLQueuePush(in->first, in->last, n);
          n->v = chunk->v + chunk_off;
          n->count = items_in_this_chunk;
          chunk_off += items_in_this_chunk;
          items_left -= items_in_this_chunk;
          if(chunk_off >= chunk->count)
          {
            chunk = chunk->next;
            chunk_off = 0;
          }
        }
        async_task_list_push(scratch.arena, &bake_string_map_build_tasks, async_task_launch(scratch.arena, rdim_bake_inline_site_strings_work, .input = in));
      }
    }
    
    // rjf: scope chunks
    ProfScope("kick off scope chunks string map build tasks")
    {
      U64 items_per_task = 4096;
      U64 num_tasks = (in_params->scopes.total_count+items_per_task-1)/items_per_task;
      RDIM_ScopeChunkNode *chunk = in_params->scopes.first;
      U64 chunk_off = 0;
      for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
      {
        RDIM_BakeScopesStringsIn *in = push_array(scratch.arena, RDIM_BakeScopesStringsIn, 1);
        in->top = &bake_string_map_topology;
        in->maps = bake_string_maps__in_progress;
        U64 items_left = items_per_task;
        for(;chunk != 0 && items_left > 0;)
        {
          U64 items_in_this_chunk = Min(items_per_task, chunk->count-chunk_off);
          RDIM_BakeScopesStringsInNode *n = push_array(scratch.arena, RDIM_BakeScopesStringsInNode, 1);
          SLLQueuePush(in->first, in->last, n);
          n->v = chunk->v + chunk_off;
          n->count = items_in_this_chunk;
          chunk_off += items_in_this_chunk;
          items_left -= items_in_this_chunk;
          if(chunk_off >= chunk->count)
          {
            chunk = chunk->next;
            chunk_off = 0;
          }
        }
        async_task_list_push(scratch.arena, &bake_string_map_build_tasks, async_task_launch(scratch.arena, rdim_bake_scopes_strings_work, .input = in));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: kick off name map building tasks
  //
  RDIM_BuildBakeNameMapIn build_bake_name_map_in[RDI_NameMapKind_COUNT] = {0};
  ASYNC_Task *build_bake_name_map_task[RDI_NameMapKind_COUNT] = {0};
  for(RDI_NameMapKind k = (RDI_NameMapKind)(RDI_NameMapKind_NULL+1);
      k < RDI_NameMapKind_COUNT;
      k = (RDI_NameMapKind)(k+1))
  {
    build_bake_name_map_in[k].k = k;
    build_bake_name_map_in[k].type_indices = type_indices;
    build_bake_name_map_in[k].params = in_params;
    build_bake_name_map_task[k] = async_task_launch(scratch.arena, rdim_build_bake_name_map_work, .input = &build_bake_name_map_in[k]);
  }
  
  //////////////////////////////
  //- rjf: join string map building tasks
  //
  ProfScope("join string map building tasks")
  {
    for(ASYNC_TaskNode *n = bake_string_map_build_tasks.first; n != 0; n = n->next)
    {
      async_task_join(n->v);
    }
  }
  
  //////////////////////////////
  //- rjf: produce joined string map
  //
  RDIM_BakeStringMapLoose *unsorted_bake_string_map = rdim_bake_string_map_loose_make(state->arena, &bake_string_map_topology);
  ProfScope("produce joined string map")
  {
    U64 slots_per_task = 16384;
    U64 num_tasks = (bake_string_map_topology.slots_count+slots_per_task-1)/slots_per_task;
    ASYNC_Task **tasks = push_array(scratch.arena, ASYNC_Task *, num_tasks);
    
    // rjf: kickoff tasks
    for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
    {
      RDIM_JoinBakeStringMapSlotsIn *in = push_array(scratch.arena, RDIM_JoinBakeStringMapSlotsIn, 1);
      in->top = &bake_string_map_topology;
      in->src_maps = bake_string_maps__in_progress;
      in->src_maps_count = async_thread_count();
      in->dst_map = unsorted_bake_string_map;
      in->slot_idx_range = r1u64(task_idx*slots_per_task, task_idx*slots_per_task + slots_per_task);
      in->slot_idx_range.max = Min(in->slot_idx_range.max, in->top->slots_count);
      tasks[task_idx] = async_task_launch(scratch.arena, rdim_bake_string_map_join_work, .input = in);
    }
    
    // rjf: join tasks
    for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
    {
      async_task_join(tasks[task_idx]);
    }
    
    // rjf: insert small top-level stuff
    rdim_bake_string_map_loose_push_top_level_info(state->arena, &bake_string_map_topology, unsorted_bake_string_map, &in_params->top_level_info);
    rdim_bake_string_map_loose_push_binary_sections(state->arena, &bake_string_map_topology, unsorted_bake_string_map, &in_params->binary_sections);
    rdim_bake_string_map_loose_push_path_tree(state->arena, &bake_string_map_topology, unsorted_bake_string_map, path_tree);
  }
  
  //////////////////////////////
  //- rjf: kick off string map sorting tasks
  //
  ASYNC_TaskList sort_bake_string_map_tasks = {0};
  RDIM_BakeStringMapLoose *sorted_bake_string_map__in_progress = rdim_bake_string_map_loose_make(state->arena, &bake_string_map_topology);
  {
    U64 slots_per_task = 4096;
    U64 num_tasks = (bake_string_map_topology.slots_count+slots_per_task-1)/slots_per_task;
    for(U64 task_idx = 0; task_idx < num_tasks; task_idx += 1)
    {
      RDIM_SortBakeStringMapSlotsIn *in = push_array(scratch.arena, RDIM_SortBakeStringMapSlotsIn, 1);
      {
        in->top = &bake_string_map_topology;
        in->src_map = unsorted_bake_string_map;
        in->dst_map = sorted_bake_string_map__in_progress;
        in->slot_idx = task_idx*slots_per_task;
        in->slot_count = slots_per_task;
        if(in->slot_idx+in->slot_count > bake_string_map_topology.slots_count)
        {
          in->slot_count = bake_string_map_topology.slots_count - in->slot_idx;
        }
      }
      async_task_list_push(scratch.arena, &sort_bake_string_map_tasks, async_task_launch(scratch.arena, rdim_bake_string_map_sort_work, .input = in));
    }
  }
  
  //////////////////////////////
  //- rjf: join string map sorting tasks
  //
  ProfScope("join string map sorting tasks")
  {
    for(ASYNC_TaskNode *n = sort_bake_string_map_tasks.first; n != 0; n = n->next)
    {
      async_task_join(n->v);
    }
  }
  RDIM_BakeStringMapLoose *sorted_bake_string_map = sorted_bake_string_map__in_progress;
  
  //////////////////////////////
  //- rjf: build finalized string map
  //
  ProfBegin("build finalized string map base indices");
  RDIM_BakeStringMapBaseIndices bake_string_map_base_idxes = rdim_bake_string_map_base_indices_from_map_loose(state->arena, &bake_string_map_topology, sorted_bake_string_map);
  ProfEnd();
  ProfBegin("build finalized string map");
  RDIM_BakeStringMapTight bake_strings = rdim_bake_string_map_tight_from_loose(state->arena, &bake_string_map_topology, &bake_string_map_base_idxes, sorted_bake_string_map);
  ProfEnd();
  
  //////////////////////////////
  //- rjf: kick off pass 2 tasks
  //
  RDIM_BakeUnitsIn bake_units_top_level_in = {&bake_strings, path_tree, &in_params->units};
  ASYNC_Task *bake_units_task = async_task_launch(scratch.arena, rdim_bake_units_work, .input = &bake_units_top_level_in);
  RDIM_BakeUnitVMapIn bake_unit_vmap_in = {&in_params->units};
  ASYNC_Task *bake_unit_vmap_task = async_task_launch(scratch.arena, rdim_bake_unit_vmap_work, .input = &bake_unit_vmap_in);
  RDIM_BakeSrcFilesIn bake_src_files_in = {&bake_strings, path_tree, &in_params->src_files};
  ASYNC_Task *bake_src_files_task = async_task_launch(scratch.arena, rdim_bake_src_files_work, .input = &bake_src_files_in);
  RDIM_BakeUDTsIn bake_udts_in = {&bake_strings, &in_params->udts, type_indices};
  ASYNC_Task *bake_udts_task = async_task_launch(scratch.arena, rdim_bake_udts_work, .input = &bake_udts_in);
  RDIM_BakeGlobalVMapIn bake_global_vmap_in = {&in_params->global_variables};
  ASYNC_Task *bake_global_vmap_task = async_task_launch(scratch.arena, rdim_bake_global_vmap_work, .input = &bake_global_vmap_in);
  RDIM_BakeScopeVMapIn bake_scope_vmap_in = {&in_params->scopes};
  ASYNC_Task *bake_scope_vmap_task = async_task_launch(scratch.arena, rdim_bake_scope_vmap_work, .input = &bake_scope_vmap_in);
  RDIM_BakeInlineSitesIn bake_inline_sites_in = {&bake_strings, &in_params->inline_sites, type_indices};
  ASYNC_Task *bake_inline_sites_task = async_task_launch(scratch.arena, rdim_bake_inline_sites_work, .input = &bake_inline_sites_in);
  RDIM_BakeFilePathsIn bake_file_paths_in = {&bake_strings, path_tree};
  ASYNC_Task *bake_file_paths_task = async_task_launch(scratch.arena, rdim_bake_file_paths_work, .input = &bake_file_paths_in);
  RDIM_BakeStringsIn bake_strings_in = {&bake_strings};
  ASYNC_Task *bake_strings_task = async_task_launch(scratch.arena, rdim_bake_strings_work, .input = &bake_strings_in);
  
  RDIM_String8List location_blocks     = {0};
  RDIM_String8List location_data_blobs = {0};
  
  // reserve null location block for opl
  rdim_location_block_chunk_list_push_array(state->arena, &location_blocks, 1);
  
  // TODO: export location instead of VOFF
  RDIM_BakeThreadVariablesIn bake_thread_variables_in = {&bake_strings, &in_params->thread_variables, type_indices};
  ASYNC_Task *bake_thread_variables_task = async_task_launch(scratch.arena, rdim_bake_thread_variables_work, .input = &bake_thread_variables_in);
  ProfScope("thread variables") out.thread_variables = *async_task_join_struct(bake_thread_variables_task, RDIM_ThreadVariableBakeResult);
  
  // TODO: export location instead of VOFF
  RDIM_BakeGlobalVariablesIn bake_global_variables_in = {&bake_strings, &in_params->global_variables, type_indices};
  ASYNC_Task *bake_global_variables_task = async_task_launch(scratch.arena, rdim_bake_global_variables_work, .input = &bake_global_variables_in);
  ProfScope("global variables") out.global_variables = *async_task_join_struct(bake_global_variables_task, RDIM_GlobalVariableBakeResult);
  
  RDIM_BakeScopesIn bake_scopes_in = {&bake_strings, &in_params->scopes, type_indices, &location_blocks, &location_data_blobs};
  ASYNC_Task *bake_scopes_task = async_task_launch(scratch.arena, rdim_bake_scopes_work, .input = &bake_scopes_in);
  ProfScope("scopes") out.scopes = *async_task_join_struct(bake_scopes_task, RDIM_ScopeBakeResult);
  
  RDIM_BakeProceduresIn bake_procedures_in = {&bake_strings, &in_params->procedures, type_indices, &location_blocks, &location_data_blobs};
  ASYNC_Task *bake_procedures_task = async_task_launch(scratch.arena, rdim_bake_procedures_work, .input = &bake_procedures_in);
  ProfScope("procedures") out.procedures = *async_task_join_struct(bake_procedures_task, RDIM_ProcedureBakeResult);
  
  //////////////////////////////
  //- rjf: join name map building tasks
  //
  RDIM_BakeNameMap *name_maps[RDI_NameMapKind_COUNT] = {0};
  ProfScope("join name map building tasks")
  {
    for(RDI_NameMapKind k = (RDI_NameMapKind)(RDI_NameMapKind_NULL+1);
        k < RDI_NameMapKind_COUNT;
        k = (RDI_NameMapKind)(k+1))
    {
      name_maps[k] = async_task_join_struct(build_bake_name_map_task[k], RDIM_BakeNameMap);
    }
  }
  
  //////////////////////////////
  //- rjf: build interned idx run map
  //
  RDIM_BakeIdxRunMap *idx_runs = 0;
  ProfScope("build interned idx run map")
  {
    idx_runs = rdim_bake_idx_run_map_from_params(state->arena, name_maps, type_indices, in_params);
  }
  
  //////////////////////////////
  //- rjf: do small top-level bakes
  //
  ProfScope("top level info")              out.top_level_info      = rdim_bake_top_level_info(state->arena, &bake_strings, &in_params->top_level_info);
  ProfScope("binary sections")             out.binary_sections     = rdim_bake_binary_sections(state->arena, &bake_strings, &in_params->binary_sections);
  ProfScope("top level name maps section") out.top_level_name_maps = rdim_bake_name_maps_top_level(state->arena, &bake_strings, idx_runs, name_maps);
  
  //////////////////////////////
  //- rjf: kick off pass 3 tasks
  //
  RDIM_BakeTypeNodesIn bake_type_nodes_in = {&bake_strings, idx_runs, &in_params->types, type_indices};
  ASYNC_Task *bake_type_nodes_task = async_task_launch(scratch.arena, rdim_bake_type_nodes_work, .input = &bake_type_nodes_in);
  ASYNC_Task *bake_name_maps_tasks[RDI_NameMapKind_COUNT] = {0};
  {
    for EachNonZeroEnumVal(RDI_NameMapKind, k)
    {
      if(name_maps[k] == 0 || name_maps[k]->name_count == 0)
      {
        continue;
      }
      RDIM_BakeNameMapIn *in = push_array(scratch.arena, RDIM_BakeNameMapIn, 1);
      in->strings       = &bake_strings;
      in->idx_runs      = idx_runs;
      in->map           = name_maps[k];
      in->kind          = k;
      bake_name_maps_tasks[k] = async_task_launch(scratch.arena, rdim_bake_name_map_work, .input = in);
    }
  }
  RDIM_BakeIdxRunsIn bake_idx_runs_in = {idx_runs};
  ASYNC_Task *bake_idx_runs_task = async_task_launch(scratch.arena, rdim_bake_idx_runs_work, .input = &bake_idx_runs_in);
  
  //////////////////////////////
  //- rjf: join remaining completed bakes
  //
  ProfScope("top-level units info") out.units            = *async_task_join_struct(bake_units_task,            RDIM_UnitBakeResult);
  ProfScope("unit vmap")            out.unit_vmap        = *async_task_join_struct(bake_unit_vmap_task,        RDIM_UnitVMapBakeResult);
  ProfScope("source files")         out.src_files        = *async_task_join_struct(bake_src_files_task,        RDIM_SrcFileBakeResult);
  ProfScope("UDTs")                 out.udts             = *async_task_join_struct(bake_udts_task,             RDIM_UDTBakeResult);
  ProfScope("global vmap")          out.global_vmap      = *async_task_join_struct(bake_global_vmap_task,      RDIM_GlobalVMapBakeResult);
  ProfScope("scope vmap")           out.scope_vmap       = *async_task_join_struct(bake_scope_vmap_task,       RDIM_ScopeVMapBakeResult);
  ProfScope("inline sites")         out.inline_sites     = *async_task_join_struct(bake_inline_sites_task,     RDIM_InlineSiteBakeResult);
  ProfScope("file paths")           out.file_paths       = *async_task_join_struct(bake_file_paths_task,       RDIM_FilePathBakeResult);
  ProfScope("strings")              out.strings          = *async_task_join_struct(bake_strings_task,          RDIM_StringBakeResult);
  ProfScope("type nodes")           out.type_nodes       = *async_task_join_struct(bake_type_nodes_task,       RDIM_TypeNodeBakeResult);
  ProfScope("idx runs")             out.idx_runs         = *async_task_join_struct(bake_idx_runs_task,         RDIM_IndexRunBakeResult);
  ProfScope("line tables")          out.line_tables      = *async_task_join_struct(bake_line_tables_task,      RDIM_LineTableBakeResult);
  
  //////////////////////////////
  //- rjf: join individual name map bakes
  //
  RDIM_NameMapBakeResult name_map_bakes[RDI_NameMapKind_COUNT] = {0};
  ProfScope("name maps")
  {
    for EachNonZeroEnumVal(RDI_NameMapKind, k)
    {
      RDIM_NameMapBakeResult *bake = async_task_join_struct(bake_name_maps_tasks[k], RDIM_NameMapBakeResult);
      if(bake != 0)
      {
        name_map_bakes[k] = *bake;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: join all individual name map bakes
  //
  ProfScope("join all name map bakes into final name map bake")
  {
    out.name_maps = rdim_name_map_bake_results_combine(state->arena, name_map_bakes, ArrayCount(name_map_bakes));
  }
  
  
  ////////////////////////////////
  
  out.location_blocks = rdim_str8_list_join(state->arena, &location_blocks,     rdim_str8(0,0));
  out.location_data   = rdim_str8_list_join(state->arena, &location_data_blobs, rdim_str8(0,0));
  
  rdim_help_state = 0;
  
  scratch_end(scratch);
  return out;
}

internal RDIM_SerializedSectionBundle
rdim_compress(Arena *arena, RDIM_SerializedSectionBundle *in)
{
  RDIM_SerializedSectionBundle out = {0};
  
  //- rjf: set up compression context
  rr_lzb_simple_context ctx = {0};
  ctx.m_tableSizeBits = 14;
  ctx.m_hashTable = push_array(arena, U16, 1<<ctx.m_tableSizeBits);
  
  //- rjf: compress, or just copy, all sections
  for EachEnumVal(RDI_SectionKind, k)
  {
    RDIM_SerializedSection *src = &in->sections[k];
    RDIM_SerializedSection *dst = &out.sections[k];
    MemoryCopyStruct(dst, src);
    
    // rjf: determine if this section should be compressed
    B32 should_compress = 1;
    
    // rjf: compress if needed
    if(should_compress)
    {
      MemoryZero(ctx.m_hashTable, sizeof(U16)*(1<<ctx.m_tableSizeBits));
      dst->data = push_array_no_zero(arena, U8, src->encoded_size);
      dst->encoded_size = rr_lzb_simple_encode_veryfast(&ctx, src->data, src->encoded_size, dst->data);
      dst->unpacked_size = src->encoded_size;
      dst->encoding = RDI_SectionEncoding_LZB;
    }
  }
  
  return out;
}
