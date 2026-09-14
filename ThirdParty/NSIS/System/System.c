// System.cpp : Defines the entry point for the DLL application.
//

// Unicode support by Jim Park & Olivier Marcoux

#include "stdafx.h"
#include "Plugin.h"
#include "Buffers.h"
#include "System.h"
#ifndef __GNUC__
#define _DECL_DLLMAIN   /* enable prototypes for DllMain and _CRT_INIT */
#include <process.h>
#include <crtdbg.h>
#else
#define _RPT0(type, msg)
#define _CRT_WARN           0
#endif /* __GNUC__ */
#include <objbase.h>



// Parse Section Type 
#define PST_PROC    0
#define PST_PARAMS  1
#define PST_RETURN  2
#define PST_OPTIONS 3

#define PCD_NONE    0
#define PCD_PROC    1
#define PCD_PARAMS  2
#define PCD_DONE    3   // Just Continue

const int PARAMSIZEBYTYPE_PTR = (4==sizeof(void*)) ? 1 : 2;
const int ParamSizeByType[8] = {
    0, // PAT_VOID (Size will be equal to 1) //BUGBUG64?
    1, // PAT_INT
    2, // PAT_LONG
    sizeof(void*) / 4, // PAT_STRING //BUGBUG64?
    sizeof(void*) / 4, // PAT_WSTRING //BUGBUG64?
    sizeof(void*) / 4, // PAT_GUID //BUGBUG64?
    0, // PAT_CALLBACK (Size will be equal to 1) //BUGBUG64?
    0 // PAT_REGMEM //BUGBUG64?
};

// The size of the base type when used in a struct with the '&' syntax (multiply by (Option - PAO_ARRBASE) to get the real size)
static const int ByteSizeByType[8] = {
    1, // PAT_VOID
    1, // PAT_INT
    1, // PAT_LONG
    1, // PAT_STRING
    2, // PAT_WSTRING (Special case for &wN notation: N is a number of WCHAR, not a number of bytes)
    1, // PAT_GUID (Must stay 1 for compatibility with the old '*(&g16,i)i.s' syntax)
    1, // PAT_CALLBACK
    1 // PAT_REGMEM
};

int LastStackPlace;
int LastStackReal;
DWORD LastError;
volatile SystemProc *LastProc;
int CallbackIndex;
CallbackThunk* g_CallbackThunkListHead;
HINSTANCE g_hInstance;

// Return to callback caller with stack restore
char retexpr[4]; //BUGBUG64?
HANDLE retaddr;

static TCHAR *MakeResultStr(SystemProc *proc, TCHAR *buf)
{
    if (proc->ProcResult == PR_CALLBACK)
    {
        INT_PTR id = proc->CallbackIndex;
#ifdef POPT_SYNTAX2
        if (proc->Options & POPT_SYNTAX2)
            id = (INT_PTR) GetAssociatedSysProcFromCallbackThunkPtr(proc->Proc);
#endif
        wsprintf(buf, sizeof(void*) > 4 ? _T("callback%Id") : _T("callback%d"), id); // "%d" must match format used by system_pushintptr() in Get() because script will StrCmp!
    }
    else
    {
        const TCHAR *resstr = _T("error");
        if (proc->ProcResult == PR_OK)
            resstr = _T("ok");
        lstrcpy(buf, resstr);
    }
    return buf;
}

TCHAR *AllocResultStr(SystemProc *proc)
{
    return MakeResultStr(proc, AllocString());
}

#ifdef SYSTEM_LOG_DEBUG

#ifndef COUNTOF
#define COUNTOF(a) ( sizeof(a) / sizeof(a[0]) )
#endif

// System log debugging turned on
#define SYSTEM_LOG_ADD(a)  do{ register int _len = lstrlen(syslogbuf); lstrcpyn(syslogbuf + _len, a, COUNTOF(syslogbuf) - _len); }while(0)
#define SYSTEM_LOG_POST     do{ SYSTEM_LOG_ADD(_T("\n")); WriteToLog(syslogbuf); *syslogbuf = 0; }while(0)

HANDLE logfile = NULL;
TCHAR syslogbuf[4096] = _T("");
int logop = 0;

void WriteToLog(TCHAR *buffer)
{
    DWORD written;
    TCHAR timebuffer[128];

    GetTickCount();

    if (logfile == NULL) return;

    SetFilePointer(logfile, 0, 0, FILE_END);

    if (-1 != logop)
    {
        wsprintf(timebuffer, _T("%04d  %04d.%03d    "), (++logop)%10000,
            (GetTickCount() / 1000) % 10000, GetTickCount() % 1000);

#ifdef _UNICODE
#ifdef _RPTW0
        _RPTW0(_CRT_WARN, timebuffer);
        _RPTW0(_CRT_WARN, buffer);
#endif
#else
        _RPT0(_CRT_WARN, timebuffer);
        _RPT0(_CRT_WARN, buffer);
#endif

        WriteFile(logfile, timebuffer, lstrlen(timebuffer)*sizeof(TCHAR), &written, NULL);
    }
    WriteFile(logfile, buffer, lstrlen(buffer)*sizeof(TCHAR), &written, NULL);
//    FlushFileBuffers(logfile);
}

PLUGINFUNCTION(Debug)
{
    TCHAR *o1;
    o1 = system_popstring();

    if (logfile == NULL)
        if (lstrlen(o1) > 0)
        {
            SYSTEMTIME t;
            TCHAR buffer[1024], buftime[1024], bufdate[1024];

            // Init debugging
            logfile = CreateFile(o1, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            SetFilePointer(logfile, 0, 0, FILE_END);

            logop = 0;
#ifdef _UNICODE
            {   // write Unicode Byte-Order Mark
                DWORD written;
                unsigned short bom = 0xfeff;
                WriteFile(logfile, &bom, 2, &written, NULL);
            }
#endif
            GetLocalTime(&t);
            GetTimeFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, &t, NULL, buftime, 1024);
            GetDateFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, &t, NULL, bufdate, 1024);
            wsprintf(buffer, _T("System, %s %s [build %hs %hs]\n"), buftime, bufdate, __TIME__, __DATE__);
            WriteToLog(buffer);
        } else ;
    else
    if (lstrlen(o1) > 0)
    {
        // Log in to log file
        int orglogop;
        WriteToLog(o1);
        orglogop = logop, logop = -1;
        WriteToLog(_T("\n"));
        logop = orglogop;
    } else
    {
        // Stop debugging
        WriteToLog(_T("Debug stopped.\n\n\n"));
        CloseHandle(logfile);
        logfile = NULL;
    }
    if (o1) GlobalFree(o1);
} PLUGINFUNCTIONEND

#else

// System log debugging turned off
#define SYSTEM_EVENT(a)
#define SYSTEM_LOG_ADD(a)
#define SYSTEM_LOG_POST

#endif

/**
 * This function is useful for Unicode support.  Since the Windows
 * GetProcAddress function always takes a char*, this function wraps
 * the windows call and does the appropriate translation when
 * appropriate.
 *
 * @param dllHandle Handle to the DLL loaded by LoadLibraryEx.
 * @param funcName The name of the function to get the address of.
 * @return The pointer to the function.  Null if failure.
 */
void * NSISGetProcAddress(HMODULE dllHandle, TCHAR* funcName)
{
#ifdef _UNICODE
  char* ansiName = NULL;
  int   len;
  void* funcPtr;

  len = WideCharToMultiByte(CP_ACP, 0, funcName, -1, ansiName, 0, NULL, NULL);
  ansiName = (char*) GlobalAlloc(GPTR, len);
  WideCharToMultiByte(CP_ACP, 0, funcName, -1, ansiName, len, NULL, NULL);
  funcPtr = GetProcAddress(dllHandle, ansiName);
  GlobalFree(ansiName);
  return funcPtr;
#else
  return GetProcAddress(dllHandle, funcName);
#endif
}

PLUGINFUNCTIONSHORT(Free)
{
    HANDLE memtofree = (HANDLE)popintptr();

    if (g_CallbackThunkListHead)
    {
        CallbackThunk *pCb=g_CallbackThunkListHead,*pPrev=NULL;
        do 
        {
            if (GetAssociatedSysProcFromCallbackThunkPtr(pCb) == (SystemProc*)memtofree) 
            {
                if (pPrev)
                    pPrev->pNext=pCb->pNext;
                else
                    g_CallbackThunkListHead=pCb->pNext;

                --(CallbackIndex);
                VirtualFree(pCb,0,MEM_RELEASE);
                break;
            }
            pPrev=pCb;
            pCb=pCb->pNext;
        }
        while( pCb != NULL );
    }

    GlobalFree(memtofree);
}
PLUGINFUNCTIONEND

PLUGINFUNCTION(Get)
{
    SystemProc *proc = PrepareProc(FALSE);
    if (proc == NULL)
    {
      system_pushstring(_T("error"));
      return;
    }

    SYSTEM_LOG_ADD(_T("Get "));
    SYSTEM_LOG_ADD(proc->DllName);
    SYSTEM_LOG_ADD(_T("::"));
    SYSTEM_LOG_ADD(proc->ProcName);
    //SYSTEM_LOG_ADD(_T("\n"));
    SYSTEM_LOG_POST;
    if (proc->Options & POPT_ALWRETURN)
    {
        // Always return flag set -> return separate proc and result
        system_pushintptr((INT_PTR) proc);
        GlobalFree(system_pushstring(AllocResultStr(proc)));
    }
    else
    {
        if (proc->ProcResult != PR_OK)
        {
            // No always return flag and error result - return result
            GlobalFree(system_pushstring(AllocResultStr(proc)));
            // If proc is permanent?
            if (!(proc->Options & POPT_PERMANENT))
                GlobalFree((HGLOBAL) proc); // No, free it
        }
        else // Ok result, return proc
            system_pushintptr((INT_PTR) proc);
    }
} PLUGINFUNCTIONEND


#ifdef SYSTEM_ARM64
/*
TODO: CallProc not implemented.
Fake the behavior of the System plugin for the LoadImage API function etc. so MUI works.
BUGBUG: MUI is leaking DeleteObject
*/
SystemProc* CallProc(SystemProc *proc)
{
    INT_PTR ret, *place;
    if (!lstrcmp(proc->ProcName, sizeof(TCHAR) > 1 ? _T("LoadImageW") : _T("LoadImageA")))
    {
        ret = (INT_PTR) LoadImage((HINSTANCE)proc->Params[1].Value, (LPCTSTR)proc->Params[2].Value, (UINT)proc->Params[3].Value, (int)proc->Params[4].Value, (int)proc->Params[5].Value, (UINT)proc->Params[6].Value);
        LastError = GetLastError();
    }
    else if (!lstrcmp(proc->ProcName, _T("GetClientRect")))
        ret = GetClientRect((HWND)proc->Params[1].Value, (RECT*)proc->Params[2].Value);
    else if (!lstrcmp(proc->ProcName, _T("GetWindowRect")))
        ret = GetWindowRect((HWND)proc->Params[1].Value, (RECT*)proc->Params[2].Value);
    else if (!lstrcmp(proc->ProcName, _T("MapWindowPoints")))
        ret = MapWindowPoints((HWND)proc->Params[1].Value, (HWND)proc->Params[2].Value, (POINT*)proc->Params[3].Value, (UINT)proc->Params[4].Value);
    else if (!lstrcmp(proc->ProcName, _T("SetWindowPos")))
        ret = SetWindowPos((HWND)proc->Params[1].Value, (HWND)proc->Params[2].Value, (int)proc->Params[3].Value, (int)proc->Params[4].Value, (int)proc->Params[5].Value, (int)proc->Params[6].Value, (UINT)proc->Params[7].Value);
    else if (!lstrcmp(proc->ProcName, _T("GetWindowLong")))
        ret = GetWindowLong((HWND)proc->Params[1].Value, (int)proc->Params[2].Value);
    else if (!lstrcmp(proc->ProcName, _T("SetWindowLong")))
        ret = SetWindowLong((HWND)proc->Params[1].Value, (int)proc->Params[2].Value, (LONG)proc->Params[3].Value);
    else if (!lstrcmp(proc->ProcName, _T("GetWindowText")))
        ret = GetWindowText((HWND)proc->Params[1].Value, (LPTSTR)proc->Params[2].Value, (int)proc->Params[3].Value);
    else if (!lstrcmp(proc->ProcName, _T("SendMessageA")))
        ret = SendMessageA((HWND)proc->Params[1].Value, (UINT)proc->Params[2].Value, (WPARAM)proc->Params[3].Value, (LPARAM)proc->Params[4].Value);
    else if (!lstrcmp(proc->ProcName, _T("SendMessage")) || !lstrcmp(proc->ProcName, _T("SendMessageW")))
        ret = SendMessageW((HWND)proc->Params[1].Value, (UINT)proc->Params[2].Value, (WPARAM)proc->Params[3].Value, (LPARAM)proc->Params[4].Value);
    else if (!lstrcmp(proc->ProcName, _T("GetVersionEx"))) // For winver
        ret = GetVersionEx((OSVERSIONINFO*)proc->Params[1].Value), LastError = GetLastError();
    else if (!lstrcmp(proc->ProcName, _T("GetNativeSystemInfo")))
        GetNativeSystemInfo((SYSTEM_INFO*)(ret = proc->Params[1].Value));
    else if (!lstrcmp(proc->ProcName, _T("CharNextW"))) // For x64
        ret = (INT_PTR) CharNextW((LPWSTR)proc->Params[1].Value);
    else if (!lstrcmp(proc->ProcName, _T("GetCurrentProcess"))) // For x64
        ret = (INT_PTR) GetCurrentProcess();
    else if (!lstrcmp(proc->ProcName, _T("IsWow64Process"))) // For x64
    {
        if (!(ret = (INT_PTR) GetProcAddress(LoadLibrary(_T("KERNEL32")), "IsWow64Process"))) goto fail;
        ret = ((BOOL(WINAPI*)(HANDLE,BOOL*))ret)((HANDLE)proc->Params[1].Value, (BOOL*)proc->Params[2].Value);
    }
    else if (!lstrcmp(proc->ProcName, _T("IsWow64Process2"))) // For x64
    {
        if (!(ret = (INT_PTR) GetProcAddress(LoadLibrary(_T("KERNEL32")), "IsWow64Process2"))) goto fail;
        ret = ((BOOL(WINAPI*)(HANDLE,USHORT*,USHORT*))ret)((HANDLE)proc->Params[1].Value, (USHORT*)proc->Params[2].Value, (USHORT*)proc->Params[3].Value);
    }
    else if (!lstrcmp(proc->ProcName, _T("Wow64EnableWow64FsRedirection"))) // For x64
    {
        if (!(ret = (INT_PTR) GetProcAddress(LoadLibrary(_T("KERNEL32")), "Wow64EnableWow64FsRedirection"))) goto fail;
        ret = ((BYTE(WINAPI*)(BYTE))ret)((BYTE)proc->Params[1].Value);
    }
    else fail:
        proc->ProcResult = PR_ERROR, ret = 0, LastError = ERROR_INVALID_FUNCTION;
    place = (INT_PTR*) proc->Params[0].Value;
    if (!ParamIsPointer(proc->Params[0])) place = (INT_PTR*) &(proc->Params[0].Value);
    if (place) *place = ret;
    return proc;
}
#endif //~ SYSTEM_ARM64
#ifdef _WIN64
/*
BUGBUG: TODO: CallBack support not implemented!
*/
SystemProc* CallBack(SystemProc *proc)
{
    proc->ProcResult = PR_ERROR;
    return proc;
}
#endif //~ _WIN64


PLUGINFUNCTION(Call)
{
    // Prepare input
    SystemProc *proc = PrepareProc(TRUE);
    if (proc == NULL) return;

    SYSTEM_LOG_ADD(_T("Call "));
    SYSTEM_LOG_ADD(proc->DllName);
    SYSTEM_LOG_ADD(_T("::"));
    SYSTEM_LOG_ADD(proc->ProcName);
    //SYSTEM_LOG_ADD(_T("\n"));
    SYSTEM_LOG_POST;
    if (proc->ProcResult != PR_CALLBACK) ParamAllocate(proc);
    ParamsIn(proc);

    // Make the call
    if (proc->ProcResult != PR_ERROR)
    {
        switch (proc->ProcType)
        {
        case PT_NOTHING:
            if (proc->ProcResult == PR_CALLBACK) 
                proc = CallBack(proc);
            break;
        case PT_PROC:
        case PT_VTABLEPROC:
            proc = CallProc(proc); break;
        case PT_STRUCT:
            CallStruct(proc); break;
        }
    }

    // Process output
    if (proc->Options & POPT_ALWRETURN)
    {
        // Always return flag set - return separate return and result
        ParamsOut(proc);
        GlobalFree(system_pushstring(AllocResultStr(proc)));
    }
    else
    {
        if (proc->ProcResult != PR_OK)
        {
            ProcParameter pp = proc->Params[0]; // Save old return param

            // Return result instead of return value
            TCHAR resstr[50];
            proc->Params[0].Value = (INT_PTR) MakeResultStr(proc, resstr);
            proc->Params[0].Type = PAT_TSTRING;
            proc->Params[0].allocatedBlock = NULL;

            ParamsOut(proc); // Return all params
            proc->Params[0] = pp; // Restore old return param
        }
        else
            ParamsOut(proc);
    }

    if (proc->ProcResult != PR_CALLBACK)
    {
        ParamsDeAllocate(proc);

        // if not callback - check for unload library option
        if ((proc->Options & POPT_UNLOAD) && proc->ProcType == PT_PROC && proc->Dll)
            FreeLibrary(proc->Dll); // and unload it :)

        // In case of POPT_ERROR - first pop will be proc error
        if ((proc->Options & POPT_ERROR) != 0) system_pushint(LastError);
    }    

    // If proc is permanent?
    if (!(proc->Options & POPT_PERMANENT)) GlobalFree((HGLOBAL) proc); // No, free it
} PLUGINFUNCTIONEND

PLUGINFUNCTIONSHORT(Int64Op)
{
    __int64 i1, i2 = 0, i3, i4;
    TCHAR *op;
#ifndef _WIN64
    TCHAR buf[25], *o1, *o2;
#endif

    // Get parameters: <num1> <op> [num2]
#ifdef _WIN64
    i1 = system_popintptr();
#else
    o1 = system_popstring(), i1 = myatoi64(o1);
#endif
    op = system_popstring();
    if ((*op != _T('~')) && (*op != _T('!')))
    {
#ifdef _WIN64
        i2 = system_popintptr();
#else
        o2 = system_popstring(), i2 = myatoi64(o2), GlobalFree(o2);
#endif
    }

    // Operation
    switch (*op)
    {
    case _T('+'): i1 += i2; break;
    case _T('-'): i1 -= i2; break;
    case _T('*'): i1 *= i2; break;
    case _T('/'): 
    case _T('%'): 
        // It's unclear, but in this case compiler will use DivMod routine
        // instead of two separate Div and Mod routines.
        if (i2 == 0) { i3 = 0; i4 = i1; }
        else {i3 = i1 / i2; i4 = i1 % i2; }
        if (*op == _T('/')) i1 = i3; else i1 = i4; 
        break;
    case _T('|'): if (op[1] == _T('|')) i1 = i1 || i2; else i1 |= i2; break;
    case _T('&'): if (op[1] == _T('&')) i1 = i1 && i2; else i1 &= i2; break;
    case _T('^'): i1 ^= i2; break;
    case _T('~'): i1 = ~i1; break;
    case _T('!'): i1 = !i1; break;
    case _T('<'): if (op[1] == _T('<')) i1 = i1 << i2; else i1 = i1 < i2; break;
    case _T('>'): if (op[1] == _T('>')) i1 = op[2] == _T('>') ? (UINT64)i1 >> (UINT64)i2 : i1 >> i2; else i1 = i1 > i2; break;
    case _T('='): i1 = (i1 == i2); break;
    }
    
    // Output and freedom
#ifdef _WIN64
    system_pushintptr(i1);
#else
    myitoa64(i1, buf), system_pushstring(buf), GlobalFree(o1);
#endif
    GlobalFree(op);
} PLUGINFUNCTIONEND

__int64 GetIntFromString(TCHAR **p)
{
    TCHAR buffer[128], *b = buffer;
    (*p)++; // First character should be skipped
    while (((**p >= _T('a')) && (**p <= _T('f'))) || ((**p >= _T('A')) && (**p <= _T('F'))) || ((**p >= _T('0')) && (**p <= _T('9'))) || (**p == _T('X')) || (**p == _T('-')) || (**p == _T('x')) || (**p == _T('|'))) *(b++) = *((*p)++);
    *b = 0;
    (*p)--; // We should point at last digit
    return myatoi64(buffer);
}

SystemProc *PrepareProc(BOOL NeedForCall)
{
    int SectionType = PST_PROC, // First section is always proc spec
        ProcType = PT_NOTHING, // Default proc spec
        ChangesDone = 0,
        ParamIndex = 0,
        temp = 0, temp2, temp3;
    INT_PTR temp4;
    BOOL param_defined = FALSE;
    SystemProc *proc = NULL;
    TCHAR *ibuf, *ib, *sbuf, *cbuf, *cb;
    unsigned int UsedTString = 0, aligntype, inpathq = 0;
#ifdef POPT_SYNTAX2
    const UINT alignflag = PAT_ALIGNFLAG;
#else
    const UINT alignflag = 0;
#endif

#ifdef __GNUC__
    temp3 = 0; // "warning: 'temp3' may be used uninitialized in this function": temp3 is set to 0 when we start parsing a new parameter
#endif

    // Retrieve proc specs
    cb = (cbuf = AllocString()); // Current String buffer
    sbuf = AllocString(); // Safe String buffer
    ib = ibuf = system_popstring(); // Input string

    // Parse the string
    while (SectionType != -1)
    {
        // Check for Section Change
        BOOL changed = TRUE;
        ChangesDone = SectionType;

        if (SectionType != PST_PROC && proc == NULL)
            // no proc after PST_PROC is done means something is wrong with
            // the call syntax and we'll get a crash because everything needs
            // proc from here on.
            break;

        switch (*ib)
        {
        case 0x0:
          SectionType = -1;
          break;
        case _T('#'): // "...#" redefines proc unless preceded by ":", then it's an ordinal (dll::#123)
          if ((ib <= ibuf || *(ib-1) != _T(':') || PST_PROC != SectionType) && !inpathq)
            SectionType = PST_PROC, ProcType = PT_NOTHING;
          else
            changed = FALSE;
          break;
        case _T('('):
            if (inpathq)
              changed = FALSE;
            else
            {
              SectionType = PST_PARAMS; 
              // fake-real parameter: for COM interfaces first param is Interface Pointer
              ParamIndex = ((ProcType == PT_VTABLEPROC)?(2):(1));
              temp3 = temp = 0;
              param_defined = FALSE;
            }
            break;
        case _T(')'):
          if (inpathq)
            changed = FALSE;
          else
          {
            SectionType = PST_RETURN;
            temp3 = temp = 0;
          }
          break;
        case _T('?'):
          SectionType = PST_OPTIONS; temp = 1;
          break;
        default:
            changed = FALSE;
        }

        // Check for changes
        if (changed)
        {
            switch (ChangesDone)
            {
            case PST_PROC:
                *cb = 0;
                // Adopt proc
                if (proc == NULL)
                {
                    proc = (SystemProc *) GlobalAlloc(GPTR, sizeof(SystemProc));
                    proc->Options = 0;
                    proc->ParamCount = 0;
                }
                // Default parameters
                *proc->DllName = 0;
                *proc->ProcName = 0;
                proc->Dll = NULL;
                proc->Proc = NULL;
                proc->ProcType = ProcType;
                proc->ProcResult = PR_OK;
    
                // Section changed and previous section was Proc
                switch (ProcType)
                {
                case PT_NOTHING:
                    // Is it previous proc or just unknown proc?
                    if (cb != cbuf)
                    {
                        // Previous proc (for clear up)
                        SystemProc *pr = NULL;

                        if (proc != NULL) GlobalFree(proc);
                        // Get already defined proc                                      
                        proc = (SystemProc *) StrToIntPtr(cbuf);
                        if (!proc) break;

                        // Find the last clone at proc queue
                        while (proc && (proc->Clone != NULL)) proc = (pr = proc)->Clone;

                        // Clear parents record for child callback proc
                        if (pr != NULL) pr->Clone = NULL;

                        // Never Redefine?
                        if (proc->Options & POPT_NEVERREDEF)
                        {
                            proc = GlobalCopy(proc); // Create new proc as copy
                            proc->Options &= ~(POPT_NEVERREDEF|POPT_PERMANENT); // NeverRedef options is never inherited
                        }
                        else
                            proc->Options |= POPT_PERMANENT; // Proc is old -> permanent
                    }
                    break;
                case PT_PROC:
                case PT_VTABLEPROC:
                    lstrcpy(proc->DllName, sbuf);
                case PT_STRUCT:
                    lstrcpy(proc->ProcName, cbuf);
                    break;
                }
                break;
            case PST_PARAMS:
                if (param_defined)
                    proc->ParamCount = ParamIndex;
                else
                    // not simply zero because of vtable calls
                    proc->ParamCount = ParamIndex - 1;
            case PST_RETURN:
            case PST_OPTIONS:
                break;
            }
            ib++;
            cb = cbuf;
            continue;
        }

        // Parse the section
        ChangesDone = PCD_NONE;
        switch (SectionType)
        {
        // Proc sections parser
        case PST_PROC: parse_next_proc_section_char:
            switch (*ib)
            {
            case _T('\"'):
              ib++; // Skip '\"'
              if (inpathq)
              {
                inpathq = 0;
                goto parse_next_proc_section_char;
              }
              else
              {
                inpathq++;
              }
              break;
            case _T(':'):
            case _T('-'):
                // Is it '::'
                if ((*(ib) == _T('-')) && (*(ib+1) == _T('>')))
                {
                    ProcType = PT_VTABLEPROC;
                }
                else
                {
                    if ((*(ib+1) != _T(':')) || (*(ib) == _T('-'))) break;
                    ProcType = PT_PROC;
                }
                ib++; // Skip next ':'

                if (cb > cbuf)
                {
                    *cb = 0;
                    lstrcpy(sbuf, cbuf);
                }
                else 
                  *sbuf = 0; // No dll - system proc
                
                // Ok
                ChangesDone = PCD_DONE;
                break;
            case _T('*'):
                // Structure definition
                ProcType = PT_STRUCT;
                ChangesDone = PCD_DONE;
                break;
            }
            break;

        // Params and return sections parser
        case PST_RETURN:
            ParamIndex = 0; // Uses the same logic as PST_PARAMS section
        case PST_PARAMS:
            temp2 = -1; temp4 = 0; // Our type placeholder
            aligntype = FALSE;
            switch (*ib)
            {
            case _T(' '):
                break;
            case _T('_'): // No param cutting specifier
                if (proc->ParamCount > ParamIndex) ParamIndex = proc->ParamCount;
                temp3 = temp = 0; // Clear parameter options
                if (proc->ParamCount != ((ProcType == PT_VTABLEPROC) ? 1 : 0))
                {
                  // only define params if the last count wasn't zero
                  // this prevents erroneous param count for:
                  //   'user32::CloseClipboard()(_)'
                  // for vtable calls, param count should not be one
                  param_defined = TRUE;
                }
                break;
            case _T(','): // Next param
                temp3 = temp = 0; // Clear parameter options
                ParamIndex++;
                param_defined = TRUE;
                break;
            case _T('&'):
                temp = PAO_ARRBASE; break; // Special parameter option
            case _T('*'):
                temp = PAO_PTRFLAG; break; // Pointer parameter option

            // Types
            case _T('@'): temp2 = PAT_REGMEM; break;
            case _T('V'): // No extra alignment for void padding
            case _T('v'): temp2 = PAT_VOID; break;
            case 'B': aligntype += alignflag ? 1 : 0;
            case 'b': temp2 = PAT_INT, temp = sizeof(BYTE) + 1; break; // INT8/BYTE/BOOLEAN alias for &1
            case 'H': aligntype += alignflag ? 1 : 0;
            case 'h': temp2 = PAT_INT, temp = sizeof(WORD) + 1; break; // INT16/WORD/SHORT alias for &2 with 'h' AKA printf type length specifier
#ifndef _WIN64
            case _T('P'):
#endif
            case _T('I'): aligntype += alignflag ? 1 : 0;
#ifndef _WIN64
            case _T('p'):
#endif
            case _T('i'): temp2 = PAT_INT; break; // INT32
#ifdef _WIN64
            case _T('P'):
#endif
            case _T('L'): aligntype += alignflag ? 1 : 0;
#ifdef _WIN64
            case _T('p'):
#endif
            case _T('l'): temp2 = PAT_LONG; break; // INT64
            case _T('M'): // 1 byte, no extra alignment
            case _T('m'): temp2 = PAT_STRING; break;
            case _T('T'): aligntype += (alignflag && sizeof(TCHAR) > 1) ? 1 : 0;
            case _T('t'): temp2 = PAT_TSTRING, ++UsedTString; break;
            case _T('G'): aligntype += alignflag ? 1 : 0;
            case _T('g'): temp2 = PAT_GUID; break;
            case _T('W'): aligntype += alignflag ? 1 : 0;
            case _T('w'): temp2 = PAT_WSTRING; break;
            case _T('K'): aligntype += alignflag ? 1 : 0;
            case _T('k'): temp2 = PAT_CALLBACK; break;

            // Input output specifiers
            case _T('.'): temp3++; break; // skip specifier

            case _T('R'):
                temp4 = ((INT_PTR) GetIntFromString(&ib))+1;
                if (temp4 < 11) temp4 += 10; 
                break;
            case _T('r'): temp4 = ((INT_PTR) GetIntFromString(&ib))+1; break; // Register

            case _T('-'):
            case _T('0'): case _T('1'): case _T('2'): case _T('3'): case _T('4'):
            case _T('5'): case _T('6'): case _T('7'): case _T('8'): case _T('9'):
                // Numeric inline
                if (temp3 == 0)
                {
                    ib--;
                    // It's stupid, I know, but I'm too lazy to do another thing
                    myitoa64(GetIntFromString(&(ib)),(TCHAR *)(temp4 = (INT_PTR) AllocString()));
                }
                break;

            case _T('\"'): case _T('\''): case _T('`'):
                // Character inline
                {
                    TCHAR start = *ib;
                    cb = cbuf;
                    // copy inline
                    while (!((*(++ib) == start) && (*(ib+1) != start)) && (*ib))
                    {
                        if ((*ib) == start) ++ib;
                        *(cb++) = *(ib);
                    }
                    // finish and save
                    *cb = 0; 
                    temp4 = (INT_PTR) AllocStr(cbuf);
                }
                break;

            case _T('s'):
            case _T('S'): temp4 = -1; break; // Stack
            case _T('c'):
            case _T('C'): temp4 = INST_CMDLINE+1; break;
            case _T('d'):
            case _T('D'): temp4 = INST_INSTDIR+1; break;
            case _T('o'):
            case _T('O'): temp4 = INST_OUTDIR+1; break;
            case _T('e'):
            case _T('E'): temp4 = INST_EXEDIR+1; break;
            case _T('a'):
            case _T('A'): temp4 = INST_LANG+1; break;
            }

            // Param type changed?
            if (temp2 != -1)
            {
                const int psbt = ParamSizeByType[temp2];
                param_defined = TRUE;
                proc->Params[ParamIndex].Type = temp2 | (alignflag && aligntype ? alignflag : 0);
                proc->Params[ParamIndex].Size = // Pointer sized or from type
                    ParamOptionIsPointer(temp) ? (PARAMSIZEBYTYPE_PTR) : ((psbt>0) ? (psbt) : (1)); //BUGBUG64: Is it safe to fallback to 1 for CALLBACK?
                if (temp == PAO_ARRBASE) temp =
                    PAO_ARRBASE + ((int) GetIntFromString(&ib)); // Read '&' array count specification
                proc->Params[ParamIndex].Option = temp;
                proc->Params[ParamIndex].Value = 0;
                proc->Params[ParamIndex].Input = IOT_NONE;
                proc->Params[ParamIndex].Output = IOT_NONE;
            }
          
            // Param source/dest changed?
            if (temp4 != 0)
            {
                param_defined = TRUE;
                if (temp3 == 0)
                {
                    // it may contain previous inline input
                    if (!((proc->Params[ParamIndex].Input > -1) && (proc->Params[ParamIndex].Input <= __INST_LAST)))
                        GlobalFree((HGLOBAL) proc->Params[ParamIndex].Input);
                    proc->Params[ParamIndex].Input = temp4;
                }
                if (temp3 == 1)
                    proc->Params[ParamIndex].Output = (int) temp4; // Note: As long as we never assign a pointer to temp4 when parsing a destination the cast to int is OK.
                // Next parameter is output or something else
                temp3++;
            }

            ChangesDone = PCD_DONE;
            break;

        // Options sections parser
        case PST_OPTIONS:
            temp2 = 0;
            switch (*ib)
            {
            case _T(' '): break;
            case _T('!'): temp = -temp; break;
            case _T('c'):
#ifndef _WIN64
                temp2 = POPT_CDECL; // Only x86 cares, just eat the option for everything else
#endif
                break;
            case _T('r'): temp2 = POPT_ALWRETURN; break;
            case _T('n'): temp2 = POPT_NEVERREDEF; break;
            case _T('s'): temp2 = POPT_GENSTACK; break;
            case _T('e'): temp2 = POPT_ERROR; break;
            case _T('u'): temp2 = POPT_UNLOAD; break;
#ifdef POPT_SYNTAX2
            case _T('2'): temp2 = POPT_SYNTAX2; break;
#endif
            }

            // New Options
            if (temp2 != 0)
            {
                if (temp == 1) proc->Options |= temp2; else proc->Options &= ~temp2;
                // Back to default (turn on nothing) state
                temp = 1; temp2 = 0;
            }

            ChangesDone = PCD_DONE;
            break;
        }

        // Nothing done, just copy char to buffer
        if (ChangesDone == PCD_NONE) *(cb++) = *(ib);
        // Something done, buffer = ""
        else cb = cbuf;

        // Increase input pointer
        ib++;
    }

    GlobalFree(ibuf);
    GlobalFree(cbuf);
    GlobalFree(sbuf);

    // Ok, the final step: check proc for existence
    if (proc != NULL && proc->Proc == NULL)
    {
        switch (proc->ProcType)
        {
        case PT_NOTHING: break;
        case PT_VTABLEPROC:
            {
                // Use direct system proc address
                INT_PTR addr;

                proc->Dll = (HMODULE) StrToIntPtr(proc->DllName);
  
                if (proc->Dll == 0)
                {
                    proc->ProcResult = PR_ERROR;
                    break;
                }

                addr = (INT_PTR) proc->Dll;

                // fake-real parameter: for COM interfaces first param is Interface Pointer
                proc->Params[1].Output = IOT_NONE;
                proc->Params[1].Input = (INT_PTR) AllocStr(proc->DllName);
                proc->Params[1].Size = PARAMSIZEBYTYPE_PTR;
                proc->Params[1].Type = PAT_PTR;
                proc->Params[1].Option = 0;

                // addr - pointer to interface vtable
                addr = *((INT_PTR *)addr);
                // now addr contains the pointer to first item at VTABLE
                // add the index of proc
                addr = addr + (INT_PTR)(myatoi64(proc->ProcName)*sizeof(void*));
                proc->Proc = *((HANDLE*)addr);
            }
            break;
        case PT_PROC:
            if (*proc->DllName == 0)
            {
                // Use direct system proc address
                if ((proc->Proc = (HANDLE) StrToIntPtr(proc->ProcName)) == 0)
                    proc->ProcResult = PR_ERROR;
            } else
            {
                // Get DLL address
                if ((proc->Dll = GetModuleHandle(proc->DllName)) == NULL)
                    if ((proc->Dll = LoadLibrary(proc->DllName)) == NULL)
                    {
                        proc->ProcResult = PR_ERROR;
                        break;
                    }

                // Get proc address
                proc->Proc = NSISGetProcAddress(proc->Dll, proc->ProcName);
                if (!proc->Proc && *proc->ProcName == _T('#'))
                {
                  int ordinal = myatoi(proc->ProcName+1);
                  if (ordinal && IS_INTRESOURCE(ordinal))
                    proc->Proc = GetProcAddress(proc->Dll, MAKEINTRESOURCEA(ordinal));
                }
                if (UsedTString || !proc->Proc)
                {
                    FARPROC tproc;
                    TCHAR*ProcName = proc->ProcName; // This buffer has room for us to party on
                    unsigned int cch = lstrlen(ProcName);
#ifdef _UNICODE
                    STRSET2CH(ProcName+cch, _T('W'), _T('\0'));
#else
                    STRSET2CH(ProcName+cch, _T('A'), _T('\0'));
#endif
                    tproc = NSISGetProcAddress(proc->Dll, ProcName);
                    if (tproc)
                        proc->Proc = tproc;
                    else
                        if (!proc->Proc) proc->ProcResult = PR_ERROR;
                }
            }
            break;
        case PT_STRUCT:
            if (*(proc->ProcName) != 0) proc->Proc = (HANDLE) StrToIntPtr(proc->ProcName);
            break;
        }
    }

    return proc;
}

void ParamAllocate(SystemProc *proc)
{
    UINT i, c;
    for (i = 0, c = proc->ParamCount; i <= c; i++)
        if (!proc->Params[i].Value && ParamIsPointer(proc->Params[i]))
            proc->Params[i].Value = (INT_PTR) GlobalAlloc(GPTR, ParamSizeByType[GetParamType(proc->Params[i])] * 4);
}

void ParamsIn(SystemProc *proc)
{
    int i;
    HGLOBAL* place;
    TCHAR *realbuf;
#ifndef _UNICODE
    LPWSTR wstr;
#endif
#if !defined(_WIN64) && defined(C_ASSERT)
    C_ASSERT(FIELD_OFFSET(ProcParameter, Value) + sizeof(int) == FIELD_OFFSET(ProcParameter, _value)); // Make sure PAT_LONG writes to the correct places
#endif
    i = (proc->ParamCount > 0)?(1):(0);
    while (TRUE)
    {
        ProcParameter *par = &proc->Params[i];
        UINT partype;
        // Step 1: retrieve value
        if ((par->Input == IOT_NONE) || (par->Input == IOT_INLINE)) 
            realbuf = AllocStr(_T(""));
        else if (par->Input == IOT_STACK) realbuf = system_popstring();
        else if ((par->Input > 0) && (par->Input <= __INST_LAST)) 
            realbuf = system_getuservariable((int)par->Input - 1);
        else 
        {
            // Inline input, will be freed as realbuf
            realbuf = (TCHAR*) par->Input;
            par->Input = IOT_INLINE;
        }

        // Retrieve pointer to place
        place = ParamIsPointer(*par) ? (HGLOBAL*) par->Value : (HGLOBAL*) &(par->Value);

        // by default no blocks are allocated
        par->allocatedBlock = NULL;

        // Step 2: place it
        switch (partype = GetParamType(*par))
        {
        case PAT_VOID:
            par->Value = 0;
            break;
        case PAT_INT:
            *(int*)place = myatoi(realbuf);
            break;
        case PAT_LONG:
            *(__int64*)place = myatoi64(realbuf);
            break;
        case PAT_TSTRING:
/*            if (par->Input == IOT_NONE) 
                *((int*) place) = (int) NULL;
            else*/
            *place = par->allocatedBlock = AllocStr(realbuf);
            break;
#ifdef _UNICODE
        case PAT_STRING:
            *place = par->allocatedBlock = GlobalAlloc(GPTR, g_stringsize);
            WideCharToMultiByte(CP_ACP, 0, realbuf, g_stringsize, *(LPSTR*)place, g_stringsize, NULL, NULL);
            break;
        case PAT_GUID:
            *place = par->allocatedBlock = GlobalAlloc(GPTR, 16);
            CLSIDFromString(realbuf, *(LPCLSID*)place);
            break;
#else
        case PAT_WSTRING:
        case PAT_GUID:
            wstr = (LPWSTR) GlobalAlloc(GPTR, g_stringsize*sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, realbuf, g_stringsize, wstr, g_stringsize);
            if (partype == PAT_GUID)
            {
                *place = par->allocatedBlock = GlobalAlloc(GPTR, 16);
                CLSIDFromString(wstr, *(LPCLSID*)place);
                GlobalFree((HGLOBAL) wstr);
            } else
                *place = par->allocatedBlock = (HGLOBAL)wstr;
            break;
#endif
        case PAT_CALLBACK:
            // Generate new or use old callback
            if (realbuf[0])
                par->Value = (INT_PTR) CreateCallback((SystemProc*) StrToIntPtr(realbuf));
            break;
        case PAT_REGMEM:
            {
              LPTSTR straddr = system_getuservariableptr(par->Input - 1);
              par->Value = (INT_PTR) straddr;
              par->Value += sizeof(void*) > 4 ? sizeof(_T("-9223372036854775807###")) : sizeof(_T("-2147483647")); // "###" for sizeof(void*) alignment
              IntPtrToStr(par->Value, straddr);
            }
            break;
        }
        GlobalFree(realbuf);

#ifdef SYSTEM_LOG_DEBUG
        {
            TCHAR buf[666];
            UINT32 hi32 = 0;
#ifndef _WIN64
            hi32 = par->_value;
#endif
            wsprintf(buf, _T("\t\t\tParam In %d:\tType=%#x Value=") SYSFMT_HEXPTR _T(" hi32=0x%08X"), i, 
                par->Type, par->Value, hi32);
            SYSTEM_LOG_ADD(buf);
            SYSTEM_LOG_POST;
        }
#endif

        if (i == 0) break;
        if (i == proc->ParamCount) i = 0; else i++;
    }
}

void ParamsDeAllocate(SystemProc *proc)
{
    int i;
    for (i = proc->ParamCount; i >= 0; i--)
        if (proc->Params[i].Value && ParamIsPointer(proc->Params[i]))
        {
            GlobalFree((HGLOBAL) (proc->Params[i].Value));
            proc->Params[i].Value = 0;
        }
}

#define GetSpecialParamTypeSize(par) ( (((par).Option)-PAO_ARRBASE) * ByteSizeByType[GetParamType(par)] ) // Option must be > PAO_ARRBASE!
static const int g_intmask[4] = { 0xFFFFFFFF, 0x000000FF, 0x0000FFFF, 0x00FFFFFF };
#define GetMaskedInt32Value(val, size) ( (val) & g_intmask[((UINT)(size) < 4) ? (size) : (0)] )
#define GetSpecialParamInt32Value(par, size) GetMaskedInt32Value((par).Value, (size))

void ParamsOut(SystemProc *proc)
{
    INT_PTR *place;
    LPWSTR wstr;
    int i, partype, intval, typsiz;
    TCHAR *realbuf = AllocString();

    i = proc->ParamCount;
    do
    {
        // Retrieve pointer to place
        if (ParamIsPointer(proc->Params[i]))
            place = (INT_PTR*) proc->Params[i].Value;
        else 
            place = (INT_PTR*) &(proc->Params[i].Value);

        // Step 1: retrieve value
        switch (partype = GetParamType(proc->Params[i]))
        {
        case PAT_VOID:
            *realbuf = _T('\0');
            break;
#ifndef _WIN64
        case PAT_REGMEM:
#endif
        case PAT_INT:
            intval = (int)(*((INT_PTR*) place));
            if (proc->Params[i].Option > 0) // Note: We don't handle '*' pointers, "*h" and "*b" are not supported, use "*i" even on smaller types
            {
                typsiz = GetSpecialParamTypeSize(proc->Params[i]);
                intval = GetMaskedInt32Value(intval, typsiz);
            }
            wsprintf(realbuf, _T("%d"), intval);
            break;
#ifdef _WIN64
        case PAT_REGMEM:
#endif
        case PAT_LONG:
            myitoa64(*((__int64*) place), realbuf);
            break;
        case PAT_STRING:
#ifdef _UNICODE
            MultiByteToWideChar(CP_ACP, 0, *((char**) place), g_stringsize, realbuf, g_stringsize-1);
            realbuf[g_stringsize-1] = _T('\0'); // make sure we have a null terminator
#else
            lstrcpyn(realbuf,*((TCHAR**) place), g_stringsize); // note: lstrcpyn always includes a null terminator (unlike strncpy)
#endif
            break;
        case PAT_GUID:
#ifdef _UNICODE
            StringFromGUID2(*((REFGUID*)place), realbuf, g_stringsize);
#else
            {
                WCHAR guidstrbuf[39];
                int guidcch = StringFromGUID2(*((REFGUID*)place), guidstrbuf, 39);
                WideCharToMultiByte(CP_ACP, 0, guidstrbuf, guidcch, realbuf, g_stringsize, NULL, NULL);
            }
#endif
            break;
        case PAT_WSTRING:
            wstr = *((LPWSTR*)place);
#ifdef _UNICODE
            lstrcpyn(realbuf, wstr, g_stringsize); // note: lstrcpyn always include a null terminator (unlike strncpy)
#else
            WideCharToMultiByte(CP_ACP, 0, wstr, g_stringsize, realbuf, g_stringsize-1, NULL, NULL);
            realbuf[g_stringsize-1] = _T('\0'); // make sure we have a null terminator
#endif
            break;
        case PAT_CALLBACK:
            wsprintf(realbuf, sizeof(void*) > 4 ? _T("%Id") : _T("%d"), proc->Params[i].Value);
            break;
        }

        // memory cleanup
        if ((proc->Params[i].allocatedBlock != NULL)
        && ((proc->ProcType != PT_STRUCT) || ParamIsArray(proc->Params[i]))
        )
            GlobalFree(proc->Params[i].allocatedBlock);

        SYSTEM_LOG_ADD(_T("\t\t\tParam Out("));
        // Step 2: place it
        if (proc->Params[i].Output == IOT_NONE)
            SYSTEM_LOG_ADD(_T("none"));
        else if (proc->Params[i].Output == IOT_STACK)
        {
            SYSTEM_LOG_ADD(_T("stack"));
            system_pushstring(realbuf);
        }
        else if (proc->Params[i].Output > 0)
        {
            SYSTEM_LOG_ADD(_T("var"));
            system_setuservariable(proc->Params[i].Output - 1, realbuf);
        }
        else
            SYSTEM_LOG_ADD(_T("?BUG?"));

#ifdef SYSTEM_LOG_DEBUG
        {
            TCHAR dbgbuf[99];
            wsprintf(dbgbuf, _T(")\t%d:\tType=%#x Optn=%d Size=%d Data="),
                i, proc->Params[i].Type, proc->Params[i].Option, proc->Params[i].Size);
            SYSTEM_LOG_ADD(dbgbuf);
            SYSTEM_LOG_ADD(realbuf);
            SYSTEM_LOG_POST;
        }
#endif

        i--;
    } 
    while (i >= 0);

    GlobalFree(realbuf);
}

HANDLE CreateCallback(SystemProc *cbproc)
{
#if defined(SYSTEM_AMD64) || defined(SYSTEM_ARM64)
    return BUGBUG64(HANDLE) NULL;
#else
    char *mem;

    if (cbproc->Proc == NULL)
    {
        // Set callback index
        cbproc->CallbackIndex = ++(CallbackIndex);
        cbproc->Options |= POPT_PERMANENT;

        mem = (char *) (cbproc->Proc = VirtualAlloc(NULL, sizeof(CallbackThunk), MEM_COMMIT, PAGE_EXECUTE_READWRITE));

        ((CallbackThunk*)mem)->pNext=g_CallbackThunkListHead;
        g_CallbackThunkListHead=(CallbackThunk*)mem;

#ifdef SYSTEM_X86
        *(mem++) = (char) 0xB8; // Mov eax, const
        *((int *)mem) = (int) cbproc;
        mem += sizeof(int);
        *(mem++) = (char) 0xe9; // Jmp relative
        *((int *)mem) = (int) RealCallBack;
        *((int *)mem) -= ((int) mem) + 4;
#else
#error "Asm thunk not implemented for this architecture!"
#endif

    }

    // Return proc address
    return cbproc->Proc;
#endif
}

#ifdef POPT_SYNTAX2
static UINT GetStructParamAlignment(const ProcParameter *par)
{
    int partype = GetParamType(*par), isarr = ParamIsArray(*par);
// &l is used to get the final struct size "*(p, &l.r0)"
// but it also allows you to write the struct size to the struct as a int<8|16|32|64> "*(p, &l4)"
// so we can't stop that with: "if ((par->Type & PAT_ALIGNFLAG) && (!isarr || partype != PAT_LONG))"
    if (par->Type & PAT_ALIGNFLAG)
    {
        if ((partype == PAT_STRING) | (partype == PAT_WSTRING)) return ByteSizeByType[partype];
        if (partype == PAT_GUID) return 16;
        return !isarr ? (par->Size * 4) : GetSpecialParamTypeSize(*par);
    }
    return 0;
}
#endif

void CallStruct(SystemProc *proc)
{
    BOOL ssflag; // "&l" struct size syntax
    UINT i, paramcount = proc->ParamCount, structsize = 0, size = 0, partype;
    char *st, *ptr;
#ifdef POPT_SYNTAX2
    BOOL tryalign = proc->Options & POPT_SYNTAX2;
#endif

    SYSTEM_LOG_ADD(_T("\t\tStruct..."));

    // Calculate the structure size 
    for (i = 1; i <= paramcount; i++)
    {
        partype = GetParamType(proc->Params[i]);
        // Emulate g as &g16
        // (Changing ByteSizeByType would break compatibility with '*(&g16,i)i.s')
        if (PAT_GUID==partype && ParamIsSimple(proc->Params[i]))
            proc->Params[i].Option = PAO_ARRBASE + 16;

        if (!ParamIsArray(proc->Params[i]))
            size = proc->Params[i].Size * 4;
        else
            size = GetParamArrayTypeSize(proc->Params[i]);

#ifdef POPT_SYNTAX2
        if (tryalign)
            structsize = SYS_ALIGNON(structsize, GetStructParamAlignment(&proc->Params[i]));
#endif
        structsize += size;
    }

    // Struct exists?
    if (proc->Proc == NULL) // No. Allocate struct memory
        proc->Proc = (HANDLE) GlobalAlloc(GPTR, structsize);
    else if (structsize == 0) // In case of zero size defined structure use mapped size 
        structsize = (int) GlobalSize((HGLOBAL) proc->Proc);

    #ifdef SYSTEM_LOG_DEBUG
    {
        TCHAR dbgbuf[99];
        wsprintf(dbgbuf, _T("\t(%u bytes)"), structsize);
        SYSTEM_LOG_ADD(dbgbuf);
    }
    #endif

    // Pointer to current data
    st = (char*) proc->Proc;

    for (i = 1; i <= paramcount; i++)
    {
        ssflag = FALSE;
        partype = GetParamType(proc->Params[i]);

        // Normal or special block?
        if (!ParamIsArray(proc->Params[i]))
        {
            // Normal
            size = proc->Params[i].Size * 4;
            ptr = (char*) &(proc->Params[i].Value);
        }
        else
        {
            // Special
            size = GetParamArrayTypeSize(proc->Params[i]);
            ptr = NULL;
            switch (partype)
            {
            case PAT_VOID:
                break;
            case PAT_LONG:
                // real structure size
                proc->Params[i].Value = structsize;
#ifndef _WIN64
                proc->Params[i]._value = 0;
#endif
                ssflag = TRUE; // System::Call '*(...,&l.r0)'
                // [[fallthrough]]
            case PAT_INT: 
                proc->Params[i].Value = GetSpecialParamInt32Value(proc->Params[i], size); // clears unused value bits
                // pointer
                ptr = (char*) &(proc->Params[i].Value); 
                break;

            case PAT_STRING:
            case PAT_WSTRING:
            case PAT_GUID:
                // Jim Park: Pointer for memcopy, so keep as char*
                ptr = (char*) proc->Params[i].Value;
                break;
            }
        }

        // Process them!
        if (ptr != NULL)
        {
#ifdef POPT_SYNTAX2
            if (tryalign)
                st = (char*) SYS_ALIGNON((INT_PTR) st, GetStructParamAlignment(&proc->Params[i]));
#endif

            // Input
            if ((proc->Params[i].Input != IOT_NONE) || (ssflag))
                copymem(st, ptr, size);

            // Output
            if (proc->Params[i].Output != IOT_NONE)
                copymem(ptr, st, size);
        }

        // Skip to next data block
        st += size;
    }

    SYSTEM_LOG_POST;

    // Proc virtual return - pointer to memory struct
    proc->Params[0].Value = (INT_PTR) proc->Proc;
}

/*
Use of system _DllMainCRTStartup to avoid endless recursion for the debug
report macro _RPT0. 

The system _DllMainCRTStartup initializes the C runtime environment. 
In particular the value for _osplatform is initialized. In the function 
_get_winmajor called in the execution of the _RPT0 macro an assertion 
failure is raised if _osplatform is not set. The assertion is reported by 
the same means as used for the _RPT0 macro. This leads to an endless recursion.
*/

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
{
    g_hInstance=hInst;

    if (DLL_PROCESS_ATTACH == fdwReason)
    {
        // change the protection of return command
        VirtualProtect(&retexpr, sizeof(retexpr), PAGE_EXECUTE_READWRITE, (PDWORD)&LastStackPlace);

        // initialize some variables
        LastStackPlace = 0, LastStackReal = 0;
        LastError = 0;
        LastProc = NULL;
        CallbackIndex = 0, g_CallbackThunkListHead = NULL;
#ifdef SYSTEM_X86
        retexpr[0] = (char) 0xC2;
        retexpr[2] = 0x00;
#elif defined(SYSTEM_AMD64) || defined(SYSTEM_ARM64)
        retexpr[0] = BUGBUG64(0);
#else
#error TODO
#endif
    }
    return TRUE;
}

/*
Returns size by which the stack should be expanded
*/
unsigned int GetNewStackSize(void)
{
    return NEW_STACK_SIZE;
}

/*
Returns non-zero value if GENSTACK option is set
*/
unsigned int GetGenStackOption(SystemProc *proc)
{
    return (proc->Options & POPT_GENSTACK);
}

/*
Returns non-zero value if CDECL option is set
*/
unsigned int GetCDeclOption(SystemProc *proc)
{
    return (proc->Options & POPT_CDECL);
}

/*
Returns non-zero value if Error option is set
*/
static unsigned int GetErrorOption(SystemProc *proc)
{
    return (proc->Options & POPT_ERROR);
}

/*
Returns offset for element Proc of SystemProc structure
*/
UINT_PTR GetProcOffset(void)
{
    return (UINT_PTR)(&(((SystemProc *)0)->Proc));
}

/*
Returns offset for element Clone of SystemProc structure
*/
unsigned int GetCloneOffset(void)
{
    return (unsigned int)(UINT_PTR) (&(((SystemProc *)0)->Clone));
}

/*
Returns offset for element ProcName of SystemProc structure
*/
unsigned int GetProcNameOffset(void)
{
    return (unsigned int)(UINT_PTR) (&(((SystemProc *)0)->ProcName));
}

/*
Returns offset for element ArgsSize of SystemProc structure
*/
unsigned int GetArgsSizeOffset(void)
{
    return (unsigned int)(UINT_PTR) (&(((SystemProc *)0)->ArgsSize));
}

/*
Returns number of parameters
*/
unsigned int GetParamCount(SystemProc *proc)
{
    return proc->ParamCount;
}

/*
Returns offset for element Params of SystemProc structure
*/
UINT_PTR GetParamsOffset(void)
{
    return (UINT_PTR)(&(((SystemProc *)0)->Params));
}

/*
Returns size of ProcParameter structure
*/
UINT_PTR GetSizeOfProcParam(void)
{
    return (sizeof(ProcParameter));
}

/*
Returns offset for element Size of ProcParameter structure
*/
unsigned int GetSizeOffsetParam(void)
{
    return (unsigned int)(UINT_PTR) (&(((ProcParameter *)0)->Size));
}

/*
Returns offset for element Value of ProcParameter structure
*/
UINT_PTR GetValueOffsetParam(void)
{
    return (UINT_PTR)(&(((ProcParameter *)0)->Value));
}

#ifndef _WIN64
/*
Returns offset for element _value of ProcParameter structure
*/
unsigned int Get_valueOffsetParam(void)
{
    return (unsigned int)(&(((ProcParameter *)0)->_value));
}
#endif

/*
Sets "CLONE" option
*/
void SetCloneOption(SystemProc *proc)
{
    proc->Options |= POPT_CLONE;
}

/*
Sets Result of procedure call to be "OK"
*/
static void SetProcResultOk(SystemProc *proc)
{
    proc->ProcResult = PR_OK;
}

/*
Sets Result of procedure call to be "CALLBACK"
*/
void SetProcResultCallback(SystemProc *proc)
{
    proc->ProcResult = PR_CALLBACK;
}

SystemProc* CALLBACK SetCallProcResultValues(SystemProc *proc, ULARGE_INTEGER retval)
{
    if (GetErrorOption(proc)) LastError = GetLastError();
#ifdef _WIN64
    proc->Params[0].Value = retval.QuadPart;
#else
    proc->Params[0].Value = retval.LowPart, proc->Params[0]._value = retval.HighPart;
#endif
    SetProcResultOk(proc);
    return proc;
}
