#ifndef __Objlist_h__
#define __Objlist_h__

#ifndef _WINDOWS_
#include <windows.h>
#endif
#ifndef _WINPERF_
#include <winperf.h>
#endif
#ifndef __Perfsnap_h__
#include "perfsnap.h"
#endif

class CPerfObject;

class CPerfObjectList
{
    public:
        
    CPerfObjectList(CPerfSnapshot * const pPerfSnapshot,
                    CPerfTitleDatabase * const pPerfTitleDatabase );

    ~CPerfObjectList( void ){ };

    // Functions that return CPerfObject pointers.  Caller is responsible
    // for deleting the CPerfObject * when done with it.

    CPerfObject * GetFirstPerfObject( void );

    CPerfObject * GetNextPerfObject( void );
    
    CPerfObject * GetPerfObject( PCTSTR const pszObjListName );

    protected:

    CPerfSnapshot * m_pPerfSnapshot;

    CPerfTitleDatabase * m_pPerfCounterTitles;

    unsigned m_currentObjectListIndex;

    PPERF_OBJECT_TYPE m_pCurrObjectType;    // current first/next object ptr
};

typedef CPerfObjectList * PCPerfObjectList;
#endif