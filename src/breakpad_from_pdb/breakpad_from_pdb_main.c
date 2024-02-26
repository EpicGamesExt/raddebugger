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
#include "lib_raddbgi_format/raddbgi_format.c"

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
    
    //- rjf: dump MODULE record
    str8_list_pushf(arena, &dump, "MODULE windows x86_64 %I64x %S\n", params->top_level_info.exe_hash, params->top_level_info.exe_name);
    
    //- rjf: dump FILE records
    for(RDIM_SrcFileChunkNode *n = params->src_files.first; n != 0; n = n->next)
    {
      for(U64 idx = 0; idx < n->count; idx += 1)
      {
        U64 file_idx = rdim_idx_from_src_file(&n->v[idx]);
        String8 src_path = n->v[idx].normal_full_path;
        str8_list_pushf(arena, &dump, "FILE %I64u %S\n", file_idx, src_path);
      }
    }
    
    //- rjf: dump FUNC records
    for(RDIM_SymbolChunkNode *n = params->procedures.first; n != 0; n = n->next)
    {
      for(U64 idx = 0; idx < n->count; idx += 1)
      {
        RDIM_Symbol *proc = &n->v[idx];
        RDIM_Scope *root_scope = proc->root_scope;
        if(root_scope != 0 && root_scope->voff_ranges.first != 0)
        {
          RDIM_Rng1U64 voff_range = root_scope->voff_ranges.first->v;
          str8_list_pushf(arena, &dump, "FUNC %I64x %I64x %I64x %S\n", voff_range.min, voff_range.max-voff_range.min, 0ull, proc->name);
        }
      }
    }
  }
  
  //- rjf: write
  ProfScope("write")
  {
    OS_Handle output_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, user2convert->output_name);
    U64 off = 0;
    for(String8Node *n = dump.first; n != 0; n = n->next)
    {
      os_file_write(output_file, r1u64(off, off+n->string.size), n->string.str);
      off += n->string.size;
    }
    os_file_close(output_file);
  }
}
