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
      rdim2_shared->final_line_tables_count = params->line_tables.total_count + 1;
      rdim2_shared->final_line_voffs_count  = params->line_tables.total_line_count + 2*params->line_tables.total_seq_count;
      rdim2_shared->final_lines_count       = params->line_tables.total_line_count + params->line_tables.total_seq_count;
      rdim2_shared->final_cols_count        = 1;
      ProfScope("allocate outputs")
      {
        rdim2_shared->unsorted_joined_line_tables = push_array(arena, RDIM_UnsortedJoinedLineTable, rdim2_shared->line_tables_count);
        rdim2_shared->sorted_line_table_keys = push_array(arena, RDIM_SortKey *, rdim2_shared->line_tables_count);
        rdim2_shared->final_line_tables = push_array(arena, RDI_LineTable, rdim2_shared->final_line_tables_count);
        rdim2_shared->final_line_voffs  = push_array(arena, RDI_U64,       rdim2_shared->final_line_voffs_count);
        rdim2_shared->final_lines       = push_array(arena, RDI_Line,      rdim2_shared->final_lines_count);
        rdim2_shared->final_cols        = push_array(arena, RDI_Column,    rdim2_shared->final_cols_count);
      }
      U64 voffs_base_idx = 0;
      U64 lines_base_idx = 0;
      U64 cols_base_idx = 0;
      ProfScope("lay out line tables") for EachIndex(idx, rdim2_shared->line_tables_count)
      {
        U64 final_idx = idx+1; // NOTE(rjf): +1, to reserve [0] for nil
        RDIM_LineTable *src = rdim2_shared->src_line_tables[idx];
        RDI_LineTable *dst = &rdim2_shared->final_line_tables[final_idx];
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
          RDI_LineTable *dst_line_table = &rdim2_shared->final_line_tables[line_table_idx+1];
          U64 *arranged_voffs           = rdim2_shared->final_line_voffs + dst_line_table->voffs_base_idx;
          RDI_Line *arranged_lines      = rdim2_shared->final_lines + dst_line_table->lines_base_idx;
          RDI_Column *arranged_cols     = rdim2_shared->final_cols + dst_line_table->cols_base_idx;
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
      //- rjf: push strings from source files
      ProfScope("src files")
      {
        for(RDIM_SrcFileChunkNode *n = params->src_files.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_src_file_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      //- rjf: push strings from units
      ProfScope("units")
      {
        for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_unit_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      //- rjf: push strings from types
      ProfScope("types")
      {
        for(RDIM_TypeChunkNode *n = params->types.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_type_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      //- rjf: push strings from udts
      ProfScope("udts")
      {
        for(RDIM_UDTChunkNode *n = params->udts.first; n != 0; n = n->next)
        {
          Rng1U64 range = lane_range(n->count);
          rdim_bake_string_map_loose_push_udt_slice(arena, lane_map_top, lane_map, n->v + range.min, dim_1u64(range));
        }
      }
      
      //- rjf: push strings from symbols
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
      
      //- rjf: sort string table
      ProfScope("sort string table")
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
#if 1
      for EachIndex(idx, rdim2_shared->bake_strings.slots_count)
      {
        for(RDIM_BakeStringChunkNode *n = rdim2_shared->bake_strings.slots[idx].first; n != 0; n = n->next)
        {
          for EachIndex(n_idx, n->count)
          {
            fprintf(stdout, "%.*s\n", str8_varg(n->v[n_idx].string));
          }
        }
      }
      fflush(stdout);
#endif
    }
  }
  lane_sync();
  
  //////////////////////////////////////////////////////////////
  //- rjf: package results
  //
  RDIM_BakeResults result = {0};
  {
    result.line_tables.line_tables                         = rdim2_shared->final_line_tables;
    result.line_tables.line_tables_count                   = rdim2_shared->final_line_tables_count;
    result.line_tables.line_table_voffs                    = rdim2_shared->final_line_voffs;
    result.line_tables.line_table_voffs_count              = rdim2_shared->final_line_voffs_count;
    result.line_tables.line_table_lines                    = rdim2_shared->final_lines;
    result.line_tables.line_table_lines_count              = rdim2_shared->final_lines_count;
    result.line_tables.line_table_columns                  = rdim2_shared->final_cols;
    result.line_tables.line_table_columns_count            = rdim2_shared->final_cols_count;
  }
  
  return result;
}
