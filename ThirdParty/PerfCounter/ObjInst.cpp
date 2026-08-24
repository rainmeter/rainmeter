//====================================
// File: OBJINST.CPP
// Author: Matt Pietrek
// From: Microsoft Systems Journal
//       "Under the Hood", April 1996
//====================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winperf.h>
#include <stdlib.h>
#pragma hdrstop
#include "titledb.h"
#include "objinst.h"
#include "perfcntr.h"
#include "makeptr.h"

CPerfObjectInstance::CPerfObjectInstance(
		PPERF_INSTANCE_DEFINITION const pPerfInstDef,
		PPERF_COUNTER_DEFINITION const pPerfCntrDef,
		DWORD nCounters, CPerfTitleDatabase * const pPerfCounterTitles,
		BOOL fDummy)
{
	m_pPerfInstDef = pPerfInstDef;
	m_pPerfCntrDef = pPerfCntrDef;
	m_nCounters = nCounters;
	m_pPerfCounterTitles = pPerfCounterTitles;

	m_fDummy = fDummy;
}

BOOL
CPerfObjectInstance::GetObjectInstanceName(
	PTSTR pszObjInstName, DWORD nSize )
{
	if ( m_fDummy )
	{
		*pszObjInstName = 0;    // Return an empty string
		return FALSE;
	}

	if ( nSize < (m_pPerfInstDef->NameLength / sizeof(TCHAR)) )
		return FALSE;

	PWSTR pszName = MakePtr(PWSTR, m_pPerfInstDef, m_pPerfInstDef->NameOffset);

	#ifdef UNICODE
	lstrcpy( pszObjInstName, pszName );
	#else
	wcstombs( pszObjInstName, pszName, nSize );
	#endif

	return TRUE;
}

CPerfCounter *
CPerfObjectInstance::MakeCounter( PPERF_COUNTER_DEFINITION const pCounterDef )
{
	// Look up the name of this counter in the title database
	PTSTR pszName = m_pPerfCounterTitles->GetTitleStringFromIndex(
								pCounterDef->CounterNameTitleIndex );

	DWORD nInstanceDefSize = m_fDummy ? 0 : m_pPerfInstDef->ByteLength;

	// Create a new CPerfCounter.  The caller is responsible for deleting it.
	return new CPerfCounter(pszName,
							pCounterDef->CounterType,
							MakePtr( PBYTE, m_pPerfInstDef,
									nInstanceDefSize +
									pCounterDef->CounterOffset ),
							pCounterDef->CounterSize );
}

CPerfCounter *
CPerfObjectInstance::GetCounterByIndex( DWORD index )
{
	PPERF_COUNTER_DEFINITION pCurrentCounter;

	if ( index >= m_nCounters )
		return 0;

	pCurrentCounter = m_pPerfCntrDef;

	// Find the correct PERF_COUNTER_DEFINITION by looping
	for ( DWORD i = 0; i < index; i++ )
	{
		pCurrentCounter = MakePtr( PPERF_COUNTER_DEFINITION,
									pCurrentCounter,
									pCurrentCounter->ByteLength );
	}

	if ( pCurrentCounter->ByteLength == 0 )
		return 0;

	return MakeCounter( pCurrentCounter );
}

CPerfCounter *
CPerfObjectInstance::GetFirstCounter( void )
{
	m_currentCounter = 0;
	return GetCounterByIndex( m_currentCounter );
}

CPerfCounter *
CPerfObjectInstance::GetNextCounter( void )
{
	m_currentCounter++;
	return GetCounterByIndex( m_currentCounter );
}

CPerfCounter *
CPerfObjectInstance::GetCounterByName( PCTSTR const pszName )
{
	DWORD cntrIdx = m_pPerfCounterTitles->GetIndexFromTitleString(pszName);
	if ( cntrIdx == 0 )
		return 0;

	PPERF_COUNTER_DEFINITION pCurrentCounter = m_pPerfCntrDef;

	// Find the correct PERF_COUNTER_DEFINITION by looping and comparing
	for ( DWORD i = 0; i < m_nCounters; i++ )
	{
		if ( pCurrentCounter->CounterNameTitleIndex == cntrIdx )
			return MakeCounter( pCurrentCounter );

		// Nope.  Not this one.  Advance to the next counter
		pCurrentCounter = MakePtr( PPERF_COUNTER_DEFINITION,
									pCurrentCounter,
									pCurrentCounter->ByteLength );
	}

	return 0;
}