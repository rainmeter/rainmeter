//===========================================================================
// File: CTITLEDB.CPP
// Author: Matt Pietrek
// From: Microsoft Systems Journal, "Under the Hood", March 1996
//===========================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdlib.h>
#include <winperf.h>
#include <tchar.h>
#pragma hdrstop
#include "Titledb.h"

CPerfTitleDatabase::CPerfTitleDatabase(
		PERFORMANCE_TITLE_TYPE titleType )
{
	m_nLastIndex = 0;
	m_TitleStrings = 0;
	m_pszRawStrings = 0;

	// Determine the appropriate strings to pass to RegOpenKeyEx
	PTSTR psz009RegValue, pszLastIndexRegValue;
	if ( PERF_TITLE_COUNTER == titleType )
	{
		psz009RegValue = TEXT("Counter 009");
		pszLastIndexRegValue = TEXT("Last Counter");
	}
	else if ( PERF_TITLE_EXPLAIN == titleType )
	{
		psz009RegValue = TEXT("Explain 009");
		pszLastIndexRegValue = TEXT("Last Help");
	}
	else
		return;

	// Find out the max number of entries
	HKEY hKeyPerflib = 0;
	DWORD cbLastIndex;

	// Open the registry key that has the values for the maximum number
	// of title strings
	if ( ERROR_SUCCESS != RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			TEXT("software\\microsoft\\windows nt\\currentversion\\perflib"),
			0, KEY_READ, &hKeyPerflib ) )
		return;

	// Read in the number of title strings
	if ( ERROR_SUCCESS != RegQueryValueEx(
				hKeyPerflib, pszLastIndexRegValue, 0, 0,
				(PBYTE)&m_nLastIndex, &cbLastIndex ) )
	{
		RegCloseKey( hKeyPerflib );
		return;
	}

	RegCloseKey( hKeyPerflib );

	//
	// Now go find and process the raw string data
	//

	// Determine how big the raw data in the REG_MULTI_SZ value is
	DWORD cbTitleStrings;
	if ( ERROR_SUCCESS != RegQueryValueEx( HKEY_PERFORMANCE_DATA, psz009RegValue, 0,0,0, &cbTitleStrings))
		return;

	// Allocate memory for the raw registry title string data
	m_pszRawStrings = new TCHAR[cbTitleStrings];

	// Read in the raw title strings
	if ( ERROR_SUCCESS != RegQueryValueEx( HKEY_PERFORMANCE_DATA,
							psz009RegValue, 0, 0, (PBYTE)m_pszRawStrings,
							&cbTitleStrings ) )
	{
		delete []m_pszRawStrings;
		return;
	}

	// allocate memory for an array of string pointers.
	m_TitleStrings = new PTSTR[ m_nLastIndex+1 ];
	if ( !m_TitleStrings )
	{
		delete []m_pszRawStrings;
		return;
	}

	// Initialize the m_TitleStrings to all NULLs, since there may
	// be some array slots that aren't used.
	memset( m_TitleStrings, 0, sizeof(PTSTR) * (m_nLastIndex+1) );

	// The raw data entries are an ASCII string index (e.g., "242"), followed
	// by the corresponding string.  Fill in the appropriate slot in the
	// m_TitleStrings array with the pointer to the string name.  The end
	// of the list is indicated by a double nullptr.

	PTSTR pszWorkStr = (PTSTR)m_pszRawStrings;
	unsigned cbCurrStr;

	// While the length of the current string isn't 0...
	while ( 0 != (cbCurrStr = lstrlen( pszWorkStr)) )
	{
		// Convert the first string to a binary representation
		unsigned index = _ttoi( pszWorkStr );   // _ttoi -> atoi()

		if ( index > m_nLastIndex )
			break;

		// Now point to the string following it.  This is the title string
		pszWorkStr += cbCurrStr + 1;

		// Fill in the appropriate slot in the title strings array.
		m_TitleStrings[index] = pszWorkStr;

		// Advance to the next index/title pair
		pszWorkStr += lstrlen(pszWorkStr) + 1;
	}
}

CPerfTitleDatabase::~CPerfTitleDatabase( )
{
	delete []m_TitleStrings;
	m_TitleStrings = 0;
	delete []m_pszRawStrings;
	m_pszRawStrings = 0;
	m_nLastIndex = 0;
}

PTSTR
CPerfTitleDatabase::GetTitleStringFromIndex( unsigned index )
{
	if ( index > m_nLastIndex ) // Is index within range?
		return 0;

	return m_TitleStrings[ index ];
}

unsigned
CPerfTitleDatabase::GetIndexFromTitleString( PCTSTR pszTitleString )
{
	if ( IsBadStringPtr(pszTitleString, 0xFFFFFFFF) )
		return 0;

	// Loop through all the non-null string array entries, doing a case-
	// insensitive comparison.  If found, return the correpsonding index
	for ( unsigned i = 1; i <= m_nLastIndex; i++ )
	{
		if ( m_TitleStrings[i] )
			if ( 0 == _tcsicmp( pszTitleString, m_TitleStrings[i] ) )
				return i;
	}

	return 0;
}
