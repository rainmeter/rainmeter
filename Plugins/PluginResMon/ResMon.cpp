/*
  Copyright (C) 2004 David Negstad

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

/*
  Requires Win2k or later.  Sorry to anyone still using 9x or ME.
  All my results seem to agree with Task Manager (it gets tedious adding up
  the different process values).  Already proved useful in identifying
  resource leaks in itself (all fixed).  There's irony for you!
*/

#pragma warning(disable: 4996)
#pragma warning(disable: 4786)

#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <math.h>
#include <map>
#include <string>
#include <psapi.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
	__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
	__declspec( dllexport ) UINT Update(UINT id);
	__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
	__declspec( dllexport ) UINT GetPluginVersion();
	__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

// system resources that can be counted
enum COUNTER
{
	GDI_COUNT,
	USER_COUNT,
	HANDLE_COUNT,
	WINDOW_COUNT
};

// list of counter types corresponding to gauges
static std::map<UINT, COUNTER> g_Counters;
static std::map<UINT, std::wstring> g_ProcessNames;

// used to track the number of existing windows
UINT g_WindowCount = 0;

// count the child windows of a system window
BOOL CALLBACK EnumChildProc ( HWND hWndChild, LPARAM lParam )
{
	++g_WindowCount;
	return TRUE;
}

// count the system windows
BOOL CALLBACK EnumWindowProc ( HWND hWnd, LPARAM lParam )
{
	++g_WindowCount;
	EnumChildWindows ( hWnd, EnumChildProc, lParam );
	return TRUE;
}

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
	LPCTSTR type = ReadConfigString(section, L"ResCountType", L"GDI");

	/* Read our own settings from the ini-file */
	if (type)
	{
		if ( _wcsicmp ( L"GDI", type ) == 0 )
		{
			g_Counters[id] = GDI_COUNT;
		}
		else if ( _wcsicmp ( L"USER", type ) == 0 )
		{
			g_Counters[id] = USER_COUNT;
		}
		else if ( _wcsicmp ( L"HANDLE", type ) == 0 )
		{
			g_Counters[id] = HANDLE_COUNT;
		}
		else if ( _wcsicmp ( L"WINDOW", type ) == 0 )
		{
			g_Counters[id] = WINDOW_COUNT;
		}
		else
		{
			std::wstring error = L"GDICountType=";
			error += type;
			error += L" is not valid in measure [";
			error += section;
			error += L"].";
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}

	LPCTSTR process = ReadConfigString(section, L"ProcessName", L"");
	if (process && wcslen(process) > 0)
	{
		g_ProcessNames[id] = process;
	}

	return 0;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
UINT Update(UINT id)
{
	std::map<UINT, COUNTER>::iterator countItor = g_Counters.find ( id );
	if ( countItor == g_Counters.end () )
	{
		return 0;
	}

	COUNTER counter = ( *countItor ).second;

	// count the existing window objects
	if ( counter == WINDOW_COUNT )
	{
		g_WindowCount = 0;
		EnumChildWindows ( NULL, EnumWindowProc, 0 );
		return g_WindowCount;
	}

	const WCHAR* processName = NULL;
	std::map<UINT, std::wstring>::iterator processItor = g_ProcessNames.find ( id );
	if ( processItor != g_ProcessNames.end () )
	{
		processName = ((*processItor).second).c_str();
	}

	DWORD aProcesses[1024];
	DWORD bytesNeeded;
	WCHAR buffer[1024];
	HMODULE hMod[1024];
	DWORD   cbNeeded;

	if ( !EnumProcesses ( aProcesses, sizeof ( aProcesses ), &bytesNeeded ) )
	{
		return 0;
	}

	// step through the running processes
	DWORD flags = PROCESS_QUERY_INFORMATION;

	if (processName)
	{
		flags |= PROCESS_VM_READ;
	}

	UINT resourceCount = 0;
	for ( UINT i = 0; i < bytesNeeded / sizeof ( DWORD ); ++i )
	{
		HANDLE hProcess = OpenProcess ( flags, true, aProcesses[i] );
		if ( hProcess != NULL )
		{
			if (processName)
			{
				if(EnumProcessModules(hProcess, hMod, sizeof(hMod), &cbNeeded))
				{
					if (GetModuleBaseName(hProcess, hMod[0], buffer, sizeof(buffer)))
					{
						if (_wcsicmp(buffer, processName) != 0)
						{
							CloseHandle ( hProcess );
							continue;
						}
					}
					else
					{
						CloseHandle ( hProcess );
						continue;
					}
				}
				else
				{
					CloseHandle ( hProcess );
					continue;
				}
			}

			if ( counter == GDI_COUNT )
			{
				resourceCount += GetGuiResources ( hProcess, GR_GDIOBJECTS );
			}
			else if ( counter == USER_COUNT )
			{
				resourceCount += GetGuiResources ( hProcess, GR_USEROBJECTS );
			}
			else if ( counter == HANDLE_COUNT )
			{
				DWORD tempHandleCount = 0;
				GetProcessHandleCount ( hProcess, &tempHandleCount );
				resourceCount += tempHandleCount;
			}
		}
		CloseHandle ( hProcess );
	}

	return resourceCount;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, COUNTER>::iterator i1 = g_Counters.find(id);
	if (i1 != g_Counters.end())
	{
		g_Counters.erase(i1);
	}

	std::map<UINT, std::wstring>::iterator i2 = g_ProcessNames.find(id);
	if (i2 != g_ProcessNames.end())
	{
		g_ProcessNames.erase(i2);
	}
}

UINT GetPluginVersion()
{
	return 1003;
}

LPCTSTR GetPluginAuthor()
{
	return L"David Negstad";
}