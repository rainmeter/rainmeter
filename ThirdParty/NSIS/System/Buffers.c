// Unicode support by Jim Park -- 08/23/2007

#include "stdafx.h"
#include "Plugin.h"
#include "System.h"
#include "Buffers.h"

typedef struct tagTempStack TempStack;
struct tagTempStack
{
    TempStack *Next;
    TCHAR Data[0];
};
TempStack *g_tempstack = NULL;

static void AllocWorker(unsigned int mult)
{
    size_t size;
    if ((size = popintptr()) == 0)
    {
        system_pushint(0);
        return;
    }
    system_pushintptr((INT_PTR) GlobalAlloc(GPTR, size * mult));
}

PLUGINFUNCTIONSHORT(Alloc)
{
    AllocWorker(sizeof(unsigned char));
}
PLUGINFUNCTIONEND

PLUGINFUNCTIONSHORT(StrAlloc)
{
    AllocWorker(sizeof(TCHAR));
}
PLUGINFUNCTIONEND

PLUGINFUNCTIONSHORT(Copy)
{
    SIZE_T size = 0;
    HANDLE source, dest;
    TCHAR *str;
    // Get the string
    if ((str = system_popstring()) == NULL) return;

    // Check for size option
    if (str[0] == _T('/'))
    {
        size = (SIZE_T) StrToIntPtr(str+1);
        dest = (HANDLE) popintptr();
    }
    else dest = (HANDLE) StrToIntPtr(str);
    source = (HANDLE) popintptr();

    // Ok, check the size
    if (size == 0) size = (SIZE_T) GlobalSize(source);
    // and the destinantion
    if (!dest) 
    {
        dest = GlobalAlloc((GPTR), size);
        system_pushintptr((INT_PTR) dest);
    }

    // COPY!
    copymem(dest, source, size);

    GlobalFree(str);
}
PLUGINFUNCTIONEND

#define EXECFLAGSSTACKMARKER ( sizeof(TCHAR) > 1 ? (TCHAR) 0x2691 : 0x1E ) // U+2691 Black Flag

PLUGINFUNCTION(Store)
{
    TempStack *tmp;
    stack_t*pNSE;
    int size = ((INST_R9+1)*g_stringsize*sizeof(TCHAR));
    int tmpint;

    TCHAR *command, *cmd = command = system_popstring();
    while (*cmd != 0)
    {
        switch (*(cmd++))
        {
        case _T('s'):
        case _T('S'):
            // Store the whole variables range
            tmp = (TempStack*) GlobalAlloc(GPTR, sizeof(TempStack)+size);
            // Fill with data
            copymem(tmp->Data, g_variables, size);
            // Push to private stack
            tmp->Next = g_tempstack, g_tempstack = tmp;
            break;
        case _T('l'):
        case _T('L'):
            if (g_tempstack == NULL) break;

            // Fill with data
            copymem(g_variables, g_tempstack->Data, size);
            // Pop from private stack
            tmp = g_tempstack, g_tempstack = g_tempstack->Next;
            GlobalFree((HANDLE) tmp);
            break;
        case _T('P'):
            *cmd += 10;
        case _T('p'):
            GlobalFree((HANDLE) system_pushstring(system_getuservariable(*(cmd++)-_T('0'))));
            break;
        case _T('R'):
            *cmd += 10;
        case _T('r'):
            GlobalFree((HANDLE) system_setuservariable(*(cmd++)-_T('0'), system_popstring()));
            break;
        case _T('f'):
            // Pop from stack
            pNSE = *g_stacktop, *g_stacktop = pNSE->next;
            // Restore data
            tmpint = extra->exec_flags->abort;
            if (pNSE->text[0] == EXECFLAGSSTACKMARKER)
              copymem(extra->exec_flags, pNSE->text+2, sizeof(exec_flags_t));
            extra->exec_flags->abort = tmpint; // Don't allow overriding the abort flag
            GlobalFree((HANDLE) pNSE);
            break;
        case _T('F'):
            // Store the data
             pNSE = (stack_t*) GlobalAlloc(GPTR, sizeof(stack_t)+(g_stringsize*sizeof(TCHAR)));
            *((UINT32*)pNSE->text) = EXECFLAGSSTACKMARKER; // marker + '\0'
            copymem(pNSE->text+2, extra->exec_flags, sizeof(exec_flags_t));
            // Push to stack
            pNSE->next = *g_stacktop, *g_stacktop = pNSE;
            break;
        }
    }

    GlobalFree((HANDLE) command);
}
PLUGINFUNCTIONEND
