////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 8
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "breakpad_from_pdb"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "lib_raddbgi_format/raddbgi_format.h"
#include "lib_raddbgi_format/raddbgi_format_parse.h"
#include "lib_raddbgi_format/raddbgi_format.c"
#include "lib_raddbgi_format/raddbgi_format_parse.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "raddbgi_make_local/raddbgi_make_local.h"
#include "coff/coff.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"
#include "raddbgi_from_pdb/raddbgi_from_pdb.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "raddbgi_make_local/raddbgi_make_local.c"
#include "coff/coff.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"
#include "raddbgi_from_pdb/raddbgi_from_pdb.c"

////////////////////////////////
//~ rjf: Baking Tasks

//- rjf: unit vmap baking

typedef struct P2B_BakeUnitVMapIn P2B_BakeUnitVMapIn;
struct P2B_BakeUnitVMapIn
{
  RDIM_BakeParams *params;
};

typedef struct P2B_BakeUnitVMapOut P2B_BakeUnitVMapOut;
struct P2B_BakeUnitVMapOut
{
  RDI_VMapEntry *vmap_entries;
  RDI_U64 vmap_entries_count;
};

internal void *
p2b_bake_unit_vmap_task__entry_point(Arena *arena, void *p)
{
  P2B_BakeUnitVMapIn *in = (P2B_BakeUnitVMapIn *)p;
  P2B_BakeUnitVMapOut *out = push_array(arena, P2B_BakeUnitVMapOut, 1);
  RDIM_BakeSectionList sections = rdim_bake_unit_vmap_section_list_from_params(arena, in->params);
  RDIM_BakeSection *vmap_section = 0;
  for(RDIM_BakeSectionNode *n = sections.first; n != 0 && vmap_section == 0; n = n->next)
  {
    switch(n->v.tag)
    {
      default:{}break;
      case RDI_DataSectionTag_UnitVmap:{vmap_section = &n->v;}break;
    }
  }
  if(vmap_section != 0)
  {
    out->vmap_entries = (RDI_VMapEntry *)vmap_section->data;
    out->vmap_entries_count = vmap_section->size/sizeof(RDI_VMapEntry);
  }
  return out;
}

//- rjf: per-unit baking

typedef struct P2B_BakeUnitIn P2B_BakeUnitIn;
struct P2B_BakeUnitIn
{
  RDIM_Unit *unit;
};

typedef struct P2B_BakeUnitOut P2B_BakeUnitOut;
struct P2B_BakeUnitOut
{
  U64 unit_line_count;
  U64 *unit_line_voffs;
  RDI_Line *unit_lines;
};

internal void *
p2b_bake_unit_task__entry_point(Arena *arena, void *p)
{
  P2B_BakeUnitIn *in = (P2B_BakeUnitIn *)p;
  P2B_BakeUnitOut *out = push_array(arena, P2B_BakeUnitOut, 1);
  RDIM_BakeSectionList sections = rdim_bake_section_list_from_unit(arena, in->unit);
  RDIM_BakeSection *voffs_section = 0;
  RDIM_BakeSection *lines_section = 0;
  for(RDIM_BakeSectionNode *n = sections.first; n != 0; n = n->next)
  {
    switch(n->v.tag)
    {
      default:{}break;
      case RDI_DataSectionTag_LineInfoVoffs:{voffs_section = &n->v;}break;
      case RDI_DataSectionTag_LineInfoData: {lines_section = &n->v;}break;
    }
  }
  if(voffs_section != 0 && lines_section != 0)
  {
    out->unit_line_count = lines_section->size/sizeof(RDI_Line);
    out->unit_line_voffs = (U64 *)voffs_section->data;
    out->unit_lines      = (RDI_Line *)lines_section->data;
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
  P2R_User2Convert *user2convert = p2r_user2convert_from_cmdln(arena, cmdline);
  
  //- rjf: display errors with input
  if(user2convert->errors.node_count > 0 && !user2convert->hide_errors.input)
  {
    for(String8Node *n = user2convert->errors.first; n != 0; n = n->next)
    {
      fprintf(stderr, "error(input): %.*s\n", str8_varg(n->string));
    }
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
    P2B_BakeUnitVMapIn bake_unit_vmap_in = {params};
    TS_Ticket bake_unit_vmap_ticket = ts_kickoff(p2b_bake_unit_vmap_task__entry_point, 0, &bake_unit_vmap_in);
    
    //- rjf: kick off per-unit baking
    P2B_BakeUnitIn *bake_units_in = push_array(arena, P2B_BakeUnitIn, params->units.total_count+1);
    TS_Ticket *bake_units_tickets = push_array(arena, TS_Ticket, params->units.total_count+1);
    {
      U64 idx = 1;
      for(RDIM_UnitChunkNode *n = params->units.first; n != 0; n = n->next)
      {
        for(U64 chunk_idx = 0; chunk_idx < n->count; chunk_idx += 1, idx += 1)
        {
          bake_units_in[chunk_idx].unit = &n->v[chunk_idx];
          bake_units_tickets[idx] = ts_kickoff(p2b_bake_unit_task__entry_point, 0, &bake_units_in[chunk_idx]);
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
    P2B_BakeUnitVMapOut *bake_unit_vmap_out = ts_join_struct(bake_unit_vmap_ticket, max_U64, P2B_BakeUnitVMapOut);
    RDI_VMapEntry *unit_vmap = bake_unit_vmap_out->vmap_entries;
    U32 unit_vmap_count = (U32)(bake_unit_vmap_out->vmap_entries_count);
    
    //- rjf: join units
    P2B_BakeUnitOut **bake_units_out = push_array(arena, P2B_BakeUnitOut*, params->units.total_count+1);
    for(U64 idx = 1; idx < params->units.total_count+1; idx += 1)
    {
      bake_units_out[idx] = ts_join_struct(bake_units_tickets[idx], max_U64, P2B_BakeUnitOut);
    }
    
    //- rjf: dump FUNC & line records
    ProfScope("dump FUNC & line records")
    {
      for(RDIM_SymbolChunkNode *n = params->procedures.first; n != 0; n = n->next)
      {
        for(U64 idx = 0; idx < n->count; idx += 1)
        {
          // NOTE(rjf): breakpad does not support multiple voff ranges per procedure.
          RDIM_Symbol *proc = &n->v[idx];
          RDIM_Scope *root_scope = proc->root_scope;
          if(root_scope != 0 && root_scope->voff_ranges.first != 0)
          {
            // rjf: dump function record
            RDIM_Rng1U64 voff_range = root_scope->voff_ranges.first->v;
            str8_list_pushf(arena, &dump, "FUNC %I64x %I64x %I64x %S\n", voff_range.min, voff_range.max-voff_range.min, 0ull, proc->name);
            
            // rjf: dump function lines
            U64 unit_idx = rdi_vmap_idx_from_voff(unit_vmap, unit_vmap_count, voff_range.min);
            if(0 < unit_idx && unit_idx <= params->units.total_count)
            {
              // rjf: unpack unit line info
              P2B_BakeUnitOut *bake_unit_out = bake_units_out[unit_idx];
              RDI_ParsedLineInfo line_info = {bake_unit_out->unit_line_voffs, bake_unit_out->unit_lines, 0, bake_unit_out->unit_line_count, 0};
              for(U64 voff = voff_range.min, last_voff = 0;
                  voff < voff_range.max && voff > last_voff;)
              {
                RDI_U64 line_info_idx = rdi_line_info_idx_from_voff(&line_info, voff);
                if(line_info_idx < line_info.count)
                {
                  RDI_Line *line = &line_info.lines[line_info_idx];
                  U64 line_voff_min = line_info.voffs[line_info_idx];
                  U64 line_voff_opl = line_info.voffs[line_info_idx+1];
                  str8_list_pushf(arena, &dump, "%I64x %I64x %I64u %I64u\n",
                                  line_voff_min,
                                  line_voff_opl-line_voff_min,
                                  (U64)line->line_num,
                                  (U64)line->file_idx);
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
