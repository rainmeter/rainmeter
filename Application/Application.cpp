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

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include "resource.h"
#include "..\Library\Rainmeter.h"

/*
** Protos
*/
BOOL InitApplication(HINSTANCE hInstance, const WCHAR* WinClass);
HWND InitInstance(HINSTANCE hInstance, const WCHAR* WinClass, const WCHAR* WinName);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void Bang(const WCHAR* command);

/*
** Stuff from the DLL
*/
extern "C" EXPORT_PLUGIN int initModuleEx(HWND ParentWnd, HINSTANCE dllInst, LPCSTR);
extern "C" EXPORT_PLUGIN void quitModule(HINSTANCE dllInst);
extern "C" EXPORT_PLUGIN void Initialize(bool DummyLS, LPCTSTR CmdLine);
extern "C" EXPORT_PLUGIN void ExecuteBang(LPCTSTR szBang);

const WCHAR* WinClass = L"DummyRainWClass";
const WCHAR* WinName = L"Rainmeter control window";

/* 
** WinMain
**
** The Main-function
**
*/
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HWND hWnd;

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetBreakAlloc(5055);

	if (lpCmdLine && lpCmdLine[0] == L'!')
	{
		// It's a !bang
		Bang(lpCmdLine);
		return 0;
	}

	if(!hPrevInstance) 
	{
		if (!InitApplication(hInstance, WinClass)) return FALSE;
	}

	hWnd=InitInstance(hInstance, WinClass, WinName);
	if(!hWnd) return FALSE;

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
void Bang(const WCHAR* command)
{
	// Check if Rainmeter is running
	HWND wnd = FindWindow(WinClass, WinName);
	if (wnd != NULL)
	{
		COPYDATASTRUCT copyData;

		copyData.dwData = 1;
		copyData.cbData = (DWORD)((wcslen(command) + 1) * sizeof(WCHAR));
		copyData.lpData = (void*)command;

		// Send the bang to the Rainmeter window
		SendMessage(wnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copyData);
	}
	else
	{
		if (wcsicmp(L"!rainmeterquit", command) != 0)
		{
			MessageBox(NULL, L"Rainmeter is not running.\nUnable to send the !bang to it.", L"Rainmeter", MB_OK);
		}
	}
}

/* 
** MainWndProc
**
** The main window procedure
**
*/
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {

	case WM_DESTROY:
		{
			quitModule(NULL);
			PostQuitMessage(0);
		}
		break;

	case WM_COPYDATA:
		{
			COPYDATASTRUCT* pCopyDataStruct = (COPYDATASTRUCT*) lParam;
			if (pCopyDataStruct && (pCopyDataStruct->dwData == 1) && (pCopyDataStruct->cbData > 0))
			{
				ExecuteBang((const WCHAR*)pCopyDataStruct->lpData);
			}
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
