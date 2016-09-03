/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

/*
Rainmeter query interface based on Window Message

The majority of the queries work as detailed below, other queries are explained along with their definitions.
Usage:
1) Post the query to Rainmeter (E.g. QueryRainmeterSkinsPath function)
   * target window : Rainmeter(TrayIcon)
   * message       : WM_QUERY_RAINMETER
   * wParam        : QUERY ID (RAINMETER_QUERY_ID_XXXXX)
   * lParam        : window handle which receives WM_COPYDATA

2) Retrieve the data received from Rainmeter, on WM_COPYDATA
   * COPYDATASTRUCT->dwData : QUERY ID (RAINMETER_QUERY_ID_XXXXX)
   * COPYDATASTRUCT->lpData : requested information. Form depends on you request.
   * COPYDATASTRUCT->cbData : size of lpData

-----
#include <Windows.h>
#include <string>
#include "RainmeterQuery.h"

void QueryRainmeterSkinsPath(HWND hWndSelf)
{
	HWND hWndRainmeter = FindWindow(RAINMETER_QUERY_CLASS_NAME, RAINMETER_QUERY_WINDOW_NAME);
	if (hWndRainmeter)
	{
		PostMessage(hWndRainmeter, WM_QUERY_RAINMETER, RAINMETER_QUERY_ID_SKINS_PATH, (LPARAM)hWndSelf);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COPYDATA:
		{
			COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
			
			// Copy ID and string to local
			DWORD id = cds->dwData;                       // contains QUERY ID (RAINMETER_QUERY_ID_XXXXX)
			std::wstring string = (WCHAR*)cds->lpData;    // contains requested string in wide char

			//
			...
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
-----
*/

#ifndef __RAINMETER_QUERY_H__
#define __RAINMETER_QUERY_H__

#include <tchar.h>

#define RAINMETER_QUERY_CLASS_NAME				TEXT("RainmeterTrayClass")
#define RAINMETER_QUERY_WINDOW_NAME				nullptr

#define WM_QUERY_RAINMETER						WM_APP + 1000
#define WM_QUERY_RAINMETER_RETURN				WM_APP + 1001

/*
QUERY IDs
These Queries return a string in a wide char format
*/
#define RAINMETER_QUERY_ID_SKINS_PATH			4101
#define RAINMETER_QUERY_ID_SETTINGS_PATH		4102
#define RAINMETER_QUERY_ID_PLUGINS_PATH			4103
#define RAINMETER_QUERY_ID_PROGRAM_PATH			4104
#define RAINMETER_QUERY_ID_LOG_PATH				4105
#define RAINMETER_QUERY_ID_CONFIG_EDITOR		4106

/*
These Queries return a numerical value in a direct message, the data 
is stored in the lParam of the message sent to your window, and the msg section
will contain WM_QUERY_RAINMETER_RETURN
*/
#define RAINMETER_QUERY_ID_IS_DEBUGGING			4116

/*QUERY IDs used with WM_COPYDATA
Usage: Send a WM_COPYDATA message to rainmeter via SendMessage().
Rainmeter will set the return value depending on the contents 
of the COPYDATASTRUCT.
*/
#define RAINMETER_QUERY_ID_SKIN_WINDOWHANDLE	5101

/*
This Retuns the Window Handle of the active skin requested by config name in cds.lpData,
or nullptr if the config is not loaded. Currently, the config name is Case-Sensitive.

To requst the data, send a message to Rainmeter in a way similar to this example.
COPYDATASTRUCT cds;
LPWSTR SkinName = L"Gnometer\\Clock";

cds.dwData = 5101;
cds.lpData = SkinName;
cds.cbData = (wcslen(SkinName) + 1) * 2;

HWND hWndMeter = (HWND) SendMessage(hWndRainmeter, WM_COPYDATA, (WPARAM) hWndYourWindow, (LPARAM) &cds);
*/

#endif
