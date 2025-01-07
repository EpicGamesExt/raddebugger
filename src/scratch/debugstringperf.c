#include <windows.h>

DWORD thread_entry_point(void *p)
{
  for(int i = 0; i < 100000; i += 1)
  {
    OutputDebugString("this is a test\n");
  }
  return 0;
}

int main(int argc, char **argv)
{
  HANDLE threads[] =
  {
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
    CreateThread(0, 0, thread_entry_point, 0, 0, 0),
  };
  for(int i = 0; i < sizeof(threads)/sizeof(threads[0]); i += 1)
  {
    WaitForSingleObject(threads[i], INFINITE);
  }
  return 0;
}
