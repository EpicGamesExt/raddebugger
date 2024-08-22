// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 12
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "rdi_breakpad_from_pdb"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "lib_rdi_format/rdi_format.h"
#include "lib_rdi_format/rdi_format.c"
#include "lib_rdi_format/rdi_format_parse.h"
#include "lib_rdi_format/rdi_format_parse.c"
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "rdi_make/rdi_make_local.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"
#include "rdi_from_pdb/rdi_from_pdb.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "rdi_make/rdi_make_local.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"
#include "rdi_from_pdb/rdi_from_pdb.c"

////////////////////////////////
//~ rjf: Baking Tasks

//- rjf: unit vmap baking

typedef struct P2B_BakeUnitVMapIn P2B_BakeUnitVMapIn;
struct P2B_BakeUnitVMapIn
{
  RDIM_UnitChunkList *units;
};

internal TS_TASK_FUNCTION_DEF(p2b_bake_unit_vmap_task__entry_point)
{
  P2B_BakeUnitVMapIn *in = (P2B_BakeUnitVMapIn *)p;
  RDIM_UnitVMapBakeResult *out = push_array(arena, RDIM_UnitVMapBakeResult, 1);
  *out = rdim_bake_unit_vmap(arena, in->units);
  return out;
}

//- rjf: line table baking

typedef struct P2B_BakeLineTablesIn P2B_BakeLineTablesIn;
struct P2B_BakeLineTablesIn
{
  RDIM_LineTableChunkList *line_tables;
};

internal TS_TASK_FUNCTION_DEF(p2b_bake_line_table_task__entry_point)
{
  P2B_BakeLineTablesIn *in = (P2B_BakeLineTablesIn *)p;
  RDIM_LineTableBakeResult *out = push_array(arena, RDIM_LineTableBakeResult, 1);
  *out = rdim_bake_line_tables(arena, in->line_tables);
  return out;
}

//- rjf: per-procedure chunk dumping

typedef struct P2B_DumpProcChunkIn P2B_DumpProcChunkIn;
struct P2B_DumpProcChunkIn
{
  RDI_VMapEntry *unit_vmap;
  U32 unit_vmap_count;
  U32 *unit_line_table_idxs;
  U64 unit_count;
  RDIM_LineTableBakeResult *line_tables_bake;
  RDIM_SymbolChunkNode *chunk;
};

internal TS_TASK_FUNCTION_DEF(p2b_dump_proc_chunk_task__entry_point)
{
  P2B_DumpProcChunkIn *in = (P2B_DumpProcChunkIn *)p;
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
  return out;
}

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  //- rjf: initialize state, unpack command line
  Arena *arena = arena_alloc();
  B32 do_help = (cmd_line_has_flag(cmdline, str8_lit("help")) ||
                 cmd_line_has_flag(cmdline, str8_lit("h")) ||
                 cmd_line_has_flag(cmdline, str8_lit("?")));
  P2R_User2Convert *user2convert = p2r_user2convert_from_cmdln(arena, cmdline);
  user2convert->flags &= ~(P2R_ConvertFlag_Types|P2R_ConvertFlag_UDTs);
  
  //- rjf: display help
  if(do_help || user2convert->errors.node_count != 0)
  {
    fprintf(stderr, "--- rdi_breakpad_from_pdb -----------------------------------------------------\n\n");
    
    fprintf(stderr, "This utility converts debug information from PDBs into the textual Breakpad\n");
    fprintf(stderr, "symbol information format, used for various external utilities, using the RAD\n");
    fprintf(stderr, "Debug Info conversion systems. The following arguments are accepted:\n\n");
    
    fprintf(stderr, "--exe:<path> [optional] Specifies the path of the executable file for which the\n");
    fprintf(stderr, "                        debug info was generated.\n");
    fprintf(stderr, "--pdb:<path>            Specifies the path of the PDB debug info file to\n");
    fprintf(stderr, "                        convert.\n");
    fprintf(stderr, "--out:<path>            Specifies the path at which the output Breakpad debug\n");
    fprintf(stderr, "                        info will be written.\n\n");
    
    if(!do_help)
    {
      for(String8Node *n = user2convert->errors.first; n != 0; n = n->next)
      {
        fprintf(stderr, "error(input): %.*s\n", str8_varg(n->string));
      }
    }
    os_abort(0);
  }
  
  //- rjf: convert
  P2R_Convert2Bake *convert2bake = 0;
  ProfScope("convert")
  {
    convert2bake = p2r_convert(arena, user2convert);
  }
  
  //- rjf: dump breakpad text
  String8List dump = {0};
  ProfScope("dump breakpad text")
  {
    RDIM_BakeParams *params = &convert2bake->bake_params;
    
    //- rjf: kick off unit vmap baking
    P2B_BakeUnitVMapIn bake_unit_vmap_in = {&params->units};
    TS_Ticket bake_unit_vmap_ticket = ts_kickoff(p2b_bake_unit_vmap_task__entry_point, 0, &bake_unit_vmap_in);
    
    //- rjf: kick off line-table baking
    P2B_BakeLineTablesIn bake_line_tables_in = {&params->line_tables};
    TS_Ticket bake_line_tables_ticket = ts_kickoff(p2b_bake_line_table_task__entry_point, 0, &bake_line_tables_in);
    
    //- rjf: build unit -> line table idx array
    U64 unit_count = params->units.total_count;
    U32 *unit_line_table_idxs = push_array(arena, U32, unit_count+1);
    {
      U64 dst_idx = 1;
      for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
      {
        for(U64 n_idx = 0; n_idx < n->count; n_idx += 1, dst_idx += 1)
        {
          unit_line_table_idxs[dst_idx] = rdim_idx_from_line_table(n->v[n_idx].line_table);
        }
      }
    }
    
    //- rjf: dump MODULE record
    str8_list_pushf(arena, &dump, "MODULE windows x86_64 %I64x %S\n", params->top_level_info.exe_hash, params->top_level_info.exe_name);
    
    //- rjf: dump FILE records
    ProfScope("dump FILE records")
    {
      for(RDIM_SrcFileChunkNode *n = params->src_files.first; n != 0; n = n->next)
      {
        for(U64 idx = 0; idx < n->count; idx += 1)
        {
          U64 file_idx = rdim_idx_from_src_file(&n->v[idx]);
          String8 src_path = n->v[idx].normal_full_path;
          str8_list_pushf(arena, &dump, "FILE %I64u %S\n", file_idx, src_path);
        }
      }
    }
    
    //- rjf: join unit vmap
    ProfBegin("join unit vmap");
    RDIM_UnitVMapBakeResult *bake_unit_vmap_out = ts_join_struct(bake_unit_vmap_ticket, max_U64, RDIM_UnitVMapBakeResult);
    RDI_VMapEntry *unit_vmap = bake_unit_vmap_out->vmap.vmap;
    U32 unit_vmap_count = bake_unit_vmap_out->vmap.count;
    ProfEnd();
    
    //- rjf: join line tables
    ProfBegin("join line table");
    RDIM_LineTableBakeResult *bake_line_tables_out = ts_join_struct(bake_line_tables_ticket, max_U64, RDIM_LineTableBakeResult);
    ProfEnd();
    
    //- rjf: kick off FUNC & line record dump tasks
    P2B_DumpProcChunkIn *dump_proc_chunk_in = push_array(arena, P2B_DumpProcChunkIn, params->procedures.chunk_count);
    TS_Ticket *dump_proc_chunk_tickets = push_array(arena, TS_Ticket, params->procedures.chunk_count);
    ProfScope("kick off FUNC & line record dump tasks")
    {
      U64 task_idx = 0;
      for(RDIM_SymbolChunkNode *n = params->procedures.first; n != 0; n = n->next, task_idx += 1)
      {
        dump_proc_chunk_in[task_idx].unit_vmap            = unit_vmap;
        dump_proc_chunk_in[task_idx].unit_vmap_count      = unit_vmap_count;
        dump_proc_chunk_in[task_idx].unit_line_table_idxs = unit_line_table_idxs;
        dump_proc_chunk_in[task_idx].unit_count           = unit_count;
        dump_proc_chunk_in[task_idx].line_tables_bake     = bake_line_tables_out;
        dump_proc_chunk_in[task_idx].chunk                = n;
        dump_proc_chunk_tickets[task_idx] = ts_kickoff(p2b_dump_proc_chunk_task__entry_point, 0, &dump_proc_chunk_in[task_idx]);
      }
    }
    
    //- rjf: join FUNC & line record dump tasks
    ProfScope("join FUNC & line record dump tasks")
    {
      for(U64 idx = 0; idx < params->procedures.chunk_count; idx += 1)
      {
        String8List *out = ts_join_struct(dump_proc_chunk_tickets[idx], max_U64, String8List);
        str8_list_concat_in_place(&dump, out);
      }
    }
  }
  
  //- rjf: bake
  String8 baked = {0};
  ProfScope("bake")
  {
    baked = str8_list_join(arena, &dump, 0);
  }
  
  //- rjf: write
  ProfScope("write")
  {
    OS_Handle output_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, user2convert->output_name);
    os_file_write(output_file, r1u64(0, baked.size), baked.str);
    os_file_close(output_file);
  }
}
