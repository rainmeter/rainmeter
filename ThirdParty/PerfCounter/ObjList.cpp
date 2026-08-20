//====================================
// File: OBJLIST.CPP
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
#include "objlist.h"
#include "perfobj.h"
#include "makeptr.h"

CPerfObjectList::CPerfObjectList(
		CPerfSnapshot * const pPerfSnapshot,
		CPerfTitleDatabase * const pPerfTitleDatabase )
{
	m_pPerfSnapshot = pPerfSnapshot;
	m_pPerfCounterTitles = pPerfTitleDatabase;
}

CPerfObject *
CPerfObjectList::GetFirstPerfObject( void )
{
	m_currentObjectListIndex = 0;
	if ( m_currentObjectListIndex >= m_pPerfSnapshot->GetNumObjectTypes() )
		return 0;

	m_pCurrObjectType =
		(PPERF_OBJECT_TYPE)m_pPerfSnapshot->GetPostHeaderPointer();

	return new CPerfObject( m_pCurrObjectType, m_pPerfCounterTitles );
}

CPerfObject *
CPerfObjectList::GetNextPerfObject( void )
{
	// Are we at the last object in the list?  Return nullptr if so.
	if ( ++m_currentObjectListIndex >= m_pPerfSnapshot->GetNumObjectTypes() )
		return 0;

	// Advance to the next PERF_OBJECT_TYPE structure
	m_pCurrObjectType = MakePtr(PPERF_OBJECT_TYPE,
								m_pCurrObjectType,
								m_pCurrObjectType->TotalByteLength );

	return new CPerfObject( m_pCurrObjectType, m_pPerfCounterTitles );
}

CPerfObject *
CPerfObjectList::GetPerfObject( PCTSTR const pszObjListName )
{
	DWORD objListIdx
		= m_pPerfCounterTitles->GetIndexFromTitleString( pszObjListName );
	if ( 0 == objListIdx )
		return 0;

	// Point at first PERF_OBJECT_TYPE, and loop through the list, looking
	// for one that matches.
	PPERF_OBJECT_TYPE pCurrObjectType =
			(PPERF_OBJECT_TYPE)m_pPerfSnapshot->GetPostHeaderPointer();

	for ( unsigned i=0; i < m_pPerfSnapshot->GetNumObjectTypes(); i++ )
	{
		// Is this the one that matches?
		if ( pCurrObjectType->ObjectNameTitleIndex == objListIdx )
			return new CPerfObject(pCurrObjectType, m_pPerfCounterTitles);

		// Nope... try the next object type
		pCurrObjectType = MakePtr(  PPERF_OBJECT_TYPE,
									pCurrObjectType,
									pCurrObjectType->TotalByteLength );
	}

	return 0;
}