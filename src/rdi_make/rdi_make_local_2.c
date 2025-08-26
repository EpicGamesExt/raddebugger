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
  //- rjf: gather all unsorted, joined, line table info
  //
  ProfScope("gather all unsorted, joined, line table info")
  {
    //- rjf: set up outputs
    if(lane_idx() == 0)
    {
      rdim2_shared->line_tables_count = params->line_tables.total_count;
      rdim2_shared->src_line_tables = push_array(arena, RDIM_LineTable *, rdim2_shared->line_tables_count);
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
      rdim2_shared->unsorted_joined_line_tables = push_array(arena, RDIM_UnsortedJoinedLineTable, rdim2_shared->line_tables_count);
      rdim2_shared->line_table_take_counter = 0;
    }
    lane_sync();
    
    //- rjf: wide gather
    {
      for(;;)
      {
        U64 line_table_num = ins_atomic_u64_inc_eval(&rdim2_shared->line_table_take_counter);
        if(0 == line_table_num || rdim2_shared->line_tables_count < line_table_num)
        {
          break;
        }
        U64 line_table_idx = line_table_num-1;
        RDIM_LineTable *src = rdim2_shared->src_line_tables[line_table_idx];
        RDIM_UnsortedJoinedLineTable *dst = &rdim2_shared->unsorted_joined_line_tables[line_table_idx];
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
      }
    }
  }
  lane_sync();
  RDI_U64 line_tables_count = rdim2_shared->line_tables_count;
  RDIM_LineTable **src_line_tables = rdim2_shared->src_line_tables;
  RDIM_UnsortedJoinedLineTable *unsorted_joined_line_tables = rdim2_shared->unsorted_joined_line_tables;
  
  //////////////////////////////////////////////////////////////
  //- rjf: sort all unsorted line table info
  //
  ProfScope("sort all unsorted line table info")
  {
    if(lane_idx() == 0)
    {
      rdim2_shared->sorted_line_table_keys = push_array(arena, RDIM_SortKey *, line_tables_count);
      rdim2_shared->line_table_take_counter = 0;
    }
    lane_sync();
    for(;;)
    {
      U64 line_table_num = ins_atomic_u64_inc_eval(&rdim2_shared->line_table_take_counter);
      if(0 == line_table_num || line_tables_count < line_table_num)
      {
        break;
      }
      U64 line_table_idx = line_table_num-1;
      rdim2_shared->sorted_line_table_keys[line_table_idx] = rdim_sort_key_array(arena, unsorted_joined_line_tables[line_table_idx].line_keys, unsorted_joined_line_tables[line_table_idx].key_count);
    }
  }
  lane_sync();
  
  RDIM_BakeResults result = {0};
  return result;
}
