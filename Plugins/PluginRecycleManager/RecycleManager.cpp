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

------------

Usage:

[MeasureBin1]
Measure=Plugin
Plugin=RecycleManager.dll
Drives=C:                     (for total system bin, use Drives=ALL  ::  for multiple drives use | as a delimiter e.g. Drives=A:|C:|D: will give the sum of these drives)
RecycleType=COUNT             (type can be COUNT for nomber of items or SIZE, for size in bytes)
Update=1000
UpdateDivider=10


*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <stdlib.h>
#include <math.h>
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
	__declspec( dllexport ) void ExecuteBang(LPCTSTR args, UINT id);
}

//const int NUMRECYCLE  = 1;
//const int SIZERECYCLE = 2;

//static std::map<UINT, int> g_Values;

// system resources that can be counted
enum MEASURETYPE
{
	NUMRECYCLE,
	SIZERECYCLE
};

// list of counter types corresponding to gauges
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
	MEASURETYPE dataType = NUMRECYCLE;  // 1 for numRecycled, 2 for sizeRecycled
	
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
			std::wstring error = L"RecycleType=";
			error += type;
			error += L" is not valid in measure [";
			error += section;
			error += L"].";
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}
	
	g_Values[id] = dataType;
	
	LPCTSTR drives = ReadConfigString(section, L"Drives", L"ALL");
	if (drives && wcslen(drives) > 0)
	{
		g_DriveList[id] = drives;
	}
	else
	{
		g_DriveList[id] = L"ALL";
	}
	
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
	MEASURETYPE dataType = g_Values[id];
	std::wstring driveSet = g_DriveList[id];
	
	SHQUERYRBINFO RecycleBinInfo = { 0 };  
	RecycleBinInfo.cbSize = sizeof( RecycleBinInfo ); // Tell size of structure  
	
	if(_wcsicmp(driveSet.c_str(), L"ALL") == 0)
	{
		if(SHQueryRecycleBin( NULL, &RecycleBinInfo ) == S_OK)
		{
			if (dataType == SIZERECYCLE)
			{
				return (double)RecycleBinInfo.i64Size; // size in bytes
			}
			else if (dataType == NUMRECYCLE)
			{
				return (double)RecycleBinInfo.i64NumItems; // number of items in bin
			}
			return 0;
		}
		else
		{
			driveSet = L"A:|B:|C:|D:|E:|F:|G:|H:|I:|J:|K:|L:|M:|N:|O:|P:|Q:|R:|S:|T:|U:|V:|W:|X:|Y:|Z:";
		}
	}
	std::vector<std::wstring> tokens;
	std::wstring toSplit(driveSet.begin(), driveSet.end()); 
	double retVal = 0;
	Tokenize(toSplit, tokens, L"|");
	
	for(int i=0;i < tokens.size(); i++)
	{
		double tempVal;
		std::wstring strd = tokens.at(i);
		SHQueryRecycleBin( strd.c_str(), &RecycleBinInfo ); // Get recycle bin info  
		if (dataType == SIZERECYCLE)
		{
			tempVal = (double)RecycleBinInfo.i64Size; // size in bytes
		}
		else if (dataType == NUMRECYCLE)
		{
			tempVal = (double)RecycleBinInfo.i64NumItems; // number of items in bin
		}
		retVal += tempVal;
	}
	
	return (retVal);
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
    std::wstring bang     = args;
	std::wstring driveSet = g_DriveList[id];

    if (_wcsicmp(bang.c_str(), L"EmptyBin") == 0)
    { //Empty the Recycle Bin
		if(_wcsicmp(driveSet.c_str(), L"ALL") == 0)
		{
			if(SHEmptyRecycleBin( NULL, NULL, NULL ) == S_OK)
			{
				return;
			}
			else
			{
				driveSet = L"A:|B:|C:|D:|E:|F:|G:|H:|I:|J:|K:|L:|M:|N:|O:|P:|Q:|R:|S:|T:|U:|V:|W:|X:|Y:|Z:";
			}
		}
		std::vector<std::wstring> tokens;
		std::wstring toSplit(driveSet.begin(), driveSet.end()); 
		Tokenize(toSplit, tokens, L"|");
		
		for(int i=0;i < tokens.size(); i++)
		{
			std::wstring strd = tokens.at(i);
			SHEmptyRecycleBin( NULL, strd.c_str(), NULL ); // empty bin
		}
		return;
    }
    else
    {
		if (_wcsicmp(bang.c_str(), L"EmptyBinSilent") == 0)
        { //Empty the Recycle Bin (no prompt)
			if(_wcsicmp(driveSet.c_str(), L"ALL") == 0)
			{
				if(SHEmptyRecycleBin( NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND ) == S_OK)
				{
					return;
				}
				else
				{
					driveSet = L"A:|B:|C:|D:|E:|F:|G:|H:|I:|J:|K:|L:|M:|N:|O:|P:|Q:|R:|S:|T:|U:|V:|W:|X:|Y:|Z:";
				}
			}
			std::vector<std::wstring> tokens;
			std::wstring toSplit(driveSet.begin(), driveSet.end()); 
			Tokenize(toSplit, tokens, L"|");

			for(int i=0;i < tokens.size(); i++)
			{
				std::wstring strd = tokens.at(i);
				SHEmptyRecycleBin( NULL, strd.c_str(), SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND ); // empty bin
			}
			return;
        }
        else if (_wcsicmp(bang.c_str(), L"OpenBin") == 0)
        { //Open the Recycle Bin folder
            //system("explorer.exe /N,::{645FF040-5081-101B-9F08-00AA002F954E}");
			std::wstring szCmd = L"explorer.exe";
			std::wstring szParm= L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}";
			ShellExecute(NULL,L"open",szCmd.c_str(),szParm.c_str(),NULL,SW_SHOW);
			return;
        }
    }
}

