#ifndef ___PLUGIN__H___
#define ___PLUGIN__H___

#include <nsis/pluginapi.h> // nsis plugin

// Always use system* functions to keep the size down
#define pushstring error(use system_pushstring)
#undef pushint
#define pushint error(use system_pushint)
#define pushintptr error(use system_pushintptr)

#undef myatoi
#define myatoi(str) ( (int) myatoi64(str) ) 
#define system_pushint(v) system_pushintptr((INT_PTR)(v))
#define popintptr system_popintptr
#ifdef _WIN64
#	define StrToIntPtr(str) ( (INT_PTR)myatoi64((str)) )
#	define IntPtrToStr(p,buf) myitoa64((p),(buf))
#else
#	define StrToIntPtr(str) ( (INT_PTR)myatoi((str)) )
#	define IntPtrToStr(p,buf) myitoa64((p),(buf))
#endif

#define BUGBUG64(brokencast) (brokencast) // Cast that needs fixing on x64

#define PLUGINFUNCTION(name) \
  void __declspec(dllexport) name( \
    HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra) { \
  /*g_hwndParent=hwndParent;*/ \
  EXDLL_INIT(); \
  extra->RegisterPluginCallback(g_hInstance, NSISCallback);
#define PLUGINFUNCTIONEND }

#define PLUGINFUNCTIONSHORT(name) void __declspec(dllexport) name(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop) { \
  g_stringsize=string_size; \
  g_stacktop=stacktop; 

extern TCHAR *AllocStr(TCHAR *str);
extern void myitoa64(__int64 i, TCHAR *buffer);
extern TCHAR *AllocString();
#define system_getuservariableptr(varnum) ( (g_variables)+(varnum)*(g_stringsize) )
extern TCHAR *system_getuservariable(int varnum); // NOTE: This dupes with GlobalAlloc!
extern TCHAR *system_setuservariable(int varnum, TCHAR *var);
extern TCHAR* system_popstring();  // NULL - stack empty
extern TCHAR* system_pushstring(TCHAR *str);
extern __int64 myatoi64(TCHAR *s);
extern INT_PTR system_popintptr();  // -1 -> stack empty
extern void system_pushintptr(INT_PTR value);

extern HANDLE GlobalCopy(HANDLE Old);
extern void *copymem(void *output, void *input, size_t cbSize);

extern UINT_PTR NSISCallback(enum NSPIM);

extern HWND g_hwndParent;
extern HINSTANCE g_hInstance;

#endif
