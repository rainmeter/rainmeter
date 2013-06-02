/*
  Copyright (C) 2010-2011 Patrick Dubbert

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

#include "DexpotMeasure.h"

#include <tchar.h>

#include "DexpotConstants.h"
#include "../../Library/Export.h"

int DexpotMeasure::InstanceCount = 0;
HWND DexpotMeasure::hWndDexpot = nullptr;
HWND DexpotMeasure::hWndMessageWindow = nullptr;
std::set<DexpotMeasure*> DexpotMeasure::DexpotMeasures;
TCHAR DexpotMeasure::StringBuffer[STRINGBUFFER_SIZE];
UINT DexpotMeasure::WM_DEXPOTSTARTED = RegisterWindowMessage(_T("DexpotStarted"));
BOOL DexpotMeasure::PluginRegistered = FALSE;
HWND DexpotMeasure::hWndRainmeterControl = nullptr;
int DexpotMeasure::CurrentDesktop = 0;

std::vector<std::wstring> DexpotDesktopNameMeasure::DesktopNames;
std::vector<std::wstring> DexpotDesktopWallpaperMeasure::DesktopWallpapers;

DexpotMeasure::DexpotMeasure(HMODULE instance, UINT _id) : VDMeasure(instance, _id)
{
}

DexpotMeasure* DexpotMeasure::CreateMeasure(HMODULE instance, UINT id, LPCTSTR iniFile, LPCTSTR section)
{
	std::wstring TypeString(ReadConfigString(section, _T("VDMeasureType"), _T("")));

	if (TypeString == _T("VDMActive")) return new DexpotVDMActiveMeasure(instance, id);
	else if (TypeString == _T("DesktopCount")) return new DexpotDesktopCountMeasure(instance, id);
	else if (TypeString == _T("CurrentDesktop")) return new DexpotCurrentDesktopMeasure(instance, id);
	else if (TypeString == _T("SwitchDesktop")) return new DexpotSwitchDesktopMeasure(instance, id);
	else if (TypeString == _T("Screenshot")) return new DexpotScreenshotMeasure(instance, id);
	else if (TypeString == _T("DesktopName")) return new DexpotDesktopNameMeasure(instance, id);
	else if (TypeString == _T("DesktopWallpaper")) return new DexpotDesktopWallpaperMeasure(instance, id);
	else if (TypeString == _T("Command")) return new DexpotCommandMeasure(instance, id);

	return nullptr;
}

UINT DexpotMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	if (InstanceCount == 0)
	{
		hWndRainmeterControl = FindWindow(_T("DummyRainWClass"), _T("Rainmeter control window"));
		hWndMessageWindow = CreateMessageWindow();
	}
	InstanceCount++;

	if (!PluginRegistered && FindDexpotWindow())
	{
		SendNotifyMessage(hWndDexpot, DEX_REGISTERPLUGIN, 0, (LPARAM) hWndMessageWindow);
		CurrentDesktop = (int) SendMessage(hWndDexpot, DEX_GETCURRENTDESKTOP, 0, 0);
		PluginRegistered = TRUE;
	}

	InitializeData();
	DexpotMeasures.insert(this);

	return 0;
}

void DexpotMeasure::Finalize()
{
	InstanceCount--;
	if (InstanceCount == 0)
	{
		if (PluginRegistered)
		{
			SendNotifyMessage(hWndDexpot, DEX_UNREGISTERPLUGIN, 0, (LPARAM) hWndMessageWindow);
			PluginRegistered = FALSE;
		}
		DestroyWindow(hWndMessageWindow);
		UnregisterClass(_T("DexpotPluginWindowClass"), hInstance);
	}
	DexpotMeasures.erase(this);
}

UINT DexpotMeasure::Update()
{
	return 0;
}

LPCTSTR DexpotMeasure::GetString(UINT flags)
{
	_stprintf_s(StringBuffer, STRINGBUFFER_SIZE, _T("%i"), Update());
	return StringBuffer;
}

void DexpotMeasure::OnDexpotStarted()
{
	InitializeData();
}

BOOL DexpotMeasure::FindDexpotWindow()
{
	if (IsWindow(hWndDexpot)) return TRUE;
	hWndDexpot = FindWindow(DEXPOTCLASS, DEXPOTTITLE);
	return hWndDexpot != nullptr;
}

HWND DexpotMeasure::CreateMessageWindow()
{
	WNDCLASS wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.hInstance = hInstance;
	wc.lpszClassName = _T("DexpotPluginWindowClass");
	wc.lpfnWndProc = WindowProc;
	RegisterClass(&wc);

	HWND hWnd = CreateWindowEx(0, _T("DexpotPluginWindowClass"), _T("Dexpot Rainmeter Plugin"), 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	SetWindowLong(hWnd, GWL_STYLE, 0);
	SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	MoveWindow(hWnd, 0, 0, 0, 0, FALSE);

	return hWnd;
}

void DexpotMeasure::SendBang(std::wstring &Bang)
{
	COPYDATASTRUCT cds;

	cds.dwData = 1;
	cds.cbData = (DWORD) (Bang.length() + 1) * sizeof(wchar_t);
	cds.lpData = (PVOID) Bang.c_str();

	SendMessage(hWndRainmeterControl, WM_COPYDATA, (WPARAM) hWndMessageWindow, (LPARAM) &cds);
}

LRESULT CALLBACK DexpotMeasure::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case DEX_SWITCHED:
		CurrentDesktop = HIWORD(lParam);
		for (std::set<DexpotMeasure*>::iterator i = DexpotMeasures.begin(); i != DexpotMeasures.end(); ++i)
		{
			(*i)->OnSwitched(LOWORD(lParam), HIWORD(lParam), LOWORD(wParam), HIWORD(wParam));
		}
		return 0;

	case DEX_DESKTOPCOUNTCHANGED:
		for (std::set<DexpotMeasure*>::iterator i = DexpotMeasures.begin(); i != DexpotMeasures.end(); ++i)
		{
			(*i)->OnDesktopCountChanged((int)wParam);
		}
		return 0;

	case DEX_SHUTDOWN:
		PluginRegistered = FALSE;
		for (std::set<DexpotMeasure*>::iterator i = DexpotMeasures.begin(); i != DexpotMeasures.end(); ++i)
		{
			(*i)->OnShutdown();
		}
		return 0;

	case DEX_DESKTOPCONFIGURATIONCHANGED:
		for (std::set<DexpotMeasure*>::iterator i = DexpotMeasures.begin(); i != DexpotMeasures.end(); ++i)
		{
			(*i)->OnDesktopConfigurationChanged();
		}
		return 0;

	case WM_COPYDATA:
		if ((HWND) wParam == hWndDexpot)
		{
			COPYDATASTRUCT *cds = (COPYDATASTRUCT*) lParam;
			switch(LOWORD(cds->dwData))
			{
			case DEX_GETDESKTOPTITLE:
				DexpotDesktopNameMeasure::SetDesktopName(HIWORD(cds->dwData), std::wstring((LPTSTR) cds->lpData));
				break;
			case DEX_GETDESKTOPWALLPAPER:
				DexpotDesktopWallpaperMeasure::SetDesktopWallpaper(HIWORD(cds->dwData), std::wstring((LPTSTR) cds->lpData));
				break;
			}
		}
		return 0;

	default:
		if (message == WM_DEXPOTSTARTED)
		{
			hWndDexpot = (HWND) wParam;
			if (!hWndDexpot) FindDexpotWindow();
			if (hWndDexpot)
			{
				SendMessage(hWndDexpot, DEX_REGISTERPLUGIN, 0, (LPARAM) hWndMessageWindow);
				CurrentDesktop = (int) SendMessage(hWndDexpot, DEX_GETCURRENTDESKTOP, 0, 0);
				PluginRegistered = TRUE;
			}
			for (std::set<DexpotMeasure*>::iterator i = DexpotMeasures.begin(); i != DexpotMeasures.end(); ++i)
			{
				(*i)->OnDexpotStarted();
			}
			return 0;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


/*
 * DexpotDesktopCountMeasure
 *
 */
DexpotDesktopCountMeasure::DexpotDesktopCountMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {}

UINT DexpotDesktopCountMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	DesktopCount = 0;
	OnChange = ReadConfigString(section, _T("VDOnChange"), _T(""));

	CountType = Total;
	LPCTSTR TypeString = ReadConfigString(section, _T("VDDesktopCount"), _T(""));
	if (_tcsicmp(TypeString, _T("X")) == 0) CountType = Columns;
	else if (_tcsicmp(TypeString, _T("Y")) == 0) CountType = Rows;

	DexpotMeasure::Initialize(iniFile, section);
	return 20;
}

void DexpotDesktopCountMeasure::InitializeData()
{
	if (PluginRegistered) DesktopCount = (int) SendMessage(hWndDexpot, DEX_GETDESKTOPCOUNT, 0, 0);
}

UINT DexpotDesktopCountMeasure::Update()
{
	if (CountType == Rows) return 1;
	else return DesktopCount;
}

void DexpotDesktopCountMeasure::OnDesktopCountChanged(int NewCount)
{
	DesktopCount = NewCount;
	if (OnChange.length()) SendBang(OnChange);
}


/*
 * DexpotCurrentDesktopMeasure
 *
 */
DexpotCurrentDesktopMeasure::DexpotCurrentDesktopMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {}

UINT DexpotCurrentDesktopMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	OnChange = ReadConfigString(section, _T("VDOnChange"), _T(""));

	DexpotMeasure::Initialize(iniFile, section);
	return 0;
}

UINT DexpotCurrentDesktopMeasure::Update()
{
	return CurrentDesktop;
}

void DexpotCurrentDesktopMeasure::OnSwitched(int FromDesktop, int ToDesktop, WORD Flags, WORD Trigger)
{
	if (OnChange.length()) SendBang(OnChange);
}


/*
 * DexpotVDMActiveMeasure
 *
 */
DexpotVDMActiveMeasure::DexpotVDMActiveMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {};

UINT DexpotVDMActiveMeasure::Update()
{
	return PluginRegistered ? 1 : 0;
}

UINT DexpotVDMActiveMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	OnActivate = ReadConfigString(section, _T("VDOnActivate"), _T(""));
	OnDeactivate = ReadConfigString(section, _T("VDOnDeactivate"), _T(""));

	DexpotMeasure::Initialize(iniFile, section);
	return 1;
}

void DexpotVDMActiveMeasure::OnShutdown()
{
	if (OnDeactivate.length()) SendBang(OnDeactivate);
}

void DexpotVDMActiveMeasure::OnDexpotStarted()
{
	if (OnActivate.length()) SendBang(OnActivate);
}


/*
 * DexpotSwitchDesktopMeasure
 *
 */
DexpotSwitchDesktopMeasure::DexpotSwitchDesktopMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {}

void DexpotSwitchDesktopMeasure::ExecuteBang(LPCTSTR args)
{
	if (PluginRegistered)
	{
		DWORD Desktop;

		if (_tcsicmp(args, _T("next")) == 0) Desktop = MAKELPARAM(0, 1);
		else if (_tcsicmp(args, _T("prev")) == 0) Desktop = MAKELPARAM(0, 2);
		else if (_tcsicmp(args, _T("back")) == 0) Desktop = MAKELPARAM(0, 3);
		else Desktop = _ttoi(args);

		SendNotifyMessage(hWndDexpot, DEX_SWITCHDESKTOP, 0, Desktop);
	}
}


/*
 * DexpotScreenshotMeasure
 *
 */
DexpotScreenshotMeasure::DexpotScreenshotMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {}

UINT DexpotScreenshotMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	OutputFile = ReadConfigString(section, _T("VDOutputFile"), _T(""));
	DesktopNumber = _ttoi(ReadConfigString(section, _T("VDDesktop"), _T("0")));
	Width = _ttoi(ReadConfigString(section, _T("VDWidth"), _T("0")));
	Height = _ttoi(ReadConfigString(section, _T("VDHeight"), _T("0")));
	RefreshOnUpdate = _ttoi(ReadConfigString(section, _T("VDRefreshOnUpdate"), _T("0")));

	DexpotMeasure::Initialize(iniFile, section);
	return 0;
}

UINT DexpotScreenshotMeasure::Update()
{
	if (RefreshOnUpdate && (DesktopNumber == 0 || DesktopNumber == CurrentDesktop))
	{
		UpdateScreenshot();
	}
	return 0;
}

LPCTSTR DexpotScreenshotMeasure::GetString(UINT flags)
{
	return OutputFile.c_str();
}

void DexpotScreenshotMeasure::InitializeData()
{
	UpdateScreenshot();
}

void DexpotScreenshotMeasure::OnSwitched(int FromDesktop, int ToDesktop, WORD Flags, WORD Trigger)
{
	if (DesktopNumber == FromDesktop || DesktopNumber == 0)
	{
		UpdateScreenshot();
	}
}

void DexpotScreenshotMeasure::UpdateScreenshot()
{
	int Desktop = DesktopNumber == 0 ? CurrentDesktop : DesktopNumber;
	int nBytes = 0;
	BYTE *pBytes = nullptr;
	HANDLE fm;
	HANDLE mutex;

	if (!IsWindow(hWndDexpot)) return;

	int DesktopWidth = (int) SendMessage(hWndDexpot, DEX_GETDESKTOPWIDTH, Desktop, 0);
	int DesktopHeight = (int) SendMessage(hWndDexpot, DEX_GETDESKTOPHEIGHT, Desktop, 0);

	mutex = OpenMutex(SYNCHRONIZE, FALSE, _T("DexpotScreenshotMutex"));
	WaitForSingleObject(mutex, 2000);
	fm = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, DesktopWidth * DesktopHeight * 4, L"Local\\DexpotScreenshotFilemap");
	pBytes = (BYTE*) MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pBytes) nBytes = (int) SendMessage(hWndDexpot, DEX_GETSCREENSHOT, Desktop, 0);

	if (nBytes > 0 && nBytes == DesktopWidth * DesktopHeight * 4)
	{
		HDC ScreenDC;
		HDC MemDC;
		HDC MemDC2;
		HBITMAP OriginalBitmap;
		HBITMAP ScaledBitmap;
		HGDIOBJ OldBitmap;
		HGDIOBJ OldBitmap2;
		BYTE *ScaledBytes;
		BITMAPINFO bmi;
		BITMAPFILEHEADER bmfh;
		int ScaledHeight = Height;
		int ScaledWidth = Width;

		if (ScaledHeight == 0) ScaledHeight = (int) ((float) DesktopHeight * (ScaledWidth / (float) DesktopWidth) + .5f);
		if (ScaledWidth == 0) ScaledWidth = (int) ((float) DesktopWidth * (ScaledHeight / (float) DesktopHeight) + .5f);
		if (ScaledHeight == 0) ScaledHeight = DesktopHeight;
		if (ScaledWidth == 0) ScaledWidth = DesktopWidth;

		ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = DesktopWidth;
		bmi.bmiHeader.biHeight = DesktopHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = nBytes;

		ScreenDC = GetDC(nullptr);
		MemDC = CreateCompatibleDC(ScreenDC);
		MemDC2 = CreateCompatibleDC(ScreenDC);
		OriginalBitmap = CreateCompatibleBitmap(ScreenDC, DesktopWidth, DesktopHeight);
		SetDIBits(MemDC2, OriginalBitmap, 0, DesktopHeight, pBytes, &bmi, 0);
		OldBitmap2 = SelectObject(MemDC2, (HGDIOBJ) OriginalBitmap);

		nBytes = ScaledWidth * ScaledHeight * 4;
		bmi.bmiHeader.biWidth = ScaledWidth;
		bmi.bmiHeader.biHeight = ScaledHeight;
		bmi.bmiHeader.biSizeImage = nBytes;

		ScaledBitmap = CreateDIBSection(MemDC, &bmi, 0, (void**) &ScaledBytes, nullptr, 0);
		OldBitmap = SelectObject(MemDC, (HGDIOBJ) ScaledBitmap);
		SetStretchBltMode(MemDC, HALFTONE);
		StretchBlt(MemDC, 0, 0, ScaledWidth, ScaledHeight, MemDC2, 0, 0, DesktopWidth, DesktopHeight, SRCCOPY);
		GdiFlush();

		ZeroMemory(&bmfh, sizeof(BITMAPFILEHEADER));
		bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bmfh.bfSize = bmfh.bfOffBits + nBytes;
		bmfh.bfType = 0x4d42;

		FILE* file = _wfopen(OutputFile.c_str(), L"wb");
		if (file)
		{
			fwrite(&bmfh, sizeof(BITMAPFILEHEADER), 1, file);
			fwrite(&bmi, sizeof(BITMAPINFOHEADER), 1, file);
			fwrite(ScaledBytes, nBytes, 1, file);
			fclose(file);
		}

		SelectObject(MemDC, OldBitmap);
		SelectObject(MemDC2, OldBitmap2);
		DeleteObject(ScaledBitmap);
		DeleteObject(OriginalBitmap);
		DeleteDC(MemDC);
		DeleteDC(MemDC2);
		ReleaseDC(nullptr, ScreenDC);
	}

	UnmapViewOfFile(pBytes);
	CloseHandle(fm);
	ReleaseMutex(mutex);
	CloseHandle(mutex);
}


/*
 * DexpotDesktopNameMeasure
 *
 */
DexpotDesktopNameMeasure::DexpotDesktopNameMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {}

UINT DexpotDesktopNameMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	DesktopNumber = _ttoi(ReadConfigString(section, _T("VDDesktop"), _T("0")));
	return DexpotMeasure::Initialize(iniFile, section);
}

LPCTSTR DexpotDesktopNameMeasure::GetString(UINT flags)
{
	UINT Desktop = (DesktopNumber == 0 ? CurrentDesktop : DesktopNumber) - 1;
	if (Desktop >= 0 && Desktop < DesktopNames.size())
	{
		return DesktopNames[Desktop].c_str();
	}
	else
	{
		StringBuffer[0] = _T('\0');
		return StringBuffer;
	}
}

void DexpotDesktopNameMeasure::InitializeData()
{
	if (PluginRegistered)
	{
		int DesktopCount = (int) SendMessage(hWndDexpot, DEX_GETDESKTOPCOUNT, 0, 0);
		DesktopNames.resize(DesktopCount);
		if (DesktopNumber == 0)
		{
			for (int i = 1; i <= DesktopCount; i++)
			{
				SendMessage(hWndDexpot, DEX_GETDESKTOPTITLE, i, (LPARAM) hWndMessageWindow);
			}
		}
		else if (DesktopNumber > 0 && DesktopNumber <= DesktopCount)
		{
			SendMessage(hWndDexpot, DEX_GETDESKTOPTITLE, DesktopNumber, (LPARAM) hWndMessageWindow);
		}
	}
}

void DexpotDesktopNameMeasure::OnDesktopConfigurationChanged()
{
	InitializeData();
}

void DexpotDesktopNameMeasure::OnDesktopCountChanged(int NewCount)
{
	InitializeData();
}

void DexpotDesktopNameMeasure::SetDesktopName(UINT Desktop, std::wstring &Name)
{
	if (--Desktop >= DesktopNames.size()) DesktopNames.resize(Desktop + 1);
	if (Desktop >= 0) DesktopNames[Desktop] = Name;
}


/*
 * DexpotDesktopWallpaperMeasure
 *
 */
DexpotDesktopWallpaperMeasure::DexpotDesktopWallpaperMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {}

UINT DexpotDesktopWallpaperMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	DesktopNumber = _ttoi(ReadConfigString(section, _T("VDDesktop"), _T("0")));
	return DexpotMeasure::Initialize(iniFile, section);
}

LPCTSTR DexpotDesktopWallpaperMeasure::GetString(UINT flags)
{
	if (DesktopNumber == 0)
	{
		SystemParametersInfo(SPI_GETDESKWALLPAPER, STRINGBUFFER_SIZE, StringBuffer, 0);
		return StringBuffer;
	}
	else if (DesktopNumber > 0 && (UINT) DesktopNumber <= DesktopWallpapers.size())
	{
		return DesktopWallpapers[DesktopNumber - 1].c_str();
	}

	StringBuffer[0] = _T('\0');
	return StringBuffer;
}

void DexpotDesktopWallpaperMeasure::InitializeData()
{
	if (PluginRegistered)
	{
		int DesktopCount = (int) SendMessage(hWndDexpot, DEX_GETDESKTOPCOUNT, 0, 0);
		DesktopWallpapers.resize(DesktopCount);
		if (DesktopNumber == 0)
		{
			for (int i = 1; i <= DesktopCount; i++)
			{
				SendMessage(hWndDexpot, DEX_GETDESKTOPWALLPAPER, i, (LPARAM) hWndMessageWindow);
			}
		}
		else if (DesktopNumber > 0 && DesktopNumber <= DesktopCount)
		{
			SendMessage(hWndDexpot, DEX_GETDESKTOPWALLPAPER, DesktopNumber, (LPARAM) hWndMessageWindow);
		}
	}
}
void DexpotDesktopWallpaperMeasure::OnSwitched(int FromDesktop, int ToDesktop, WORD Flags, WORD Trigger)
{
	SendMessage(hWndDexpot, DEX_GETDESKTOPWALLPAPER, FromDesktop, (LPARAM) hWndMessageWindow);
}

void DexpotDesktopWallpaperMeasure::OnDesktopConfigurationChanged()
{
	InitializeData();
}

void DexpotDesktopWallpaperMeasure::OnDesktopCountChanged(int NewCount)
{
	InitializeData();
}

void DexpotDesktopWallpaperMeasure::SetDesktopWallpaper(UINT Desktop, std::wstring &Wallpaper)
{
	if (--Desktop >= DesktopWallpapers.size()) DesktopWallpapers.resize(Desktop + 1);
	if (Desktop >= 0) DesktopWallpapers[Desktop] = Wallpaper;
}


/*
 * DexpotCommandMeasure
 *
 */
DexpotCommandMeasure::DexpotCommandMeasure(HMODULE instance, UINT id) : DexpotMeasure(instance, id) {}

void DexpotCommandMeasure::ExecuteBang(LPCTSTR args)
{
	if (PluginRegistered)
	{
		COPYDATASTRUCT cds;

		cds.dwData = DEX_DEXPOTCOMMAND;
		cds.lpData = (PVOID) args;
		cds.cbData = (DWORD) (_tcslen(args) + 1) * sizeof(TCHAR);

		SendMessage(hWndDexpot, WM_COPYDATA, (WPARAM) hWndMessageWindow, (LPARAM) &cds);
	}
}
