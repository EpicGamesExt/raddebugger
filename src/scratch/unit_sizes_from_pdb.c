#define BUILD_CONSOLE_INTERFACE 1
#define NO_ASYNC 1

#include "base/base_inc.h"
#include "x64/x64.h"
#include "linker/hash_table.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "coff/coff_obj_writer.h"
#include "coff/coff_lib_writer.h"
#include "pe/pe.h"
#include "pe/pe_section_flags.h"
#include "pe/pe_make_import_table.h"
#include "pe/pe_make_export_table.h"
#include "pe/pe_make_debug_dir.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "msvc_crt/msvc_crt.h"
#include "llvm/llvm.h"
#include "dwarf/dwarf.h"
#include "dwarf/x64/dwarf_x64.h"

#include "base/base_inc.c"
#include "x64/x64.c"
#include "linker/hash_table.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "coff/coff_obj_writer.c"
#include "coff/coff_lib_writer.c"
#include "pe/pe.c"
#include "pe/pe_make_import_table.c"
#include "pe/pe_make_export_table.c"
#include "pe/pe_make_debug_dir.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "msvc_crt/msvc_crt.c"
#include "llvm/llvm.c"
#include "dwarf/x64/dwarf_x64.c"

int is_before(void *a, void *b)
{ 
  return str8_compar_ignore_case(&(*(PDB_CompUnit**)a)->obj_name, &(*(PDB_CompUnit**)b)->obj_name) < 0;
}

int sc_is_before(void *a, void *b)
{
  return (*(PDB_CompUnitContribution **)a)->voff_first < (*(PDB_CompUnitContribution **)b)->voff_first;
}

internal void
entry_point(CmdLine *cmdline)
{
  Temp scratch = scratch_begin(0,0);

  String8 data = data_from_file_path(scratch.arena, cmdline->inputs.first->string);
  if (data.size == 0) {
    fprintf(stderr, "ERROR: input file was not read %.*s\n", str8_varg(cmdline->inputs.first->string));
    goto exit;
  }

  // parse msf
  MSF_Parsed *msf = msf_parsed_from_data(scratch.arena, data);

  // parse dbi
  String8       dbi_data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
  PDB_DbiParsed *dbi = pdb_dbi_from_data(scratch.arena, dbi_data);

  // parse image section table
  COFF_SectionHeaderArray coff_sections = {0};
  MSF_StreamNumber section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
  String8 section_data = msf_data_from_stream(msf, section_stream);
  coff_sections = pdb_coff_section_array_from_data(scratch.arena, section_data);

  // parse section contribs
  String8 contribs_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_SecCon);
  PDB_CompUnitContributionArray comp_unit_contributions = pdb_comp_unit_contribution_array_from_data(scratch.arena, contribs_data, coff_sections);

  // parse compile units
  String8 comp_units_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
  PDB_CompUnitArray *comp_units = pdb_comp_unit_array_from_data(scratch.arena, comp_units_data);

  HashMap sc_hm = {0};
  
  typedef struct SC_Node {
    PDB_CompUnitContribution *v;
    struct SC_Node *next;
  } SC_Node;

  typedef struct {
    U64 count;
    SC_Node *first;
    SC_Node *last;
  } SC_List;

  U64 *sizes = push_array(scratch.arena, U64, comp_units->count + 1);
  for EachIndex(i, comp_unit_contributions.count) {
    PDB_CompUnitContribution *sc = &comp_unit_contributions.contributions[i];

    U64 s = sc->voff_opl - sc->voff_first;
    sizes[sc->mod] += s;

    PDB_CompUnit *cu = comp_units->units[sc->mod];
    SC_List *list = hash_map_search_raw_raw(&sc_hm, cu);
    if (list == 0) {
      list = push_array(scratch.arena, SC_List, 1);
      hash_map_push_raw_raw(scratch.arena, &sc_hm, cu, list);
    }
    SC_Node *n = push_array(scratch.arena, SC_Node, 1);
    n->v = sc;
    SLLQueuePush(list->first, list->last, n);
    list->count += 1;
  }

  PDB_CompUnit **sorted_comp_units = push_array(scratch.arena, PDB_CompUnit *, comp_units->count);
  for EachIndex(i, comp_units->count) { sorted_comp_units[i] = comp_units->units[i]; }
  radsort(sorted_comp_units, comp_units->count, is_before);

  HashMap hm = {0};
  for EachIndex(i, comp_units->count) { hash_map_push_raw_u64(scratch.arena, &hm, comp_units->units[i], i); }

  for (U64 i = 0; i < comp_units->count; ++i) {
    U64     *cu_idx  = hash_map_search_raw_u64(&hm, sorted_comp_units[i]);
    SC_List *sc_list = hash_map_search_raw_raw(&sc_hm, sorted_comp_units[i]);
    
    PDB_CompUnitContribution **cu_sc = 0;
    if (sc_list) {
      cu_sc = push_array(scratch.arena, PDB_CompUnitContribution *, sc_list->count);
      U64 sc_idx = 0;
      for EachNode(n, SC_Node, sc_list->first) {
        cu_sc[sc_idx++] = n->v;
      }
      radsort(cu_sc, sc_idx, sc_is_before);
    }

    fprintf(stdout, "0x%llx - %.*s %.*s\n", sizes[*cu_idx], str8_varg(sorted_comp_units[i]->obj_name), str8_varg(str8_skip_last_slash(sorted_comp_units[i]->group_name)));
    if (cu_sc) {
      for EachIndex(i, sc_list->count) {
        fprintf(stdout, "\t0x%llx 0x%llx\n", (unsigned long long)(cu_sc[i]->voff_opl - cu_sc[i]->voff_first), (unsigned long long)cu_sc[i]->voff_first);
      }
    }
  }

  exit:;
  scratch_end(scratch);
}

