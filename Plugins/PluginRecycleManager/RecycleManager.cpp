/*
Copyright (C) 2005 Kimmo Pekkola, 2009 Greg Schoppe

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <stdlib.h> 
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <time.h>
#include "..\..\Library\Export.h"       // Rainmeter's exported functions

/* The exported functions */
extern "C"
{
	__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
	__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
	__declspec( dllexport ) double Update2(UINT id);
	__declspec( dllexport ) UINT GetPluginVersion();
	__declspec( dllexport ) LPCTSTR GetPluginAuthor();
	__declspec( dllexport ) void BinBang(LPCTSTR args, UINT id);
}

const int NUMRECYCLE  = 1;
const int SIZERECYCLE = 2;

static std::map<UINT, int> g_Values;

/*
This function is called when the measure is initialized.
The function must return the maximum value that can be measured.
The return value can also be 0, which means that Rainmeter will
track the maximum value automatically. The parameters for this
function are:

instance  The instance of this DLL
iniFile   The name of the ini-file (usually Rainmeter.ini)
section   The name of the section in the ini-file for this measure
id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	int dataType = 0;  // 1 for numRecycled, 2 for sizeRecycled

	/* Read our own settings from the ini-file */
	LPCTSTR type = ReadConfigString(section, L"RecycleType", L"COUNT");
	if (type)
	{
		if (wcsicmp(L"COUNT", type) == 0)
		{
			dataType = NUMRECYCLE;
		} 
		else if (wcsicmp(L"SIZE", type) == 0)
		{
			dataType = SIZERECYCLE;
		} 
		else
		{
			std::wstring error = L"No such RecycleType: ";
			error += type;
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
		}
	}

	g_Values[id] = dataType;

	return 0;
}

/*
This function is called when new value should be measured.
The function returns the new value.
*/
double Update2(UINT id)
{
	int dataType = g_Values[id];

	SHQUERYRBINFO RecycleBinInfo = { 0 };  
	RecycleBinInfo.cbSize = sizeof( RecycleBinInfo ); // Tell size of structure  
	SHQueryRecycleBin( L"C:", &RecycleBinInfo ); // Get recycle bin info  

	if (dataType == NUMRECYCLE)
	{
		return (double)RecycleBinInfo.i64Size; // size in bytes
	}
	else if (dataType == SIZERECYCLE)
	{
		return (double)RecycleBinInfo.i64NumItems; // number of items in bin
	}
	return 0;
}

/*
If the measure needs to free resources before quitting.
The plugin can export Finalize function, which is called
when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, int>::iterator i1 = g_Values.find(id);
	if (i1 != g_Values.end())
	{
		g_Values.erase(i1);
	}
}

UINT GetPluginVersion()
{
	return 1000;
}

LPCTSTR GetPluginAuthor()
{
	return L"gschoppe (gschoppe@gmail.com)";
}

void BinBang(LPCTSTR args, UINT id)
{
	std::wstring bang = args;

	if (wcsicmp(bang.c_str(), L"EmptyBin") == 0)
	{ //Empty the Recycle Bin
		SHEmptyRecycleBin( NULL, NULL, NULL );
	}
	else
	{
		if (wcsicmp(bang.c_str(), L"EmptyBinSilent") == 0)
		{ //Empty the Recycle Bin (no prompt)
			SHEmptyRecycleBin( NULL, NULL,
				SHERB_NOCONFIRMATION |
				SHERB_NOPROGRESSUI   |
				SHERB_NOSOUND );
		}
		else
		{ //Open the Recycle Bin folder
			system("explorer.exe /N,::{645FF040-5081-101B-9F08-00AA002F954E}");
		}
	}
}