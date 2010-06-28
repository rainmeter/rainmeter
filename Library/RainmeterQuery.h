/*
  Copyright (C) 2010 JamesAC, spx

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
Rainmeter query interface based on Window Message

The majority of the queries work as detailed below, other queries are explained along with their definitions.
Usage:
1) Post the query to Rainmeter (E.g. QueryRainmeterSkinsPath function)
   * target window : Rainmeter(TrayWindow)
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
#define RAINMETER_QUERY_WINDOW_NAME				NULL

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
#define RAINMETER_QUERY_ID_COMMAND_LINE			4107
#define RAINMETER_QUERY_ID_STATS_DATE			4108
#define RAINMETER_QUERY_ID_TRAY_EX_L			4109
#define RAINMETER_QUERY_ID_TRAY_EX_R			4110
#define RAINMETER_QUERY_ID_TRAY_EX_M			4111
#define RAINMETER_QUERY_ID_TRAY_EX_DL			4112
#define RAINMETER_QUERY_ID_TRAY_EX_DR			4113
#define RAINMETER_QUERY_ID_TRAY_EX_DM			4114

/*
These Queries return a numerical value in a direct message, the data 
is stored in the lParam of the message sent to your window, and the msg section
will contain WM_QUERY_RAINMETER_RETURN
*/
#define RAINMETER_QUERY_ID_VERSION_CHECK		4115	//0 for disabled, 1 for checking, 2 for new version available
#define RAINMETER_QUERY_ID_IS_DEBUGGING			4116
#define RAINMETER_QUERY_ID_IS_LITESTEP			4117

/*QUERY IDs used with WM_COPYDATA
Usage: Send a WM_COPYDATA message to rainmeter via SendMessage().
Rainmeter will set the return value depending on the contents 
of the COPYDATASTRUCT.
*/
#define RAINMETER_QUERY_ID_SKIN_WINDOWHANDLE	5101

/*
This Retuns the Window Handle of the active skin requested by config name in cds.lpData,
or NULL if the config is not loaded. Currently, the config name is Case-Sensitive.

To requst the data, send a message to Rainmeter in a way similar to this example.
COPYDATASTRUCT cds;
LPWSTR SkinName = L"Gnometer\\Clock";

cds.dwData = 5101;
cds.lpData = SkinName;
cds.cbData = (wcslen(SkinName) + 1) * 2;

HWND hWndMeter = (HWND) SendMessage(hWndRainmeter, WM_COPYDATA, (WPARAM) hWndYourWindow, (LPARAM) &cds);
*/

#endif
