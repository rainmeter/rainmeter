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

#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <map>
#include <vector>
#include <time.h>
#include "../../Library/Export.h"       // Rainmeter's exported functions

#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

/* The exported functions */
extern "C"
{
	__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
	__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
	__declspec( dllexport ) double Update2(UINT id);
	__declspec( dllexport ) UINT GetPluginVersion();
	__declspec( dllexport ) LPCTSTR GetPluginAuthor();
	__declspec( dllexport ) void ExecuteBang(LPCTSTR args, UINT id);
}

// System resources that can be counted
enum MEASURETYPE
{
	NUMRECYCLE,
	SIZERECYCLE
};

static std::map<UINT, MEASURETYPE> g_Values;
static std::map<UINT, std::wstring> g_DriveList;

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
	MEASURETYPE dataType = NUMRECYCLE;

	/* Read our own settings from the ini-file */
	LPCTSTR type = ReadConfigString(section, L"RecycleType", L"COUNT");
	if (type)
	{
		if (_wcsicmp(L"COUNT", type) == 0)
		{
			dataType = NUMRECYCLE;
		}
		else if (_wcsicmp(L"SIZE", type) == 0)
		{
			dataType = SIZERECYCLE;
		}
		else
		{
			std::wstring error = L"RecycleManager.dll: RecycleType=";
			error += type;
			error += L" is not valid in [";
			error += section;
			error += L"]";
			LSLog(LOG_ERROR, NULL, error.c_str());
		}
	}

	g_Values[id] = dataType;

	LPCTSTR drives = ReadConfigString(section, L"Drives", L"");
	g_DriveList[id] = (drives && *drives) ? drives : L"ALL";

	return 0;
}

void Tokenize(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters = L"|")
{
	// Skip delimiters at beginning.
	std::wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::wstring::size_type pos     = str.find_first_of(delimiters, lastPos);

	while (std::wstring::npos != pos || std::wstring::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
double Update2(UINT id)
{
	double retVal = 0;
	MEASURETYPE dataType = g_Values[id];
	const std::wstring& driveSet = g_DriveList[id];

	SHQUERYRBINFO rbi = {0};
	rbi.cbSize = sizeof(SHQUERYRBINFO);

	if (_wcsicmp(driveSet.c_str(), L"ALL") == 0)
	{
		if (SHQueryRecycleBin(NULL, &rbi) == S_OK)
		{
			if (dataType == SIZERECYCLE)
			{
				retVal = (double)rbi.i64Size; // size in bytes
			}
			else if (dataType == NUMRECYCLE)
			{
				retVal = (double)rbi.i64NumItems; // number of items in bin
			}
		}
	}
	else
	{
		std::vector<std::wstring> tokens;
		Tokenize(driveSet, tokens);

		for (int i = 0, isize = (int)tokens.size(); i < isize; i++)
		{
			if (SHQueryRecycleBin(tokens[i].c_str(), &rbi) == S_OK)
			{
				if (dataType == SIZERECYCLE)
				{
					retVal += (double)rbi.i64Size; // size in bytes
				}
				else if (dataType == NUMRECYCLE)
				{
					retVal += (double)rbi.i64NumItems; // number of items in bin
				}
			}
		}
	}

	return retVal;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, MEASURETYPE>::iterator i1 = g_Values.find(id);
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

void ExecuteBang(LPCTSTR args, UINT id)
{
	const std::wstring& driveSet = g_DriveList[id];
	if (_wcsicmp(args, L"EmptyBin") == 0)
	{
		// Empty the Recycle Bin
		if (_wcsicmp(driveSet.c_str(), L"ALL") == 0)
		{
			SHEmptyRecycleBin( NULL, NULL, NULL );
		}
		else
		{
			std::vector<std::wstring> tokens;
			Tokenize(driveSet, tokens);

			for (int i = 0, isize = (int)tokens.size(); i < isize; i++)
			{
				SHEmptyRecycleBin( NULL, tokens[i].c_str(), NULL ); // empty bin
			}
		}
		return;
	}
	else
	{
		if (_wcsicmp(args, L"EmptyBinSilent") == 0)
		{
			// Empty the Recycle Bin (no prompt)
			if (_wcsicmp(driveSet.c_str(), L"ALL") == 0)
			{
				SHEmptyRecycleBin( NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND );
			}
			else
			{
				std::vector<std::wstring> tokens;
				Tokenize(driveSet, tokens);

				for (int i = 0, isize = (int)tokens.size(); i < isize; i++)
				{
					SHEmptyRecycleBin( NULL, tokens[i].c_str(), SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND ); // empty bin
				}
			}
		}
		else if (_wcsicmp(args, L"OpenBin") == 0)
		{
			// Open the Recycle Bin folder
			ShellExecute(NULL, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", NULL, SW_SHOW);
			return;
		}
	}
}

