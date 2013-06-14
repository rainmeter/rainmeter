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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <windows.h>
#include <stdio.h>
#include "../../Common/RawString.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions

struct MeasureData
{
	RawString windowName;
	RawString windowClass;
	RawString value;
	WPARAM wParam;
	LPARAM lParam;
	DWORD uMsg;

	MeasureData() : wParam(), lParam(), uMsg() {}
};

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	measure->windowName = RmReadString(rm, L"WindowName", L"");
	measure->windowClass = RmReadString(rm, L"WindowClass", L"");

	DWORD uMsg, wParam, lParam;
	LPCWSTR message = RmReadString(rm, L"WindowMessage", L"");
	if (3 == swscanf(message, L"%u %u %u", &uMsg, &wParam, &lParam))
	{
		measure->uMsg = uMsg;
		measure->wParam = wParam;
		measure->lParam = lParam;
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	HWND hwnd = FindWindow(
		measure->windowClass.empty() ? nullptr : measure->windowClass.c_str(),
		measure->windowName.empty() ? nullptr : measure->windowName.c_str());

	if (hwnd)
	{
		if (measure->uMsg == 0)
		{
			// Get window text
			WCHAR buffer[256];
			GetWindowText(hwnd, buffer, 256);
			measure->value = buffer;
		}
		else
		{
			return (double)SendMessage(hwnd, measure->uMsg, measure->wParam, measure->lParam);
		}
	}
	else if (measure->uMsg == 0)
	{
		measure->value.clear();
	}

	return 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	if (measure->uMsg == 0)
	{
		return measure->value.c_str();
	}

	return nullptr;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	MeasureData* measure = (MeasureData*)data;

	const WCHAR* pos = wcschr(args, L' ');
	if (pos)
	{
		size_t len = pos - args;
		if (_wcsnicmp(args, L"SendMessage", len) == 0)
		{
			++pos;

			// Parse parameters
			DWORD uMsg, wParam, lParam;
			if (3 == swscanf(pos, L"%u %u %u", &uMsg, &wParam, &lParam))
			{
				
				HWND hwnd = FindWindow(
					measure->windowClass.empty() ? nullptr : measure->windowClass.c_str(),
					measure->windowName.empty() ? nullptr : measure->windowName.c_str());

				if (hwnd)
				{
					PostMessage(hwnd, uMsg, wParam, lParam);
				}
				else
				{
					RmLog(LOG_ERROR, L"WindowMessagePlugin.dll: Unable to find window");
				}
			}
			else
			{
				RmLog(LOG_WARNING, L"WindowMessagePlugin.dll: Incorrect number of arguments for bang");
			}

			return;
		}
	}

	RmLog(LOG_WARNING, L"WindowMessagePlugin.dll: Unknown bang");
}
