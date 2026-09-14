#include <windows.h>
#include "pluginapi.h"

#ifndef COUNTOF
#define COUNTOF(a) (sizeof(a)/sizeof(a[0]))
#endif

// minimal tchar.h emulation
#ifndef _T
#  define _T TEXT
#endif
#if !defined(TCHAR) && !defined(_TCHAR_DEFINED)
#  ifdef UNICODE
#    define TCHAR WCHAR
#  else
#    define TCHAR char
#  endif
#endif

#define isvalidnsisvarindex(varnum) ( ((unsigned int)(varnum)) < (__INST_LAST) )

unsigned int g_stringsize;
stack_t **g_stacktop;
LPTSTR g_variables;

// utility functions (not required but often useful)

int NSISCALL popstring(LPTSTR str)
{
  stack_t *th;
  if (!g_stacktop || !*g_stacktop) return 1;
  th=(*g_stacktop);
  if (str) lstrcpy(str,th->text);
  *g_stacktop = th->next;
  GlobalFree((HGLOBAL)th);
  return 0;
}

int NSISCALL popstringn(LPTSTR str, int maxlen)
{
  stack_t *th;
  if (!g_stacktop || !*g_stacktop) return 1;
  th=(*g_stacktop);
  if (str) lstrcpyn(str,th->text,maxlen?maxlen:g_stringsize);
  *g_stacktop = th->next;
  GlobalFree((HGLOBAL)th);
  return 0;
}

void NSISCALL pushstring(LPCTSTR str)
{
  stack_t *th;
  if (!g_stacktop) return;
  th=(stack_t*)GlobalAlloc(GPTR,(sizeof(stack_t)+(g_stringsize)*sizeof(*str)));
  lstrcpyn(th->text,str,g_stringsize);
  th->next=*g_stacktop;
  *g_stacktop=th;
}

LPTSTR NSISCALL getuservariable(const int varnum)
{
  if (!isvalidnsisvarindex(varnum)) return NULL;
  return g_variables+varnum*g_stringsize;
}

void NSISCALL setuservariable(const int varnum, LPCTSTR var)
{
  if (var && isvalidnsisvarindex(varnum)) 
    lstrcpy(g_variables + varnum*g_stringsize, var);
}

#ifdef UNICODE
int NSISCALL PopStringA(LPSTR ansiStr)
{
   LPWSTR wideStr = (LPWSTR) GlobalAlloc(GPTR, g_stringsize*sizeof(WCHAR));
   int rval = popstring(wideStr);
   WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
   GlobalFree((HGLOBAL)wideStr);
   return rval;
}

int NSISCALL PopStringNA(LPSTR ansiStr, int maxlen)
{
   int realLen = maxlen ? maxlen : g_stringsize;
   LPWSTR wideStr = (LPWSTR) GlobalAlloc(GPTR, realLen*sizeof(WCHAR));
   int rval = popstringn(wideStr, realLen);
   WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, realLen, NULL, NULL);
   GlobalFree((HGLOBAL)wideStr);
   return rval;
}

void NSISCALL PushStringA(LPCSTR ansiStr)
{
   LPWSTR wideStr = (LPWSTR) GlobalAlloc(GPTR, g_stringsize*sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
   pushstring(wideStr);
   GlobalFree((HGLOBAL)wideStr);
   return;
}

void NSISCALL GetUserVariableW(const int varnum, LPWSTR wideStr)
{
   lstrcpyW(wideStr, getuservariable(varnum));
}

void NSISCALL GetUserVariableA(const int varnum, LPSTR ansiStr)
{
   LPWSTR wideStr = getuservariable(varnum);
   WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
}

void NSISCALL SetUserVariableA(const int varnum, LPCSTR ansiStr)
{
   if (ansiStr && isvalidnsisvarindex(varnum))
   {
      LPWSTR wideStr = g_variables + varnum * g_stringsize;
      MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
   }
}

#else
// ANSI defs
int NSISCALL PopStringW(LPWSTR wideStr)
{
   LPSTR ansiStr = (LPSTR) GlobalAlloc(GPTR, g_stringsize);
   int rval = popstring(ansiStr);
   MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
   GlobalFree((HGLOBAL)ansiStr);
   return rval;
}

int NSISCALL PopStringNW(LPWSTR wideStr, int maxlen)
{
   int realLen = maxlen ? maxlen : g_stringsize;
   LPSTR ansiStr = (LPSTR) GlobalAlloc(GPTR, realLen);
   int rval = popstringn(ansiStr, realLen);
   MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, realLen);
   GlobalFree((HGLOBAL)ansiStr);
   return rval;
}

void NSISCALL PushStringW(LPWSTR wideStr)
{
   LPSTR ansiStr = (LPSTR) GlobalAlloc(GPTR, g_stringsize);
   WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
   pushstring(ansiStr);
   GlobalFree((HGLOBAL)ansiStr);
}

void NSISCALL GetUserVariableW(const int varnum, LPWSTR wideStr)
{
   LPSTR ansiStr = getuservariable(varnum);
   MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wideStr, g_stringsize);
}

void NSISCALL GetUserVariableA(const int varnum, LPSTR ansiStr)
{
   lstrcpyA(ansiStr, getuservariable(varnum));
}

void NSISCALL SetUserVariableW(const int varnum, LPCWSTR wideStr)
{
   if (wideStr && isvalidnsisvarindex(varnum))
   {
      LPSTR ansiStr = g_variables + varnum * g_stringsize;
      WideCharToMultiByte(CP_ACP, 0, wideStr, -1, ansiStr, g_stringsize, NULL, NULL);
   }
}
#endif

// playing with integers

INT_PTR NSISCALL nsishelper_str_to_ptr(LPCTSTR s)
{
  INT_PTR v=0;
  if (*s == _T('0') && (s[1] == _T('x') || s[1] == _T('X')))
  {
    s++;
    for (;;)
    {
      int c=*(++s);
      if (c >= _T('0') && c <= _T('9')) c-=_T('0');
      else if (c >= _T('a') && c <= _T('f')) c-=_T('a')-10;
      else if (c >= _T('A') && c <= _T('F')) c-=_T('A')-10;
      else break;
      v<<=4;
      v+=c;
    }
  }
  else if (*s == _T('0') && s[1] <= _T('7') && s[1] >= _T('0'))
  {
    for (;;)
    {
      int c=*(++s);
      if (c >= _T('0') && c <= _T('7')) c-=_T('0');
      else break;
      v<<=3;
      v+=c;
    }
  }
  else
  {
    int sign=0;
    if (*s == _T('-')) sign++; else s--;
    for (;;)
    {
      int c=*(++s) - _T('0');
      if (c < 0 || c > 9) break;
      v*=10;
      v+=c;
    }
    if (sign) v = -v;
  }

  return v;
}

unsigned int NSISCALL myatou(LPCTSTR s)
{
  unsigned int v=0;

  for (;;)
  {
    unsigned int c=*s++;
    if (c >= _T('0') && c <= _T('9')) c-=_T('0');
    else break;
    v*=10;
    v+=c;
  }
  return v;
}

int NSISCALL myatoi_or(LPCTSTR s)
{
  int v=0;
  if (*s == _T('0') && (s[1] == _T('x') || s[1] == _T('X')))
  {
    s++;
    for (;;)
    {
      int c=*(++s);
      if (c >= _T('0') && c <= _T('9')) c-=_T('0');
      else if (c >= _T('a') && c <= _T('f')) c-=_T('a')-10;
      else if (c >= _T('A') && c <= _T('F')) c-=_T('A')-10;
      else break;
      v<<=4;
      v+=c;
    }
  }
  else if (*s == _T('0') && s[1] <= _T('7') && s[1] >= _T('0'))
  {
    for (;;)
    {
      int c=*(++s);
      if (c >= _T('0') && c <= _T('7')) c-=_T('0');
      else break;
      v<<=3;
      v+=c;
    }
  }
  else
  {
    int sign=0;
    if (*s == _T('-')) sign++; else s--;
    for (;;)
    {
      int c=*(++s) - _T('0');
      if (c < 0 || c > 9) break;
      v*=10;
      v+=c;
    }
    if (sign) v = -v;
  }

  // Support for simple ORed expressions
  if (*s == _T('|')) 
  {
      v |= myatoi_or(s+1);
  }

  return v;
}

INT_PTR NSISCALL popintptr()
{
  TCHAR buf[128];
  if (popstringn(buf,COUNTOF(buf)))
    return 0;
  return nsishelper_str_to_ptr(buf);
}

int NSISCALL popint_or()
{
  TCHAR buf[128];
  if (popstringn(buf,COUNTOF(buf)))
    return 0;
  return myatoi_or(buf);
}

void NSISCALL pushintptr(INT_PTR value)
{
  TCHAR buffer[30];
  wsprintf(buffer, sizeof(void*) > 4 ? _T("%Id") : _T("%d"), value);
  pushstring(buffer);
}
