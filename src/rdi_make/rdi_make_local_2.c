// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDIM_BakeResults
rdim2_bake(Arena *arena, RDIM_BakeParams *params)
{
  //////////////////////////////////////////////////////////////
  //- rjf: set up shared state
  //
  if(lane_idx() == 0)
  {
    rdim2_shared = push_array(arena, RDIM2_Shared, 1);
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: build interned path tree
  //
  if(lane_idx() == 0) ProfScope("build interned path tree")
  {
    rdim2_shared->path_tree = rdim_bake_path_tree_from_params(arena, params);
  }
  lane_sync();
  RDIM_BakePathTree *path_tree = rdim2_shared->path_tree;
  
  //////////////////////////////////////////////////////////////
  //- rjf: gather all unsorted, joined, line table info; & sort
  //
  ProfScope("gather all unsorted, joined, line table info; & sort")
  {
    //- rjf: set up outputs
    ProfScope("set up outputs") if(lane_idx() == 0)
    {
      rdim2_shared->line_tables_count = params->line_tables.total_count;
      rdim2_shared->src_line_tables = push_array(arena, RDIM_LineTable *, rdim2_shared->line_tables_count);
      ProfScope("flatten chunk list")
      {
        U64 joined_idx = 0;
        for(RDIM_LineTableChunkNode *n = params->line_tables.first; n != 0; n = n->next)
        {
          for EachIndex(idx, n->count)
          {
            rdim2_shared->src_line_tables[joined_idx] = &n->v[idx];
            joined_idx += 1;
          }
        }
      }
      rdim2_shared->baked_line_tables.line_tables_count       = params->line_tables.total_count + 1;
      rdim2_shared->baked_line_tables.line_table_voffs_count  = params->line_tables.total_line_count + 2*params->line_tables.total_seq_count;
      rdim2_shared->baked_line_tables.line_table_lines_count  = params->line_tables.total_line_count + params->line_tables.total_seq_count;
      rdim2_shared->baked_line_tables.line_table_columns_count= 1;
      ProfScope("allocate outputs")
      {
        rdim2_shared->unsorted_joined_line_tables = push_array(arena, RDIM_UnsortedJoinedLineTable, rdim2_shared->line_tables_count);
        rdim2_shared->sorted_line_table_keys = push_array(arena, RDIM_SortKey *, rdim2_shared->line_tables_count);
        rdim2_shared->baked_line_tables.line_tables        = push_array(arena, RDI_LineTable, rdim2_shared->baked_line_tables.line_tables_count);
        rdim2_shared->baked_line_tables.line_table_voffs   = push_array(arena, RDI_U64,       rdim2_shared->baked_line_tables.line_table_voffs_count);
        rdim2_shared->baked_line_tables.line_table_lines   = push_array(arena, RDI_Line,      rdim2_shared->baked_line_tables.line_table_lines_count);
        rdim2_shared->baked_line_tables.line_table_columns = push_array(arena, RDI_Column,    rdim2_shared->baked_line_tables.line_table_columns_count);
      }
      U64 voffs_base_idx = 0;
      U64 lines_base_idx = 0;
      U64 cols_base_idx = 0;
      ProfScope("lay out line tables") for EachIndex(idx, rdim2_shared->line_tables_count)
      {
        U64 final_idx = idx+1; // NOTE(rjf): +1, to reserve [0] for nil
        RDIM_LineTable *src = rdim2_shared->src_line_tables[idx];
        RDI_LineTable *dst = &rdim2_shared->baked_line_tables.line_tables[final_idx];
        dst->voffs_base_idx = voffs_base_idx; // TODO(rjf): @u64_to_u32
        dst->lines_base_idx = lines_base_idx; // TODO(rjf): @u64_to_u32
        dst->cols_base_idx  = cols_base_idx; // TODO(rjf): @u64_to_u32
        dst->lines_count    = src->line_count + src->seq_count; // TODO(rjf): @u64_to_u32
        voffs_base_idx += src->line_count + 2*src->seq_count;
        lines_base_idx += src->line_count + 1*src->seq_count;
      }
      rdim2_shared->line_table_block_take_counter = 0;
    }
    lane_sync();
    
    //- rjf: wide bake
    ProfScope("wide bake") 
    {
      U64 line_table_block_size = 4;
      U64 line_table_block_count = (rdim2_shared->line_tables_count + line_table_block_size - 1) / line_table_block_size;
      for(;;)
      {
        U64 line_table_block_num = ins_atomic_u64_inc_eval(&rdim2_shared->line_table_block_take_counter);
        if(0 == line_table_block_num || line_table_block_count < line_table_block_num)
        {
          break;
        }
        U64 line_table_block_idx = line_table_block_num-1;
        Rng1U64 line_table_range = r1u64(line_table_block_idx*line_table_block_size, (line_table_block_idx+1)*line_table_block_size);
        line_table_range.max = Min(rdim2_shared->line_tables_count, line_table_range.max);
        for EachInRange(line_table_idx, line_table_range)
        {
          RDIM_LineTable *src = rdim2_shared->src_line_tables[line_table_idx];
          RDIM_UnsortedJoinedLineTable *dst = &rdim2_shared->unsorted_joined_line_tables[line_table_idx];
          
          //- rjf: gather
          dst->line_count = src->line_count;
          dst->seq_count = src->seq_count;
          dst->key_count = dst->line_count + dst->seq_count;
          dst->line_keys = rdim_push_array_no_zero(arena, RDIM_SortKey, dst->key_count);
          dst->line_recs = rdim_push_array_no_zero(arena, RDIM_LineRec, dst->line_count);
          {
            RDIM_SortKey *key_ptr = dst->line_keys;
            RDIM_LineRec *rec_ptr = dst->line_recs;
            for(RDIM_LineSequenceNode *seq_n = src->first_seq; seq_n != 0; seq_n = seq_n->next)
            {
              RDIM_LineSequence *seq = &seq_n->v;
              for(RDI_U64 line_idx = 0; line_idx < seq->line_count; line_idx += 1)
              {
                key_ptr->key = seq->voffs[line_idx];
                key_ptr->val = rec_ptr;
                key_ptr += 1;
                rec_ptr->file_id = (RDI_U32)rdim_idx_from_src_file(seq->src_file); // TODO(rjf): @u64_to_u32
                rec_ptr->line_num = seq->line_nums[line_idx];
                if(seq->col_nums != 0)
                {
                  rec_ptr->col_first = seq->col_nums[line_idx*2];
                  rec_ptr->col_opl = seq->col_nums[line_idx*2 + 1];
                }
                rec_ptr += 1;
              }
              key_ptr->key = seq->voffs[seq->line_count];
              key_ptr->val = 0;
              key_ptr += 1;
            }
          }
          
          //- rjf: sort
          rdim2_shared->sorted_line_table_keys[line_table_idx] = rdim_sort_key_array(arena,
                                                                                     rdim2_shared->unsorted_joined_line_tables[line_table_idx].line_keys,
                                                                                     rdim2_shared->unsorted_joined_line_tables[line_table_idx].key_count);
          
          //- rjf: fill
          RDIM_SortKey *sorted_line_keys = rdim2_shared->sorted_line_table_keys[line_table_idx];
          U64 sorted_line_keys_count = rdim2_shared->unsorted_joined_line_tables[line_table_idx].key_count;
          RDI_LineTable *dst_line_table = &rdim2_shared->baked_line_tables.line_tables[line_table_idx+1];
          U64 *arranged_voffs           = rdim2_shared->baked_line_tables.line_table_voffs   + dst_line_table->voffs_base_idx;
          RDI_Line *arranged_lines      = rdim2_shared->baked_line_tables.line_table_lines   + dst_line_table->lines_base_idx;
          RDI_Column *arranged_cols     = rdim2_shared->baked_line_tables.line_table_columns + dst_line_table->cols_base_idx;
          {
            for EachIndex(idx, sorted_line_keys_count)
            {
              arranged_voffs[idx] = sorted_line_keys[idx].key;
            }
            arranged_voffs[sorted_line_keys_count] = ~0ull;
            for EachIndex(idx, sorted_line_keys_count)
            {
              RDIM_LineRec *rec = (RDIM_LineRec*)sorted_line_keys[idx].val;
              if(rec != 0)
              {
                arranged_lines[idx].file_idx = rec->file_id;
                arranged_lines[idx].line_num = rec->line_num;
              }
              else
              {
                arranged_lines[idx].file_idx = 0;
                arranged_lines[idx].line_num = 0;
              }
            }
          }
        }
      }
    }
  }
  lane_sync();
  RDI_U64 line_tables_count = rdim2_shared->line_tables_count;
  RDIM_LineTable **src_line_tables = rdim2_shared->src_line_tables;
  RDIM_UnsortedJoinedLineTable *unsorted_joined_line_tables = rdim2_shared->unsorted_joined_line_tables;
  RDIM_SortKey **sorted_line_table_keys = rdim2_shared->sorted_line_table_keys;
  
  //////////////////////////////////////////////////////////////
  //- rjf: build string map
  //
  ProfScope("build string map")
  {
    //- rjf: set up per-lane outputs
    if(lane_idx() == 0) ProfScope("set up per-lane outputs")
    {
      rdim2_shared->bake_string_map_topology.slots_count = (64 +
                                                            params->procedures.total_count*1 +
                                                            params->global_variables.total_count*1 +
                                                            params->thread_variables.total_count*1 +
                                                            params->types.total_count/2);
      rdim2_shared->lane_bake_string_maps__loose = push_array(arena, RDIM_BakeStringMapLoose *, lane_count());
    }
    lane_sync();
    
    //- rjf: set up this lane's map
    ProfScope("set up this lane's map")
    {
      rdim2_shared->lane_bake_string_maps__loose[lane_idx()] = rdim_bake_string_map_loose_make(arena, &rdim2_shared->bake_string_map_topology);
    }
    RDIM_BakeStringMapTopology *lane_map_top = &rdim2_shared->bake_string_map_topology;
    RDIM_BakeStringMapLoose *lane_map = rdim2_shared->lane_bake_string_maps__loose[lane_idx()];
    
    //- rjf: push all strings into this lane's map
    ProfScope("push all strings into this lane's map")
    {
      // rjf: push small top-level strings
      if(lane_idx() == 0) ProfScope("push small top-level strings")
      {
        rdim_bake_string_map_loose_push_top_level_info(arena, lane_map_top, lane_map, &params->top_level_info);
        rdim_bake_string_map_loose_push_binary_sections(arena, lane_map_top, lane_map, &params->binary_sections);
        rdim_bake_string_map_loose_push_path_tree(arena, lane_map_top, lane_map, path_tree);
      }
      
      // rjf: push strings from source files
      ProfScope("src files")
      {
        for(RDIM_SrcFileChunkNode *n = params->src_files.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_src_file_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      // rjf: push strings from units
      ProfScope("units")
      {
        for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_unit_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      // rjf: push strings from types
      ProfScope("types")
      {
        for(RDIM_TypeChunkNode *n = params->types.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_type_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      // rjf: push strings from udts
      ProfScope("udts")
      {
        for(RDIM_UDTChunkNode *n = params->udts.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_udt_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      // rjf: push strings from symbols
      RDIM_SymbolChunkList *symbol_lists[] =
      {
        &params->global_variables,
        &params->thread_variables,
        &params->procedures,
        &params->constants,
      };
      ProfScope("symbols")
      {
        for EachElement(list_idx, symbol_lists)
        {
          ProfScope("symbols (%I64u)", list_idx)
          {
            for(RDIM_SymbolChunkNode *n = symbol_lists[list_idx]->first; n != 0; n = n->next)
            {
              Rng1U64 range = lane_range(n->count);
              rdim_bake_string_map_loose_push_symbol_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
            }
          }
        }
      }
      
      //- rjf: push strings from inline sites
      ProfScope("inline sites")
      {
        for(RDIM_InlineSiteChunkNode *n = params->inline_sites.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_inline_site_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      //- rjf: push strings from scopes
      ProfScope("scopes")
      {
        for(RDIM_ScopeChunkNode *n = params->scopes.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_scope_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
    }
    
    //- rjf: join & sort
    if(lane_idx() == 0)
    {
      rdim2_shared->bake_string_map__loose = rdim_bake_string_map_loose_make(arena, &rdim2_shared->bake_string_map_topology);
    }
    lane_sync();
    ProfScope("join & sort")
    {
      //- rjf: join
      ProfScope("join")
      {
        Rng1U64 slot_range = lane_range(rdim2_shared->bake_string_map_topology.slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          for EachIndex(src_lane_idx, lane_count())
          {
            RDIM_BakeStringMapLoose *src_map = rdim2_shared->lane_bake_string_maps__loose[src_lane_idx];
            RDIM_BakeStringMapLoose *dst_map = rdim2_shared->bake_string_map__loose;
            if(dst_map->slots[slot_idx] == 0 && src_map->slots[slot_idx] != 0)
            {
              dst_map->slots[slot_idx] = src_map->slots[slot_idx];
            }
            else if(dst_map->slots[slot_idx] != 0 && src_map->slots[slot_idx] != 0)
            {
              rdim_bake_string_chunk_list_concat_in_place(dst_map->slots[slot_idx], src_map->slots[slot_idx]);
            }
          }
        }
      }
      
      //- rjf: sort
      ProfScope("sort")
      {
        RDIM_BakeStringMapLoose *map = rdim2_shared->bake_string_map__loose;
        Rng1U64 slot_range = lane_range(rdim2_shared->bake_string_map_topology.slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          if(map->slots[slot_idx] != 0 && map->slots[slot_idx]->total_count > 1)
          {
            *map->slots[slot_idx] = rdim_bake_string_chunk_list_sorted_from_unsorted(arena, map->slots[slot_idx]);
          }
        }
      }
    }
    lane_sync();
    
    //- rjf: tighten string table
    if(lane_idx() == 0) ProfScope("tighten string table")
    {
      RDIM_BakeStringMapLoose *map = rdim2_shared->bake_string_map__loose;
      RDIM_BakeStringMapTopology *map_top = &rdim2_shared->bake_string_map_topology;
      RDIM_BakeStringMapBaseIndices bake_string_map_base_idxes = rdim_bake_string_map_base_indices_from_map_loose(arena, map_top, map);
      rdim2_shared->bake_strings = rdim_bake_string_map_tight_from_loose(arena, map_top, &bake_string_map_base_idxes, map);
    }
  }
  lane_sync();
  RDIM_BakeStringMapTight *bake_strings = &rdim2_shared->bake_strings;
  
  //////////////////////////////////////////////////////////////
  //- rjf: bake name maps
  //
  ProfScope("bake name maps")
  {
    //- rjf: set up
    if(lane_idx() == 0)
    {
      for EachNonZeroEnumVal(RDI_NameMapKind, k)
      {
        U64 slot_count = 0;
        switch((RDI_NameMapKindEnum)k)
        {
          case RDI_NameMapKind_NULL:
          case RDI_NameMapKind_COUNT:
          {}break;
#define Case(name, total_count) case RDI_NameMapKind_##name:{slot_count = ((total_count) + (total_count)/4);}break
          Case(GlobalVariables, params->global_variables.total_count);
          Case(ThreadVariables, params->thread_variables.total_count);
          Case(Constants, params->constants.total_count);
          Case(Procedures, params->procedures.total_count);
          Case(LinkNameProcedures, params->procedures.total_count);
          Case(Types, params->types.total_count);
          Case(NormalSourcePaths, params->src_files.total_count);
#undef Case
        }
        rdim2_shared->lane_bake_name_maps[k] = push_array(arena, RDIM_BakeNameMap2 *, lane_count());
        rdim2_shared->bake_name_map_topology[k].slots_count = slot_count;
      }
    }
    lane_sync();
    
    //- rjf: wide build
    for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("name map build %.*s", str8_varg(rdi_string_from_name_map_kind(k)))
    {
      RDIM_BakeNameMapTopology *top = &rdim2_shared->bake_name_map_topology[k];
      rdim2_shared->lane_bake_name_maps[k][lane_idx()] = rdim_bake_name_map_2_make(arena, top);
      RDIM_BakeNameMap2 *map = rdim2_shared->lane_bake_name_maps[k][lane_idx()];
      B32 link_names = 0;
      RDIM_SymbolChunkList *symbols = 0;
      switch((RDI_NameMapKindEnum)k)
      {
        case RDI_NameMapKind_NULL:
        case RDI_NameMapKind_COUNT:
        {}break;
        case RDI_NameMapKind_GlobalVariables:   {symbols = &params->global_variables;}goto symbol_name_map_build;
        case RDI_NameMapKind_ThreadVariables:   {symbols = &params->thread_variables;}goto symbol_name_map_build;
        case RDI_NameMapKind_Constants:         {symbols = &params->constants;}goto symbol_name_map_build;
        case RDI_NameMapKind_Procedures:        {symbols = &params->procedures;}goto symbol_name_map_build;
        case RDI_NameMapKind_LinkNameProcedures:{symbols = &params->procedures; link_names = 1;}goto symbol_name_map_build;
        symbol_name_map_build:;
        {
          for EachNode(n, RDIM_SymbolChunkNode, symbols->first)
          {
            Rng1U64 n_range = lane_range(n->count);
            for EachInRange(n_idx, n_range)
            {
              RDIM_Symbol *symbol = &n->v[n_idx];
              rdim_bake_name_map_2_insert(arena, top, map, 4, link_names ? symbol->link_name : symbol->name, rdim_idx_from_symbol(symbol));
            }
          }
        }break;
        case RDI_NameMapKind_Types:
        {
          RDIM_TypeChunkList *types = &params->types;
          for EachNode(n, RDIM_TypeChunkNode, types->first)
          {
            Rng1U64 n_range = lane_range(n->count);
            for EachInRange(n_idx, n_range)
            {
              RDIM_Type *type = &n->v[n_idx];
              rdim_bake_name_map_2_insert(arena, top, map, 4, type->name, rdim_idx_from_type(type));
            }
          }
        }break;
        case RDI_NameMapKind_NormalSourcePaths:
        {
          RDIM_SrcFileChunkList *src_files = &params->src_files;
          for EachNode(n, RDIM_SrcFileChunkNode, src_files->first)
          {
            Rng1U64 n_range = lane_range(n->count);
            for EachInRange(n_idx, n_range)
            {
              RDIM_SrcFile *src_file = &n->v[n_idx];
              RDIM_String8 normalized_path = rdim_lower_from_str8(arena, src_file->path);
              rdim_bake_name_map_2_insert(arena, top, map, 4, normalized_path, rdim_idx_from_src_file(src_file));
            }
          }
        }break;
      }
    }
    lane_sync();
    
    //- rjf: join & sort
    if(lane_idx() == 0)
    {
      for EachNonZeroEnumVal(RDI_NameMapKind, k)
      {
        rdim2_shared->bake_name_maps[k] = rdim_bake_name_map_2_make(arena, &rdim2_shared->bake_name_map_topology[k]);
      }
    }
    lane_sync();
    for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("name map join & sort %.*s", str8_varg(rdi_string_from_name_map_kind(k)))
    {
      RDIM_BakeNameMapTopology *top = &rdim2_shared->bake_name_map_topology[k];
      RDIM_BakeNameMap2 *map = rdim2_shared->bake_name_maps[k];
      
      //- rjf: join
      ProfScope("join")
      {
        Rng1U64 slot_range = lane_range(top->slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          for EachIndex(src_lane_idx, lane_count())
          {
            RDIM_BakeNameMap2 *src_map = rdim2_shared->lane_bake_name_maps[k][src_lane_idx];
            RDIM_BakeNameMap2 *dst_map = map;
            if(dst_map->slots[slot_idx] == 0 && src_map->slots[slot_idx] != 0)
            {
              dst_map->slots[slot_idx] = src_map->slots[slot_idx];
            }
            else if(dst_map->slots[slot_idx] != 0 && src_map->slots[slot_idx] != 0)
            {
              rdim_bake_name_chunk_list_concat_in_place(dst_map->slots[slot_idx], src_map->slots[slot_idx]);
            }
          }
        }
      }
      
      //- rjf: sort
      ProfScope("sort")
      {
        Rng1U64 slot_range = lane_range(top->slots_count);
        for EachInRange(slot_idx, slot_range)
        {
          if(map->slots[slot_idx] != 0 && map->slots[slot_idx]->total_count > 1)
          {
            *map->slots[slot_idx] = rdim_bake_name_chunk_list_sorted_from_unsorted(arena, map->slots[slot_idx]);
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: bake index runs
  //
  ProfScope("bake index runs")
  {
    //- rjf: set up per-lane outputs
    if(lane_idx() == 0) ProfScope("set up per-lane outputs")
    {
      rdim2_shared->bake_idx_run_map_topology.slots_count = (64 +
                                                             params->procedures.total_count +
                                                             params->global_variables.total_count +
                                                             params->thread_variables.total_count +
                                                             params->types.total_count);
      rdim2_shared->lane_bake_idx_run_maps__loose = push_array(arena, RDIM_BakeIdxRunMapLoose *, lane_count());
    }
    lane_sync();
    
    //- rjf: set up this lane's map
    ProfScope("set up this lane's map")
    {
      rdim2_shared->lane_bake_idx_run_maps__loose[lane_idx()] = rdim_bake_idx_run_map_loose_make(arena, &rdim2_shared->bake_idx_run_map_topology);
    }
    RDIM_BakeIdxRunMapTopology *lane_map_top = &rdim2_shared->bake_idx_run_map_topology;
    RDIM_BakeIdxRunMapLoose *lane_map = rdim2_shared->lane_bake_idx_run_maps__loose[lane_idx()];
    
    //- rjf: wide fill of all index runs
    ProfScope("fill all lane index run maps")
    {
      //- rjf: bake runs of function-type parameter lists
      ProfScope("bake runs of function-type parameter lists")
      {
        for EachNode(n, RDIM_TypeChunkNode, params->types.first)
        {
          Rng1U64 range = lane_range(n->count);
          ProfScope("[%I64u, %I64u)", range.min, range.max) for EachInRange(n_idx, range)
          {
            RDIM_Type *type = &n->v[n_idx];
            if(type->count == 0)
            {
              continue;
            }
            if(type->kind == RDI_TypeKind_Function || type->kind == RDI_TypeKind_Method)
            {
              RDI_U32 param_idx_run_count = type->count;
              RDI_U32 *param_idx_run = rdim_push_array_no_zero(arena, RDI_U32, param_idx_run_count);
              for(RDI_U32 idx = 0; idx < param_idx_run_count; idx += 1)
              {
                param_idx_run[idx] = (RDI_U32)rdim_idx_from_type(type->param_types[idx]); // TODO(rjf): @u64_to_u32
              }
              rdim_bake_idx_run_map_loose_insert(arena, lane_map_top, lane_map, 4, param_idx_run, param_idx_run_count);
            }
          }
        }
      }
      
      //- rjf: bake runs of name map match lists
      for EachNonZeroEnumVal(RDI_NameMapKind, k) ProfScope("bake runs of name map match lists (%.*s)", str8_varg(rdi_string_from_name_map_kind(k)))
      {
        RDIM_BakeNameMapTopology *top = &rdim2_shared->bake_name_map_topology[k];
        RDIM_BakeNameMap2 *map = rdim2_shared->bake_name_maps[k];
        Rng1U64 slot_idx_range = lane_range(top->slots_count);
        for EachInRange(slot_idx, slot_idx_range)
        {
          RDIM_BakeNameChunkList *slot = map->slots[slot_idx];
          if(slot != 0)
          {
            Temp scratch = scratch_begin(&arena, 1);
            typedef struct IdxRunNode IdxRunNode;
            struct IdxRunNode
            {
              IdxRunNode *next;
              RDI_U64 idx;
            };
            IdxRunNode *first_idx_run_node = 0;
            IdxRunNode *last_idx_run_node = 0;
            U64 active_idx_count = 0;
            U64 active_hash = 0;
            RDIM_BakeNameChunkNode *n = slot->first;
            U64 n_idx = 0;
            for(;;)
            {
              // rjf: advance chunk
              if(n != 0 && n_idx >= n->count)
              {
                n = n->next;
                n_idx = 0;
              }
              
              // rjf: grab next element
              U64 hash = 0;
              U64 idx = 0;
              if(n != 0)
              {
                hash = n->v[n_idx].hash;
                idx  = n->v[n_idx].idx;
              }
              
              // rjf: next element hash doesn't match the active? -> push index run, clear active list, start new list
              if(hash != active_hash && active_idx_count != 0)
              {
                if(active_idx_count > 1)
                {
                  RDI_U64 idxs_count = active_idx_count;
                  RDI_U32 *idxs = rdim_push_array(arena, RDI_U32, idxs_count);
                  {
                    U64 write_idx = 0;
                    for EachNode(idx_run_n, IdxRunNode, first_idx_run_node)
                    {
                      idxs[write_idx] = (RDI_U32)idx_run_n->idx; // TODO(rjf): @u64_to_u32
                      write_idx += 1;
                    }
                  }
                  rdim_bake_idx_run_map_loose_insert(arena, lane_map_top, lane_map, 4, idxs, idxs_count);
                }
                active_hash = hash;
                first_idx_run_node = 0;
                last_idx_run_node = 0;
                temp_end(scratch);
              }
              
              // rjf: hash matches the active list -> push
              if(hash != 0 && hash == active_hash)
              {
                IdxRunNode *idx_run_n = push_array(scratch.arena, IdxRunNode, 1);
                idx_run_n->idx = idx;
                SLLQueuePush(first_idx_run_node, last_idx_run_node, idx_run_n);
                active_idx_count += 1;
              }
              
              // rjf: advance index
              n_idx += 1;
              
              // rjf: end on zero node
              if(n == 0)
              {
                break;
              }
            }
            scratch_end(scratch);
          }
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: bake units, src files, symbols, types, UDTs
  //
  {
    //- rjf: setup outputs
    if(lane_idx() == lane_from_task_idx(0))
    {
      rdim2_shared->baked_units.units_count = params->units.total_count+1;
      rdim2_shared->baked_units.units = push_array(arena, RDI_Unit, rdim2_shared->baked_units.units_count);
    }
    if(lane_idx() == lane_from_task_idx(1))
    {
      rdim2_shared->baked_src_files.source_files_count = params->src_files.total_count+1;
      rdim2_shared->baked_src_files.source_files = push_array(arena, RDI_SourceFile, rdim2_shared->baked_src_files.source_files_count);
    }
    if(lane_idx() == lane_from_task_idx(2))
    {
      rdim2_shared->baked_src_files.source_line_maps_count = params->src_files.source_line_map_count+1;
      rdim2_shared->baked_src_files.source_line_maps = push_array(arena, RDI_SourceLineMap, rdim2_shared->baked_src_files.source_line_maps_count);
    }
    if(lane_idx() == lane_from_task_idx(3))
    {
      rdim2_shared->baked_type_nodes.type_nodes_count = params->types.total_count+1;
      rdim2_shared->baked_type_nodes.type_nodes = push_array(arena, RDI_TypeNode, rdim2_shared->baked_type_nodes.type_nodes_count);
    }
    if(lane_idx() == lane_from_task_idx(4))
    {
      rdim2_shared->baked_udts.udts_count = params->udts.total_count+1;
      rdim2_shared->baked_udts.udts = push_array(arena, RDI_UDT, rdim2_shared->baked_udts.udts_count);
    }
    if(lane_idx() == lane_from_task_idx(5))
    {
      rdim2_shared->baked_udts.members_count = params->udts.total_member_count+1;
      rdim2_shared->baked_udts.members = push_array(arena, RDI_Member, rdim2_shared->baked_udts.members_count);
    }
    if(lane_idx() == lane_from_task_idx(6))
    {
      rdim2_shared->baked_udts.enum_members_count = params->udts.total_enum_val_count+1;
      rdim2_shared->baked_udts.enum_members = push_array(arena, RDI_EnumMember, rdim2_shared->baked_udts.enum_members_count);
    }
    if(lane_idx() == lane_from_task_idx(7))
    {
      rdim2_shared->baked_global_variables.global_variables_count = params->global_variables.total_count+1;
      rdim2_shared->baked_global_variables.global_variables = push_array(arena, RDI_GlobalVariable, rdim2_shared->baked_global_variables.global_variables_count);
    }
    if(lane_idx() == lane_from_task_idx(8))
    {
      rdim2_shared->baked_thread_variables.thread_variables_count = params->thread_variables.total_count+1;
      rdim2_shared->baked_thread_variables.thread_variables = push_array(arena, RDI_ThreadVariable, rdim2_shared->baked_thread_variables.thread_variables_count);
    }
    if(lane_idx() == lane_from_task_idx(9))
    {
      rdim2_shared->baked_constants.constants_count = params->constants.total_count+1;
      rdim2_shared->baked_constants.constants = push_array(arena, RDI_Constant, rdim2_shared->baked_constants.constants_count);
    }
    if(lane_idx() == lane_from_task_idx(10))
    {
      rdim2_shared->baked_constants.constant_values_count = params->constants.total_count+1;
      rdim2_shared->baked_constants.constant_values = push_array(arena, RDI_U32, rdim2_shared->baked_constants.constant_values_count);
    }
    if(lane_idx() == lane_from_task_idx(11))
    {
      rdim2_shared->baked_constants.constant_value_data_size = params->constants.total_value_data_size;
      rdim2_shared->baked_constants.constant_value_data = push_array(arena, RDI_U8, rdim2_shared->baked_constants.constant_value_data_size);
    }
    if(lane_idx() == lane_from_task_idx(12))
    {
      rdim2_shared->baked_procedures.procedures_count = params->procedures.total_count+1;
      rdim2_shared->baked_procedures.procedures = push_array(arena, RDI_Procedure, rdim2_shared->baked_procedures.procedures_count);
    }
    if(lane_idx() == lane_from_task_idx(13))
    {
      rdim2_shared->baked_inline_sites.inline_sites_count = params->inline_sites.total_count+1;
      rdim2_shared->baked_inline_sites.inline_sites = push_array(arena, RDI_InlineSite, rdim2_shared->baked_inline_sites.inline_sites_count);
    }
    lane_sync();
    
    //- rjf: bake units
    ProfScope("bake units")
    {
      for EachNode(n, RDIM_UnitChunkNode, params->units.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Unit *src = &n->v[n_idx];
          RDI_Unit *dst = &rdim2_shared->baked_units.units[n->base_idx + n_idx + 1];
          dst->unit_name_string_idx     = rdim_bake_idx_from_string(bake_strings, src->unit_name);
          dst->compiler_name_string_idx = rdim_bake_idx_from_string(bake_strings, src->compiler_name);
          dst->source_file_path_node    = rdim_bake_path_node_idx_from_string(path_tree, src->source_file);
          dst->object_file_path_node    = rdim_bake_path_node_idx_from_string(path_tree, src->object_file);
          dst->archive_file_path_node   = rdim_bake_path_node_idx_from_string(path_tree, src->archive_file);
          dst->build_path_node          = rdim_bake_path_node_idx_from_string(path_tree, src->build_path);
          dst->language                 = src->language;
          dst->line_table_idx           = (RDI_U32)rdim_idx_from_line_table(src->line_table); // TODO(rjf): @u64_to_u32
        }
      }
    }
    
    //- rjf: bake global variables
    ProfScope("bake global variables")
    {
      for EachNode(n, RDIM_SymbolChunkNode, params->global_variables.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_GlobalVariable *dst = &rdim2_shared->baked_global_variables.global_variables[n->base_idx + n_idx + 1];
          dst->name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->voff            = src->offset;
          dst->type_idx        = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
          }
        }
      }
    }
    
    //- rjf: bake thread variables
    ProfScope("bake thread variables")
    {
      for EachNode(n, RDIM_SymbolChunkNode, params->thread_variables.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDIM_Symbol *src = &n->v[n_idx];
          RDI_ThreadVariable *dst = &rdim2_shared->baked_thread_variables.thread_variables[n->base_idx + n_idx + 1];
          dst->name_string_idx = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->tls_off         = (RDI_U32)src->offset; // TODO(rjf): @u64_to_u32
          dst->type_idx        = (RDI_U32)rdim_idx_from_type(src->type);
          if(src->is_extern)
          {
            dst->link_flags |= RDI_LinkFlag_External;
          }
          if(src->container_type != 0)
          {
            dst->link_flags |= RDI_LinkFlag_TypeScoped;
            dst->container_idx = src->container_type ? (RDI_U32)rdim_idx_from_udt(src->container_type->udt) : 0; // TODO(rjf): @u64_to_u32
          }
          else if(src->container_symbol != 0)
          {
            dst->link_flags |= RDI_LinkFlag_ProcScoped;
            dst->container_idx = (RDI_U32)rdim_idx_from_symbol(src->container_symbol); // TODO(rjf): @u64_to_u32
          }
        }
      }
    }
    
    //- rjf: bake inline sites
    ProfScope("bake inline sites")
    {
      for EachNode(n, RDIM_InlineSiteChunkNode, params->inline_sites.first)
      {
        Rng1U64 range = lane_range(n->count);
        for EachInRange(n_idx, range)
        {
          RDI_InlineSite *dst = &rdim2_shared->baked_inline_sites.inline_sites[n->base_idx + n_idx + 1];
          RDIM_InlineSite *src = &n->v[n_idx];
          dst->name_string_idx   = rdim_bake_idx_from_string(bake_strings, src->name);
          dst->type_idx          = (RDI_U32)rdim_idx_from_type(src->type); // TODO(rjf): @u64_to_u32
          dst->owner_type_idx    = (RDI_U32)rdim_idx_from_type(src->owner); // TODO(rjf): @u64_to_u32
          dst->line_table_idx    = (RDI_U32)rdim_idx_from_line_table(src->line_table); // TODO(rjf): @u64_to_u32
        }
      }
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: package results
  //
  RDIM_BakeResults result = {0};
  {
    // result.top_level_info         = rdim2_shared->baked_top_level_info;
    // result.binary_sections        = rdim2_shared->baked_binary_sections;
    result.units                  = rdim2_shared->baked_units;
    result.unit_vmap              = rdim2_shared->baked_unit_vmap;
    result.src_files              = rdim2_shared->baked_src_files;
    result.line_tables            = rdim2_shared->baked_line_tables;
    // result.type_nodes             = rdim2_shared->baked_type_nodes;
    result.udts                   = rdim2_shared->baked_udts;
    result.global_variables       = rdim2_shared->baked_global_variables;
    result.global_vmap            = rdim2_shared->baked_global_vmap;
    result.thread_variables       = rdim2_shared->baked_thread_variables;
    result.constants              = rdim2_shared->baked_constants;
    result.procedures             = rdim2_shared->baked_procedures;
    // result.scopes                 = rdim2_shared->baked_scopes;
    result.inline_sites           = rdim2_shared->baked_inline_sites;
    result.scope_vmap             = rdim2_shared->baked_scope_vmap;
    // result.top_level_name_maps    = rdim2_shared->baked_top_level_name_maps;
    // result.name_maps              = rdim2_shared->baked_name_maps;
    // result.file_paths             = rdim2_shared->baked_file_paths;
    result.strings                = rdim2_shared->baked_strings;
    // result.idx_runs               = rdim2_shared->baked_idx_runs;
    // result.location_blocks        = rdim2_shared->baked_location_blocks;
    // result.location_data          = rdim2_shared->baked_location_data;
  }
  
  return result;
}
