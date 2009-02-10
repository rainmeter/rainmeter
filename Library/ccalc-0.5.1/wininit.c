#include <windows.h>

extern int WINAPI dll_entry (HANDLE h, DWORD reason, void *ptr)
{
  return 1;
}
