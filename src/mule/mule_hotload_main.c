#include <stdint.h>
#include <windows.h>

int main(int argc, char **argv)
{
  int lib_loaded = 0;
  HANDLE lib = {0};
  FILETIME lib_last_filetime = {0};
  int (*get_number)(void) = 0;
  for(;;)
  {
    //- rjf: hot-load dll
    {
      HANDLE file = CreateFileA("mule_hotload_module.dll", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      FILETIME modified = {0};
      if(GetFileTime(file, 0, 0, &modified) &&
         CompareFileTime(&lib_last_filetime, &modified) == -1)
      {
        for(int reloaded = 0; !reloaded;)
        {
          if(lib_loaded)
          {
            FreeLibrary(lib);
            lib_loaded = 0;
          }
          BOOL copy_worked = CopyFile("mule_hotload_module.dll", "mule_hotload_module_temp.dll", 0);
          lib = LoadLibraryA("mule_hotload_module_temp.dll");
          if(lib != INVALID_HANDLE_VALUE)
          {
            reloaded = 1;
            lib_last_filetime = modified;
            get_number = (int(*)(void))GetProcAddress(lib, "get_number");
            lib_loaded = 1;
          }
        }
      }
      CloseHandle(file);
    }
    int number = get_number();
    printf("got a number: %i\n", number);
    if(number == 0)
    {
      break;
    }
    Sleep(1000);
  }
  return 0;
}
