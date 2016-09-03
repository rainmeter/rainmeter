#ifndef __Perfcntr_h__
#define __Perfcntr_h__

class CPerfCounter
{
    public:

    CPerfCounter(   PTSTR const pszName, DWORD type,
                    PBYTE const pData, DWORD cbData );

    ~CPerfCounter( void );

    PTSTR GetName( void ) { return m_pszName; }

    DWORD GetType( void ) { return m_type; }

    DWORD GetSize( void ) { return m_cbData; }
    
    BOOL GetData( PBYTE pBuffer, DWORD cbBuffer, DWORD *pType );
    
    BOOL Format( PTSTR pszBuffer, DWORD nSize, BOOL fHex = FALSE );

    protected:
        
    PTSTR m_pszName;

    DWORD m_type;

    PBYTE m_pData;
    
    DWORD m_cbData;
};

typedef CPerfCounter * PCPerfCounter;
#endif