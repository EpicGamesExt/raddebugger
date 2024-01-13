////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "mdesk/mdesk.h"
#include "hash_store/hash_store.h"
#include "text_cache/text_cache.h"
#include "path/path.h"
#include "txti/txti.h"
#include "coff/coff.h"
#include "pe/pe.h"
#include "raddbg_format/raddbg_format.h"
#include "raddbg_format/raddbg_format_parse.h"
#include "raddbg_cons/raddbg_cons.h"
#include "raddbg_convert/pdb/raddbg_coff.h"
#include "raddbg_convert/pdb/raddbg_codeview.h"
#include "raddbg_convert/pdb/raddbg_msf.h"
#include "raddbg_convert/pdb/raddbg_pdb.h"
#include "raddbg_convert/pdb/raddbg_coff_conversion.h"
#include "raddbg_convert/pdb/raddbg_codeview_conversion.h"
#include "raddbg_convert/pdb/raddbg_from_pdb.h"
#include "raddbg_convert/pdb/raddbg_codeview_stringize.h"
#include "raddbg_convert/pdb/raddbg_pdb_stringize.h"
#include "regs/regs.h"
#include "regs/raddbg/regs_raddbg.h"
#include "type_graph/type_graph.h"
#include "dbgi/dbgi.h"
#include "demon/demon_inc.h"
#include "eval/eval_compiler.h"
#include "eval/eval_machine.h"
#include "eval/eval_parser.h"
#include "unwind/unwind.h"
#include "ctrl/ctrl_inc.h"
#include "dasm/dasm.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "texture_cache/texture_cache.h"
#include "geo_cache/geo_cache.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "df/df_inc.h"
#include "raddbg.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "mdesk/mdesk.c"
#include "hash_store/hash_store.c"
#include "text_cache/text_cache.c"
#include "path/path.c"
#include "txti/txti.c"
#include "coff/coff.c"
#include "pe/pe.c"
#include "raddbg_format/raddbg_format.c"
#include "raddbg_format/raddbg_format_parse.c"
#include "raddbg_cons/raddbg_cons.c"
#include "raddbg_convert/pdb/raddbg_msf.c"
#include "raddbg_convert/pdb/raddbg_codeview.c"
#include "raddbg_convert/pdb/raddbg_pdb.c"
#include "raddbg_convert/pdb/raddbg_coff_conversion.c"
#include "raddbg_convert/pdb/raddbg_codeview_conversion.c"
#include "raddbg_convert/pdb/raddbg_codeview_stringize.c"
#include "raddbg_convert/pdb/raddbg_pdb_stringize.c"
#include "raddbg_convert/pdb/raddbg_from_pdb.c"
#include "regs/regs.c"
#include "regs/raddbg/regs_raddbg.c"
#include "type_graph/type_graph.c"
#include "dbgi/dbgi.c"
#include "demon/demon_inc.c"
#include "eval/eval_compiler.c"
#include "eval/eval_machine.c"
#include "eval/eval_parser.c"
#include "unwind/unwind.c"
#include "ctrl/ctrl_inc.c"
#include "dasm/dasm.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "texture_cache/texture_cache.c"
#include "geo_cache/geo_cache.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "df/df_inc.c"
#include "raddbg.c"

////////////////////////////////
//~ rjf: Low-Level Entry Points

//- rjf: windows
#if OS_WINDOWS

global DWORD g_saved_exception_code = 0;

internal DWORD
win32_exception_filter(DWORD dwExceptionCode)
{
  g_saved_exception_code = dwExceptionCode;
  return EXCEPTION_EXECUTE_HANDLER;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  HANDLE output_handles[3] =
  {
    GetStdHandle(STD_INPUT_HANDLE),
    GetStdHandle(STD_OUTPUT_HANDLE),
    GetStdHandle(STD_ERROR_HANDLE),
  };
  for(U64 idx = 0; idx < ArrayCount(output_handles); idx += 1)
  {
    B32 duplicate = 0;
    for(U64 idx2 = 0; idx2 < idx; idx2 += 1)
    {
      if(output_handles[idx2] == output_handles[idx])
      {
        duplicate = 1;
        break;
      }
    }
    if(duplicate)
    {
      output_handles[idx] = 0;
    }
  }
  for(U64 idx = 0; idx < ArrayCount(output_handles); idx += 1)
  {
    if(output_handles[idx] != 0)
    {
      CloseHandle(output_handles[idx]);
    }
  }
  SetStdHandle(STD_INPUT_HANDLE, 0);
  SetStdHandle(STD_OUTPUT_HANDLE, 0);
  SetStdHandle(STD_ERROR_HANDLE, 0);
  static TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
  Arena *perm_arena = arena_alloc();
  WCHAR *command_line = GetCommandLineW();
  int argc;
  WCHAR **argv_16 = CommandLineToArgvW(command_line, &argc);
  char **argv = push_array(perm_arena, char *, argc);
  for(int i = 0; i < argc; i += 1)
  {
    String16 arg16 = str16_cstring((U16 *)argv_16[i]);
    String8 arg8 = str8_from_16(perm_arena, arg16);
    argv[i] = (char *)arg8.str;
  }
  __try
  {
    entry_point(argc, argv);
  }
  __except(win32_exception_filter(GetExceptionCode()))
  {
    char buffer[256] = {0};
    raddbg_snprintf(buffer, sizeof(buffer), "A fatal exception (code 0x%x) occurred. The process is terminating.", (U32)g_saved_exception_code);
    os_graphical_message(1, str8_lit("Fatal Exception"), str8_cstring(buffer));
    ExitProcess(1);
  }
  return 0;
}

//- rjf: linux
#elif OS_LINUX

int main(int argument_count, char **arguments)
{
  static TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
  entry_point(argument_count, arguments);
  return 0;
}

#endif
