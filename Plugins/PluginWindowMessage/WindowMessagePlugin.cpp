/*
  Copyright (C) 2005 Kimmo Pekkola

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
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <time.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
__declspec( dllexport ) void ExecuteBang(LPCTSTR args, UINT id);
}

struct windowData
{
	std::wstring windowName;
	std::wstring windowClass;
	WPARAM wParam;
	LPARAM lParam;
	DWORD uMsg;
	std::wstring value;
	DWORD result;
};

static std::map<UINT, windowData> g_Values;

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
	windowData wData;

	wData.uMsg = 0;
	wData.wParam = 0;
	wData.lParam = 0;
	wData.result = 0;

	/* Read our own settings from the ini-file */
	LPCTSTR data = ReadConfigString(section, L"WindowName", NULL);
	if (data)
	{
		wData.windowName = data;
	}

	data = ReadConfigString(section, L"WindowClass", NULL);
	if (data)
	{
		wData.windowClass = data;
	}

	data = ReadConfigString(section, L"WindowMessage", NULL);
	if (data)
	{
		DWORD uMsg, wParam, lParam;
		if (3 == swscanf(data, L"%u %u %u", &uMsg, &wParam, &lParam))
		{
			wData.uMsg = uMsg;
			wData.wParam = wParam;
			wData.lParam = lParam;
		}
	}

	g_Values[id] = wData;

	return 0;
}

/*
This function is called when new value should be measured.
The function returns the new value.
*/
double Update2(UINT id)
{	
	std::map<UINT, windowData>::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		std::wstring& winName = (*i).second.windowName;
		std::wstring& winClass = (*i).second.windowClass;
		HWND hwnd = FindWindow(winClass.empty() ? NULL : winClass.c_str(), winName.empty() ? NULL : winName.c_str());
		if (hwnd)
		{
			if ((*i).second.uMsg == 0)
			{
				// Get window text
				WCHAR buffer[1024];
				buffer[0] = 0;
				GetWindowText(hwnd, buffer, 1024);
				(*i).second.value = buffer;
			}
			else
			{
				(*i).second.result = (DWORD)SendMessage(hwnd, (*i).second.uMsg, (*i).second.wParam, (*i).second.lParam);
				return (int)((*i).second.result);
			}
		}
	}
	return 0;
}

LPCTSTR GetString(UINT id, UINT flags) 
{
	static WCHAR buffer[256];

	std::map<UINT, windowData>::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		if (((*i).second).value.empty())
		{
			_itow(((*i).second).result, buffer, 10);
			return buffer;
		}
		else
		{
			return ((*i).second).value.c_str();
		}
	}

	return NULL;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, windowData>::iterator i1 = g_Values.find(id);
	if (i1 != g_Values.end())
	{
		g_Values.erase(i1);
	}
}

UINT GetPluginVersion()
{
	return 1001;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}

void ExecuteBang(LPCTSTR args, UINT id)
{
	std::wstring wholeBang = args;

	size_t pos = wholeBang.find(' ');
	if (pos != -1)
	{
		std::wstring bang = wholeBang.substr(0, pos);
		wholeBang.erase(0, pos + 1);

		if (_wcsicmp(bang.c_str(), L"SendMessage") == 0)
		{
			// Parse parameters
			DWORD uMsg, wParam, lParam;
			if (3 == swscanf(wholeBang.c_str(), L"%u %u %u", &uMsg, &wParam, &lParam))
			{
				std::map<UINT, windowData>::iterator i = g_Values.find(id);
				if (i != g_Values.end())
				{
					std::wstring& winName = (*i).second.windowName;
					std::wstring& winClass = (*i).second.windowClass;
					HWND hwnd = FindWindow(winClass.empty() ? NULL : winClass.c_str(), winName.empty() ? NULL : winName.c_str());
					if (hwnd)
					{
						PostMessage(hwnd, uMsg, wParam, lParam);
					}
					else
					{
						LSLog(LOG_DEBUG, L"Rainmeter", L"WindowMessagePlugin: Unable to find the window!");
					}
				}
				else
				{
					LSLog(LOG_DEBUG, L"Rainmeter", L"WindowMessagePlugin: Unable to find the window data!");
				}
			}
			else
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"WindowMessagePlugin: Incorrect number of arguments for the bang!");
			}
		}
		else
		{
			LSLog(LOG_DEBUG, L"Rainmeter", L"WindowMessagePlugin: Unknown bang!");
		}
	}
	else
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"WindowMessagePlugin: Unable to parse the bang!");
	}
}
