//====================================
// File: PERFCNTR.CPP
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
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>
#pragma hdrstop
#include "perfcntr.h"

CPerfCounter::CPerfCounter( PTSTR const pszName, DWORD type,
							PBYTE const pData, DWORD cbData )
{
	m_pszName = _tcsdup( pszName );
	m_type = type;
	m_cbData = cbData;
	m_pData = new BYTE[m_cbData];
	memcpy( m_pData, pData, m_cbData );
}

CPerfCounter::~CPerfCounter( void )
{
	free( m_pszName );
	delete []m_pData;
}

BOOL
CPerfCounter::GetData( PBYTE pBuffer, DWORD cbBuffer, DWORD *pType )
{
	if ( cbBuffer < m_cbData )  // Make sure the buffer is big enough
		return FALSE;

	memcpy( pBuffer, m_pData, m_cbData );   // copy the data

	if ( pType )            // If the user wants the type, give it to them
		*pType = m_type;

	return TRUE;
}

BOOL
CPerfCounter::Format( PTSTR pszBuffer, DWORD nSize, BOOL fHex )
{
	// Do better formatting!!!  Check length!!!

	PTSTR pszPrefix = TEXT("");
	TCHAR szTemp[512];

	// First, ascertain the basic type (number, counter, text, or zero)
	switch ( m_type & 0x00000C00 )
	{
		case PERF_TYPE_ZERO:
		{
			wsprintf( pszBuffer, TEXT("ZERO") ); return TRUE;
		}
		case PERF_TYPE_TEXT:
		{
			wsprintf( pszBuffer, TEXT("text counter") ); return TRUE;
		}
		case PERF_TYPE_COUNTER:
		{
			switch( m_type & 0x00070000 )
			{
				case PERF_COUNTER_RATE:
					pszPrefix = TEXT("counter rate "); break;
				case PERF_COUNTER_FRACTION:
					pszPrefix = TEXT("counter fraction "); break;
				case PERF_COUNTER_BASE:
					pszPrefix = TEXT("counter base "); break;
				case PERF_COUNTER_ELAPSED:
					pszPrefix = TEXT("counter elapsed "); break;
				case PERF_COUNTER_QUEUELEN:
					pszPrefix = TEXT("counter queuelen "); break;
				case PERF_COUNTER_HISTOGRAM:
					pszPrefix = TEXT("counter histogram "); break;
				default:
					pszPrefix = TEXT("counter value "); break;
			}
		}
	}

	PTSTR pszFmt = fHex ? TEXT("%s%Xh") : TEXT("%s%u");

	switch ( m_cbData )
	{
		case 1: wsprintf(szTemp, pszFmt, pszPrefix, *(PBYTE)m_pData);
				break;
		case 2: wsprintf(szTemp, pszFmt, pszPrefix, *(PWORD)m_pData);
				break;
		case 4: wsprintf(szTemp, pszFmt, pszPrefix, *(PDWORD)m_pData);
				break;
		case 8: // Danger!  Assumes little-endian (X86) byte ordering
				wsprintf( szTemp, TEXT("%s%X%X"), pszPrefix,
						*(PDWORD)(m_pData+4), *(PDWORD)m_pData ); break;
				break;

		default: wsprintf( szTemp, TEXT("<unhandled size %u>"), m_cbData );
	}

	switch ( m_type & 0x70000000 )
	{
		case PERF_DISPLAY_SECONDS:
			_tcscat( szTemp, TEXT(" secs") ); break;
		case PERF_DISPLAY_PERCENT:
			_tcscat( szTemp, TEXT(" %") ); break;
		case PERF_DISPLAY_PER_SEC:
			_tcscat( szTemp, TEXT(" /sec") ); break;
	}

	lstrcpyn( pszBuffer, szTemp, nSize );

	return TRUE;
}