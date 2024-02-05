////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "mdesk/mdesk.h"
#include "hash_store/hash_store.h"
#include "file_stream/file_stream.h"
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
#include "file_stream/file_stream.c"
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

#include <dbghelp.h>

#undef OS_WINDOWS // shlwapi uses its own OS_WINDOWS include inside
#include <shlwapi.h>

internal B32 g_is_quiet = 0;

internal HRESULT WINAPI
win32_dialog_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LONG_PTR data)
{
  if(msg == TDN_HYPERLINK_CLICKED)
  {
    ShellExecuteW(NULL, L"open", (LPWSTR)lparam, NULL, NULL, SW_SHOWNORMAL);
  }
  return S_OK;
}

internal LONG WINAPI
win32_exception_filter(EXCEPTION_POINTERS* exception_ptrs)
{
  if(g_is_quiet)
  {
    ExitProcess(1);
  }
  
  static volatile LONG first = 0;
  if(InterlockedCompareExchange(&first, 1, 0) != 0)
  {
    // prevent failures in other threads to popup same message box
    // this handler just shows first thread that crashes
    // we are terminating afterwards anyway
    for (;;) Sleep(1000);
  }
  
  WCHAR buffer[4096] = {0};
  int buflen = 0;
  
  DWORD exception_code = exception_ptrs->ExceptionRecord->ExceptionCode;
  buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"A fatal exception (code 0x%x) occurred. The process is terminating.\n", exception_code);
  
  // load dbghelp dynamically just in case if it is missing
  HMODULE dbghelp = LoadLibraryA("dbghelp.dll");
  if(dbghelp)
  {
    DWORD (WINAPI *dbg_SymSetOptions)(DWORD SymOptions);
    BOOL (WINAPI *dbg_SymInitializeW)(HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess);
    BOOL (WINAPI *dbg_StackWalk64)(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                                   LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                                   PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                                   PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
    PVOID (WINAPI *dbg_SymFunctionTableAccess64)(HANDLE hProcess, DWORD64 AddrBase);
    DWORD64 (WINAPI *dbg_SymGetModuleBase64)(HANDLE hProcess, DWORD64 qwAddr);
    BOOL (WINAPI *dbg_SymFromAddrW)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFOW Symbol);
    BOOL (WINAPI *dbg_SymGetLineFromAddrW64)(HANDLE hProcess, DWORD64 dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINEW64 Line);
    BOOL (WINAPI *dbg_SymGetModuleInfoW64)(HANDLE hProcess, DWORD64 qwAddr, PIMAGEHLP_MODULEW64 ModuleInfo);
    
    *(FARPROC*)&dbg_SymSetOptions            = GetProcAddress(dbghelp, "SymSetOptions");
    *(FARPROC*)&dbg_SymInitializeW           = GetProcAddress(dbghelp, "SymInitializeW");
    *(FARPROC*)&dbg_StackWalk64              = GetProcAddress(dbghelp, "StackWalk64");
    *(FARPROC*)&dbg_SymFunctionTableAccess64 = GetProcAddress(dbghelp, "SymFunctionTableAccess64");
    *(FARPROC*)&dbg_SymGetModuleBase64       = GetProcAddress(dbghelp, "SymGetModuleBase64");
    *(FARPROC*)&dbg_SymFromAddrW             = GetProcAddress(dbghelp, "SymFromAddrW");
    *(FARPROC*)&dbg_SymGetLineFromAddrW64    = GetProcAddress(dbghelp, "SymGetLineFromAddrW64");
    *(FARPROC*)&dbg_SymGetModuleInfoW64      = GetProcAddress(dbghelp, "SymGetModuleInfoW64");
    
    if(dbg_SymSetOptions && dbg_SymInitializeW && dbg_StackWalk64 && dbg_SymFunctionTableAccess64 && dbg_SymGetModuleBase64 && dbg_SymFromAddrW && dbg_SymGetLineFromAddrW64 && dbg_SymGetModuleInfoW64)
    {
      HANDLE process = GetCurrentProcess();
      HANDLE thread = GetCurrentThread();
      CONTEXT* context = exception_ptrs->ContextRecord;
      
      dbg_SymSetOptions(SYMOPT_EXACT_SYMBOLS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
      if(dbg_SymInitializeW(process, L"", TRUE))
      {
        // check that raddbg.pdb file is good
        B32 raddbg_pdb_valid = 0;
        {
          IMAGEHLP_MODULEW64 module = {0};
          module.SizeOfStruct = sizeof(module);
          if(dbg_SymGetModuleInfoW64(process, (DWORD64)&win32_exception_filter, &module))
          {
            raddbg_pdb_valid = (module.SymType == SymPdb);
          }
        }
        
        if(!raddbg_pdb_valid)
        {
          buflen += wnsprintfW(buffer + buflen, sizeof(buffer) - buflen,
                               L"\nraddbg.pdb debug file is not valid or not found. Please rebuild binary to get call stack.\n");
        }
        else
        {
          STACKFRAME64 frame = {0};
          DWORD image_type;
#if defined(_M_AMD64)
          image_type = IMAGE_FILE_MACHINE_AMD64;
          frame.AddrPC.Offset = context->Rip;
          frame.AddrPC.Mode = AddrModeFlat;
          frame.AddrFrame.Offset = context->Rbp;
          frame.AddrFrame.Mode = AddrModeFlat;
          frame.AddrStack.Offset = context->Rsp;
          frame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_ARM64)
          image_type = IMAGE_FILE_MACHINE_ARM64;
          frame.AddrPC.Offset = context->Pc;
          frame.AddrPC.Mode = AddrModeFlat;
          frame.AddrFrame.Offset = context->Fp;
          frame.AddrFrame.Mode = AddrModeFlat;
          frame.AddrStack.Offset = context->Sp;
          frame.AddrStack.Mode = AddrModeFlat;
#else
#  error Architecture not supported!
#endif
          
          for(U32 idx=0; ;idx++)
          {
            const U32 max_frames = 32;
            if(idx == max_frames)
            {
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"...");
              break;
            }
            
            if(!dbg_StackWalk64(image_type, process, thread, &frame, context, 0, dbg_SymFunctionTableAccess64, dbg_SymGetModuleBase64, 0))
            {
              break;
            }
            
            U64 address = frame.AddrPC.Offset;
            if(address == 0)
            {
              break;
            }
            
            if(idx==0)
            {
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen,
                                   L"\nPress Ctrl+C to copy this text to clipboard, then create a new issue in\n"
                                   L"<a href=\"%S\">%S</a>\n\n", RADDBG_GITHUB_ISSUES, RADDBG_GITHUB_ISSUES);
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"Call stack:\n");
            }
            
            buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"%u. [0x%I64x]", idx + 1, address);
            
            struct {
              SYMBOL_INFOW info;
              WCHAR name[MAX_SYM_NAME];
            } symbol = {0};
            
            symbol.info.SizeOfStruct = sizeof(symbol.info);
            symbol.info.MaxNameLen = MAX_SYM_NAME;
            
            DWORD64 displacement = 0;
            if(dbg_SymFromAddrW(process, address, &displacement, &symbol.info))
            {
              buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L" %s +%u", symbol.info.Name, (DWORD)displacement);
              
              IMAGEHLP_LINEW64 line = {0};
              line.SizeOfStruct = sizeof(line);
              
              DWORD line_displacement = 0;
              if(dbg_SymGetLineFromAddrW64(process, address, &line_displacement, &line))
              {
                buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L", %s line %u", PathFindFileNameW(line.FileName), line.LineNumber);
              }
            }
            else
            {
              IMAGEHLP_MODULEW64 module = {0};
              module.SizeOfStruct = sizeof(module);
              if(dbg_SymGetModuleInfoW64(process, address, &module))
              {
                buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L" %s", module.ModuleName);
              }
            }
            
            buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"\n");
          }
        }
      }
    }
  }

  buflen += wnsprintfW(buffer + buflen, ArrayCount(buffer) - buflen, L"\nVersion: %S%S", RADDBG_VERSION_STRING_LITERAL, RADDBG_GIT_STR);
  
  TASKDIALOGCONFIG dialog = {0};
  dialog.cbSize = sizeof(dialog);
  dialog.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
  dialog.pszMainIcon = TD_ERROR_ICON;
  dialog.dwCommonButtons = TDCBF_CLOSE_BUTTON;
  dialog.pszWindowTitle = L"Fatal Exception";
  dialog.pszContent = buffer;
  dialog.pfCallback = &win32_dialog_callback;
  TaskDialogIndirect(&dialog, 0, 0, 0);
  
  ExitProcess(1);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  SetUnhandledExceptionFilter(&win32_exception_filter);
  
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
    if(str8_match(arg8, str8_lit("--quiet"), StringMatchFlag_CaseInsensitive))
    {
      g_is_quiet = 1;
    }
    argv[i] = (char *)arg8.str;
  }
  entry_point(argc, argv);
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
