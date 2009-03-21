/*
  Copyright (C) 2001 Kimmo Pekkola

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

#pragma warning(disable : 4996)

#include "resource.h"
#include "..\Library\Rainmeter.h"

/*
** Protos
*/
BOOL InitApplication(HINSTANCE hInstance, const WCHAR* WinClass);
HWND InitInstance(HINSTANCE hInstance, const WCHAR* WinClass, const WCHAR* WinName);
LONG APIENTRY MainWndProc(HWND, UINT, UINT, LONG);
void Bang(HWND hWnd, const WCHAR* command);

/*
** Stuff from the DLL
*/
extern "C" EXPORT_PLUGIN int initModuleEx(HWND ParentWnd, HINSTANCE dllInst, LPCSTR);
extern "C" EXPORT_PLUGIN void quitModule(HINSTANCE dllInst);
extern "C" EXPORT_PLUGIN void Initialize(bool DummyLS, LPCTSTR CmdLine);
extern "C++" CRainmeter* Rainmeter;

/* 
** WinMain
**
** The Main-function
**
*/
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	WCHAR* WinClass = L"DummyRainWClass";
	WCHAR* WinName = L"Rainmeter control window";
	HWND hWnd;

	if(!hPrevInstance) 
	{
		if (!InitApplication(hInstance, WinClass)) return FALSE;
	}

	hWnd=InitInstance(hInstance, WinClass, WinName);
	if(!hWnd) return FALSE;

	if (lpCmdLine[0] == '!')
	{
		// It's a !bang
		Bang(hWnd, lpCmdLine);
		return 0;
	}

	// Remove quotes from the commandline
	WCHAR Path[256];
	Path[0] = 0;
	int Pos = 0;
	if(lpCmdLine)
	{
		for(size_t i = 0; i <= wcslen(lpCmdLine); i++) 
		{
			if(lpCmdLine[i] != L'\"') Path[Pos++] = lpCmdLine[i];
		}
	}

	// Initialize from exe
	Initialize(true, Path);

	// Check that the DLL is available
	HMODULE module = GetModuleHandle(L"Rainmeter.dll");
	if(module == NULL)
	{
		MessageBox(NULL, L"Unable to load Rainmeter.dll", L"Rainmeter", MB_OK);
		return 0;
	}

	// Initialize the DLL
	initModuleEx(hWnd, module, NULL);

	// Run the standard window message loop
	while(GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg); 
	} 

	return (int)msg.wParam; 
} 

/* 
** InitApplication
**
** Creates the windowclass
**
*/
BOOL InitApplication(HINSTANCE hInstance, const WCHAR* WinClass)
{
	WNDCLASS  wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC) MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_RAINMETER));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); 
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = WinClass;

	return RegisterClass(&wc);
}

/* 
** InitInstance
**
** Creates the window. This is just an invisible window. The real window
** is created by the DLL.
**
*/
HWND InitInstance(HINSTANCE hInstance, const WCHAR* WinClass, const WCHAR* WinName)
{
	return CreateWindowEx(
		WS_EX_TOOLWINDOW,
		WinClass,
		WinName,
		WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);
}

/* 
** Bang
**
** Sends bangs to the DLL
**
*/
void Bang(HWND hWnd, const WCHAR* command)
{
	// Check if Rainlendar is running
	HWND wnd = FindWindow(L"RainmeterMeterWindow", NULL);

	if (wnd != NULL)
	{
		COPYDATASTRUCT copyData;

		copyData.dwData = 1;
		copyData.cbData = (DWORD)((wcslen(command) + 1) * sizeof(WCHAR));
		copyData.lpData = (void*)command;

		// Send the bang to the Rainlendar window
		SendMessage(wnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&copyData);
	}
	else
	{
		MessageBox(hWnd, L"Rainmeter is not running.\nUnable to send the !bang to it.", L"Rainmeter", MB_OK);
	}
}

/* 
** MainWndProc
**
** The main window procedure
**
*/
LONG APIENTRY MainWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch(message) {

	case WM_DESTROY:
		{
			quitModule(NULL);
			PostQuitMessage(0);
		}
		break;

	default:
		return (LONG)DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
