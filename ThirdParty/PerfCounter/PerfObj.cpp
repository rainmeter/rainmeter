//====================================
// File: PERFOBJ.CPP
// Author: Matt Pietrek
// From: Microsoft Systems Journal
//       "Under the Hood", APRIL 1996
//====================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winperf.h>
#include <stdlib.h>
#pragma hdrstop
#include "titledb.h"
#include "perfobj.h"
#include "objinst.h"
#include "makeptr.h"

CPerfObject::CPerfObject(   PPERF_OBJECT_TYPE const pObjectList,
							CPerfTitleDatabase * const pPerfCounterTitles)
{
	m_pObjectList = pObjectList;
	m_pPerfCounterTitles = pPerfCounterTitles;
}

CPerfObjectInstance *
CPerfObject::GetFirstObjectInstance( void )
{
	m_currentObjectInstance = 0;
	if ( m_currentObjectInstance >= GetObjectInstanceCount() )
		return 0;

	// Point at the first PERF_INSTANCE_DEFINITION
	m_pCurrentObjectInstanceDefinition =
		MakePtr( PPERF_INSTANCE_DEFINITION, m_pObjectList,
				m_pObjectList->DefinitionLength );

	return new CPerfObjectInstance(
				m_pCurrentObjectInstanceDefinition,
				MakePtr(PPERF_COUNTER_DEFINITION,
						m_pObjectList, m_pObjectList->HeaderLength),
				m_pObjectList->NumCounters,
				m_pPerfCounterTitles,
				m_pObjectList->NumInstances ==
					PERF_NO_INSTANCES ? TRUE : FALSE );
}

CPerfObjectInstance *
CPerfObject::GetNextObjectInstance( void )
{
	if ( m_pObjectList->NumInstances == PERF_NO_INSTANCES )
		return 0;

	if ( ++m_currentObjectInstance >= GetObjectInstanceCount() )
		return 0;

	// Advance to the next PERF_INSTANCE_DEFINITION in the list.  However,
	// following the current PERF_INSTANCE_DEFINITION is the counter data,
	// which is also of variable length.  So, we gotta take that into
	// account when finding the next PERF_INSTANCE_DEFINITION

	// First, get a pointer to the counter data size field
	PDWORD pCounterDataSize
		= MakePtr(PDWORD, m_pCurrentObjectInstanceDefinition,
					m_pCurrentObjectInstanceDefinition->ByteLength);

	// Now we can point at the next PPERF_INSTANCE_DEFINITION
	m_pCurrentObjectInstanceDefinition = MakePtr(PPERF_INSTANCE_DEFINITION,
				m_pCurrentObjectInstanceDefinition,
				m_pCurrentObjectInstanceDefinition->ByteLength
				+ *pCounterDataSize);

	// Create a CPerfObjectInstance based around the PPERF_INSTANCE_DEFINITION
	return new CPerfObjectInstance(m_pCurrentObjectInstanceDefinition,
								   MakePtr(PPERF_COUNTER_DEFINITION,
										   m_pObjectList,
										   m_pObjectList->HeaderLength),
								   m_pObjectList->NumCounters,
								   m_pPerfCounterTitles,
								   FALSE );
}

BOOL
CPerfObject::GetObjectTypeName( PTSTR pszObjTypeName, DWORD nSize )
{
	PTSTR pszName = m_pPerfCounterTitles->GetTitleStringFromIndex(
									m_pObjectList->ObjectNameTitleIndex );

	if ( !pszName )
		return FALSE;

	lstrcpyn( pszObjTypeName, pszName, nSize );
	return TRUE;
}