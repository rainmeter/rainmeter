#ifndef __Perfobj_h__
#define __Perfobj_h__

#ifndef _WINDOWS_
#include <windows.h>
#endif
#ifndef _WINPERF_
#include <winperf.h>
#endif

class CPerfObjectInstance;

class CPerfObject
{
    public:
        
    CPerfObject(    PPERF_OBJECT_TYPE const pObjectList,
                    CPerfTitleDatabase * const pPerfTitleDatabase );

    ~CPerfObject( void ){ }
    
    // Functions that return CPerfObjectInstance pointers.  Caller is
    // responsible for deleting the CPerfObjectInstance * when done with it.

    CPerfObjectInstance * GetFirstObjectInstance( void );

    CPerfObjectInstance * GetNextObjectInstance( void );    

    unsigned GetObjectInstanceCount(void){return m_pObjectList->NumInstances;}

    BOOL GetObjectTypeName( PTSTR pszObjTypeName, DWORD nSize );

    protected:
        
    PPERF_OBJECT_TYPE m_pObjectList;

    unsigned m_currentObjectInstance;

    PPERF_INSTANCE_DEFINITION m_pCurrentObjectInstanceDefinition;
        
    CPerfTitleDatabase * m_pPerfCounterTitles;
};

typedef CPerfObject * PCPerfObject;
#endif