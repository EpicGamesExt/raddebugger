#include <windows.h>
#include <winternl.h>
#include "mule_peb_trample_reload.c"

static void
HideModuleFromWindowsReload(HMODULE ModuleToFlush)
{
  /* NOTE(casey): Normally you cannot "reload" an executable module with the same name,
     because Windows checks a linked list of loaded modules and assumes that if
     it's already loaded, it doesn't need to reload it, even though it may have to because
     it has changed on disk.
     
     This solution to that problem comes from some excellent spelunking by Martins Mozeiko,
     who figured out that you could overwrite the filenames Windows stores in your process's
     loaded module table, thus thwarting the Windows filename check against loaded modules,
     allowing you to reload an existing module that has changed without requiring it to
     have a different filename!
  */
  
  PEB *Peb = (PEB *)__readgsqword(offsetof(TEB, ProcessEnvironmentBlock));
  LIST_ENTRY *Head = &Peb->Ldr->InMemoryOrderModuleList;
  for(LIST_ENTRY *Entry = Head->Flink;
      Entry != Head;
      Entry = Entry->Flink)
  {
    LDR_DATA_TABLE_ENTRY *Mod = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    if(Mod->DllBase == ModuleToFlush)
    {
      ZeroMemory(Mod->FullDllName.Buffer, Mod->FullDllName.Length);
      Mod->DllBase = 0;
      break;
    }
  }
}

int main(int argument_count, char **arguments)
{
  char *exe_name = arguments[0];
  HANDLE last_module = GetModuleHandle(0);
  int (*loop_iteration_function)(int it) = (int (*)(int))GetProcAddress(last_module, "loop_iteration");
  FILETIME last_filetime = {0};
  int should_exit = 0;
  for(int it = 0; !should_exit; it += 1)
  {
    int result = loop_iteration_function(it);
    printf("%i\n", result);
    Sleep(50);
    FILETIME current_filetime = {0};
    HANDLE current_exe_file = CreateFile(exe_name, 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    GetFileTime(current_exe_file, 0, 0, &current_filetime);
    CloseHandle(current_exe_file);
    if(it != 0 && CompareFileTime(&last_filetime, &current_filetime) < 0)
    {
      HideModuleFromWindowsReload(last_module);
      last_module = LoadLibrary(arguments[0]);
      loop_iteration_function = (int (*)(int))GetProcAddress(last_module, "loop_iteration");
    }
    last_filetime = current_filetime;
  }
  return 0;
}
