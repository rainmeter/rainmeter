#ifndef __Ctitledb_h__
#define __Ctitledb_h__

enum PERFORMANCE_TITLE_TYPE { PERF_TITLE_COUNTER, PERF_TITLE_EXPLAIN };

class CPerfTitleDatabase
{
    private:

    unsigned    m_nLastIndex;
    PTSTR       * m_TitleStrings;
    PTSTR       m_pszRawStrings;
    
    public:
        
    CPerfTitleDatabase( PERFORMANCE_TITLE_TYPE titleType );
    ~CPerfTitleDatabase( );

    unsigned    GetLastTitleIndex(void) { return m_nLastIndex; }
    PTSTR       GetTitleStringFromIndex( unsigned index );
    unsigned    GetIndexFromTitleString( PCTSTR pszTitleString );
};

#endif
