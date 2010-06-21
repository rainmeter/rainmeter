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

Usage:
1) Post the query to Rainmeter (E.g. QueryRainmeterSkinsPath function)
   * target window : Rainmeter(TrayWindow)
   * message       : WM_QUERY_RAINMETER
   * wParam        : QUERY ID (RAINMETER_QUERY_ID_XXXXX)
   * lParam        : window handle which receives WM_COPYDATA

2) Retrieve the data received from Rainmeter, on WM_COPYDATA
   * COPYDATASTRUCT->dwData : QUERY ID (RAINMETER_QUERY_ID_XXXXX)
   * COPYDATASTRUCT->lpData : requested string in wide char
   * COPYDATASTRUCT->cbData : size of lpData

-----
#include <Windows.h>
#include <string>
#include "RainmeterQuery.h"

void QueryRainmeterSkinsPath(HWND hWndSelf)
{
	HWND hWndRainmeter = FindWindow(RAINMETER_QUERY_WINDOW_TITLE, RAINMETER_QUERY_WINDOW_CLASS);
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

#define RAINMETER_QUERY_WINDOW_NAME				TEXT("RainmeterTrayClass")
#define RAINMETER_QUERY_CLASS_NAME				NULL

#define WM_QUERY_RAINMETER						WM_APP + 1000

// QUERY IDs
#define RAINMETER_QUERY_ID_SKINS_PATH			4101
#define RAINMETER_QUERY_ID_SETTINGS_PATH		4102
#define RAINMETER_QUERY_ID_PLUGINS_PATH			4103

#endif
