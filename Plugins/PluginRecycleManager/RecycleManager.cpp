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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../Library/RawString.h"
#include "../../Library/Export.h"       // Rainmeter's exported functions
#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

// System resources that can be counted
enum MEASURETYPE
{
	NUMRECYCLE,
	SIZERECYCLE
};

struct MeasureData
{
	MEASURETYPE type;
	CRawString drives;

	MeasureData() : type(NUMRECYCLE) {}
};

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR value = RmReadString(rm, L"RecycleType", L"COUNT");
	if (_wcsicmp(L"COUNT", value) == 0)
	{
		measure->type = NUMRECYCLE;
	}
	else if (_wcsicmp(L"SIZE", value) == 0)
	{
		measure->type = SIZERECYCLE;
	}
	else
	{
		WCHAR buffer[256];
		_snwprintf_s(buffer, _TRUNCATE, L"RecycleManager.dll: RecycleType=%s is not valid in [%s]", value, RmGetMeasureName(rm));
		RmLog(LOG_ERROR, buffer);
	}

	value = RmReadString(rm, L"Drives", L"ALL");
	measure->drives = (_wcsicmp(value, L"ALL") == 0) ? NULL : value;
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	double retVal = 0;

	SHQUERYRBINFO rbi = {0};
	rbi.cbSize = sizeof(SHQUERYRBINFO);

	if (measure->drives.empty())
	{
		if (SHQueryRecycleBin(NULL, &rbi) == S_OK)
		{
			if (measure->type == SIZERECYCLE)
			{
				retVal = (double)rbi.i64Size; // size in bytes
			}
			else if (measure->type == NUMRECYCLE)
			{
				retVal = (double)rbi.i64NumItems; // number of items in bin
			}
		}
	}
	else
	{
		WCHAR* drives = _wcsdup(measure->drives.c_str());
		WCHAR* token = wcstok(drives, L"|");
		while (token)
		{
			if (SHQueryRecycleBin(token, &rbi) == S_OK)
			{
				if (measure->type == SIZERECYCLE)
				{
					retVal += (double)rbi.i64Size; // size in bytes
				}
				else if (measure->type == NUMRECYCLE)
				{
					retVal += (double)rbi.i64NumItems; // number of items in bin
				}
			}

			token = wcstok(NULL, L"|");
		}
		free(drives);
	}

	return retVal;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	MeasureData* measure = (MeasureData*)data;

	auto emptyBin = [&](DWORD flags)
	{
		if (measure->drives.empty())
		{
			SHEmptyRecycleBin(NULL, NULL, flags);
		}
		else
		{
			WCHAR* drives = _wcsdup(measure->drives.c_str());
			WCHAR* token = wcstok(drives, L"|");
			while (token)
			{
				SHEmptyRecycleBin(NULL, token, flags);
				token = wcstok(NULL, L"|");
			}
			free(drives);
		}
	};
	
	if (_wcsicmp(args, L"EmptyBin") == 0)
	{
		emptyBin(0);
	}
	else if (_wcsicmp(args, L"EmptyBinSilent") == 0)
	{
		emptyBin(SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
	}
	else if (_wcsicmp(args, L"OpenBin") == 0)
	{
		// Open the Recycle Bin folder
		ShellExecute(NULL, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", NULL, SW_SHOW);
	}
}
