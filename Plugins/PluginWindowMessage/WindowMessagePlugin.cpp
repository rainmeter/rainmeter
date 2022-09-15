/* Copyright (C) 2005 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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

	void* rm;

	MeasureData() : wParam(0), lParam(0), uMsg(0UL), rm(nullptr) {}
};

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;

	measure->rm = rm;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	measure->windowName = RmReadString(rm, L"WindowName", L"");
	measure->windowClass = RmReadString(rm, L"WindowClass", L"");

	DWORD uMsg = 0UL, wParam = 0UL, lParam = 0UL;
	LPCWSTR message = RmReadString(rm, L"WindowMessage", L"");
	if (3 == swscanf_s(message, L"%u %u %u", &uMsg, &wParam, &lParam))
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
			WCHAR buffer[256] = { 0 };
			GetWindowText(hwnd, buffer, _countof(buffer));
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
	measure = nullptr;
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
			DWORD uMsg = 0UL, wParam = 0UL, lParam = 0UL;
			if (3 == swscanf_s(pos, L"%u %u %u", &uMsg, &wParam, &lParam))
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
					RmLog(measure->rm, LOG_ERROR, L"WindowMessagePlugin.dll: Unable to find window");
				}
			}
			else
			{
				RmLog(measure->rm, LOG_WARNING, L"WindowMessagePlugin.dll: Incorrect number of arguments for bang");
			}

			return;
		}
	}

	RmLog(measure->rm, LOG_WARNING, L"WindowMessagePlugin.dll: Unknown bang");
}
