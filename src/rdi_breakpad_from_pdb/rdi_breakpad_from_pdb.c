// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Baking Tasks

//- rjf: unit vmap baking

ASYNC_WORK_DEF(p2b_bake_unit_vmap_work)
{
  ProfBeginFunction();
  Arena *arena = async_root_thread_arena(p2b_async_root);
  P2B_BakeUnitVMapIn *in = (P2B_BakeUnitVMapIn *)input;
  RDIM_UnitVMapBakeResult *out = push_array(arena, RDIM_UnitVMapBakeResult, 1);
  *out = rdim_bake_unit_vmap(arena, in->units);
  ProfEnd();
  return out;
}

//- rjf: line table baking

ASYNC_WORK_DEF(p2b_bake_line_table_work)
{
  ProfBeginFunction();
  Arena *arena = async_root_thread_arena(p2b_async_root);
  P2B_BakeLineTablesIn *in = (P2B_BakeLineTablesIn *)input;
  RDIM_LineTableBakeResult *out = push_array(arena, RDIM_LineTableBakeResult, 1);
  *out = rdim_bake_line_tables(arena, in->line_tables);
  ProfEnd();
  return out;
}

//- rjf: per-procedure chunk dumping

ASYNC_WORK_DEF(p2b_dump_proc_chunk_work)
{
  ProfBeginFunction();
  Arena *arena = async_root_thread_arena(p2b_async_root);
  P2B_DumpProcChunkIn *in = (P2B_DumpProcChunkIn *)input;
  String8List *out = push_array(arena, String8List, 1);
  RDI_LineTable *line_tables = in->line_tables_bake->line_tables;
  RDI_U64 line_tables_count = in->line_tables_bake->line_tables_count;
  RDI_U64 *line_table_voffs = in->line_tables_bake->line_table_voffs;
  RDI_U64 line_table_voffs_count = in->line_tables_bake->line_table_voffs_count;
  RDI_Line *line_table_lines = in->line_tables_bake->line_table_lines;
  RDI_U64 line_table_lines_count = in->line_tables_bake->line_table_lines_count;
  for(U64 idx = 0; idx < in->chunk->count; idx += 1)
  {
    // NOTE(rjf): breakpad does not support multiple voff ranges per procedure.
    RDIM_Symbol *proc = &in->chunk->v[idx];
    RDIM_Scope *root_scope = proc->root_scope;
    if(root_scope != 0 && root_scope->voff_ranges.first != 0)
    {
      // rjf: dump function record
      RDIM_Rng1U64 voff_range = root_scope->voff_ranges.first->v;
      str8_list_pushf(arena, out, "FUNC %I64x %I64x %I64x %S\n", voff_range.min, voff_range.max-voff_range.min, 0ull, proc->name);
      
      // rjf: dump function lines
      U64 unit_idx = rdi_vmap_idx_from_voff(in->unit_vmap, in->unit_vmap_count, voff_range.min);
      if(0 < unit_idx && unit_idx <= in->unit_count)
      {
        U32 line_table_idx = in->unit_line_table_idxs[unit_idx];
        if(0 < line_table_idx && line_table_idx <= line_tables_count)
        {
          // rjf: unpack unit line info
          RDI_LineTable *line_table = &line_tables[line_table_idx];
          RDI_ParsedLineTable line_info =
          {
            line_table_voffs + line_table->voffs_base_idx,
            line_table_lines + line_table->lines_base_idx,
            0,
            line_table->lines_count,
            0
          };
          for(U64 voff = voff_range.min, last_voff = 0;
              voff < voff_range.max && voff > last_voff;)
          {
            RDI_U64 line_info_idx = rdi_line_info_idx_from_voff(&line_info, voff);
            if(line_info_idx < line_info.count)
            {
              RDI_Line *line = &line_info.lines[line_info_idx];
              U64 line_voff_min = line_info.voffs[line_info_idx];
              U64 line_voff_opl = line_info.voffs[line_info_idx+1];
              if(line->file_idx != 0)
              {
                str8_list_pushf(arena, out, "%I64x %I64x %I64u %I64u\n",
                                line_voff_min,
                                line_voff_opl-line_voff_min,
                                (U64)line->line_num,
                                (U64)line->file_idx);
              }
              last_voff = voff;
              voff = line_voff_opl;
            }
            else
            {
              break;
            }
          }
        }
      }
    }
  }
  ProfEnd();
  return out;
}
