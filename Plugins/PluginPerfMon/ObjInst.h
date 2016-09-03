#ifndef __Obinst_h__
#define __Objinst_h__

#ifndef _WINDOWS_
#include <windows.h>
#endif
#ifndef _WINPERF_
#include <winperf.h>
#endif

class CPerfTitleDatabase;
class CPerfCounter;

class CPerfObjectInstance
{
    public:
        
    CPerfObjectInstance(
            PPERF_INSTANCE_DEFINITION const pPerfInstDef,
            PPERF_COUNTER_DEFINITION const pPerfCntrDef, DWORD nCounters,
            CPerfTitleDatabase * const pPerfTitleDatabase, BOOL fDummy );

    ~CPerfObjectInstance( void ){ }
    
    BOOL GetObjectInstanceName( PTSTR pszObjInstName, DWORD nSize );
    
    // Functions that return CPerfCounter pointers.  Caller is
    // responsible for deleting the CPerfCounter * when done with it.

    CPerfCounter * GetFirstCounter( void );

    CPerfCounter * GetNextCounter( void );

    CPerfCounter * GetCounterByName( PCTSTR const pszName );

    protected:
        
    PPERF_INSTANCE_DEFINITION m_pPerfInstDef;

    unsigned m_nCounters;
    
    unsigned m_currentCounter;
    
    PPERF_COUNTER_DEFINITION m_pPerfCntrDef;

    CPerfTitleDatabase * m_pPerfCounterTitles;

    CPerfCounter * MakeCounter( PPERF_COUNTER_DEFINITION const pCounter );

    CPerfCounter * GetCounterByIndex( DWORD index );

    CPerfTitleDatabase *m_pCounterTitleDatabase;

    BOOL m_fDummy;  // FALSE normally, TRUE when an object with no instances
};

typedef CPerfObjectInstance * PCPerfObjectInstance;

#endif