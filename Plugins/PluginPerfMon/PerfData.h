#ifndef __Perfdata_h__
#define __Perfdata_h__

#include "titledb.h"
#include "perfsnap.h"
#include "objlist.h"
#include "perfobj.h"
#include "objinst.h"
#include "perfcntr.h"

#pragma warning ( disable: 4786 )

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

#endif