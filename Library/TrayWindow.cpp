/*
  Copyright (C) 2004 Kimmo Pekkola

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

#include "StdAfx.h"
#include "TrayWindow.h"
#include "Measure.h"
#include "resource.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "DialogAbout.h"
#include "DialogManage.h"
#include "Error.h"
#include "RainmeterQuery.h"
#include "../Version.h"

#define RAINMETER_OFFICIAL		L"http://rainmeter.net/cms/"
#define RAINMETER_MANUAL		L"http://rainmeter.net/cms/Manual"
#define RAINMETER_MANUALBETA	L"http://rainmeter.net/cms/Manual_beta"

#define ZPOS_FLAGS	(SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)

enum TIMER
{
	TIMER_TRAY = 3
};
enum INTERVAL
{
	INTERVAL_TRAY = 1000
};

const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

CTrayWindow::CTrayWindow(HINSTANCE instance) : m_Instance(instance),
	m_TrayIcon(),
	m_Measure(),
	m_MeterType(TRAY_METER_TYPE_HISTOGRAM),
	m_TrayColor1(0, 100, 0),
	m_TrayColor2(0, 255, 0),
	m_Bitmap(),
	m_TrayValues(),
	m_TrayPos(),
	m_TrayIconEnabled(true)
{
	WNDCLASS wc = {0};
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.hInstance = instance;
	wc.lpszClassName = L"RainmeterTrayClass";
	wc.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_RAINMETER), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED);

	RegisterClass(&wc);

	m_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		L"RainmeterTrayClass",
		NULL,
		WS_POPUP | WS_DISABLED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		this);

	SetWindowPos(m_Window, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
}

CTrayWindow::~CTrayWindow()
{
	KillTimer(m_Window, TIMER_TRAY);
	RemoveTrayIcon();

	delete m_Bitmap;
	delete m_Measure;

	for (size_t i = 0, isize = m_TrayIcons.size(); i < isize; ++i)
	{
		DestroyIcon(m_TrayIcons[i]);
	}
	m_TrayIcons.clear();

	if (m_Window) DestroyWindow(m_Window);
}

BOOL CTrayWindow::AddTrayIcon()
{
	BOOL res = FALSE;

	if (m_TrayIcon)
	{
		DestroyIcon(m_TrayIcon);
		m_TrayIcon = NULL;
	}

	m_TrayIcon = CreateTrayIcon(0);

	if (m_TrayIcon)
	{
		NOTIFYICONDATA tnid = {sizeof(NOTIFYICONDATA)};
		tnid.hWnd = m_Window;
		tnid.uID = IDI_TRAY;
		tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		tnid.uCallbackMessage = WM_TRAY_NOTIFYICON;
		tnid.hIcon = m_TrayIcon;
		wcsncpy_s(tnid.szTip, L"Rainmeter", _TRUNCATE);

		res = Shell_NotifyIcon(NIM_ADD, &tnid);
	}
	return res;
}

BOOL CTrayWindow::RemoveTrayIcon()
{
	BOOL res = FALSE;

	if (m_TrayIcon)
	{
		NOTIFYICONDATA tnid = {sizeof(NOTIFYICONDATA)};
		tnid.hWnd = m_Window;
		tnid.uID = IDI_TRAY;
		tnid.uFlags = 0;

		res = Shell_NotifyIcon(NIM_DELETE, &tnid);

		DestroyIcon(m_TrayIcon);
		m_TrayIcon = NULL;
	}

	return res;
}

BOOL CTrayWindow::ModifyTrayIcon(double value)
{
	BOOL res = FALSE;

	if (m_TrayIcon)
	{
		DestroyIcon(m_TrayIcon);
		m_TrayIcon = NULL;
	}

	m_TrayIcon = CreateTrayIcon(value);

	NOTIFYICONDATA tnid = {sizeof(NOTIFYICONDATA)};
	tnid.hWnd = m_Window;
	tnid.uID = IDI_TRAY;
	tnid.uFlags = NIF_ICON;
	tnid.hIcon = m_TrayIcon;

	res = Shell_NotifyIcon(NIM_MODIFY, &tnid);
	return res;
}

HICON CTrayWindow::CreateTrayIcon(double value)
{
	if (m_Measure != NULL)
	{
		if (m_MeterType == TRAY_METER_TYPE_HISTOGRAM)
		{
			m_TrayValues[m_TrayPos] = value;
			m_TrayPos = (m_TrayPos + 1) % TRAYICON_SIZE;

			Bitmap trayBitmap(TRAYICON_SIZE, TRAYICON_SIZE);
			Graphics graphics(&trayBitmap);
			graphics.SetSmoothingMode(SmoothingModeAntiAlias);

			Point points[TRAYICON_SIZE + 2];
			points[0].X = 0;
			points[0].Y = TRAYICON_SIZE;
			points[TRAYICON_SIZE + 1].X = TRAYICON_SIZE - 1;
			points[TRAYICON_SIZE + 1].Y = TRAYICON_SIZE;

			for (int i = 0; i < TRAYICON_SIZE; ++i)
			{
				points[i + 1].X = i;
				points[i + 1].Y = (int)(TRAYICON_SIZE * (1.0 - m_TrayValues[(m_TrayPos + i) % TRAYICON_SIZE]));
			}

			SolidBrush brush(m_TrayColor1);
			graphics.FillRectangle(&brush, 0, 0, TRAYICON_SIZE, TRAYICON_SIZE);

			SolidBrush brush2(m_TrayColor2);
			graphics.FillPolygon(&brush2, points, TRAYICON_SIZE + 2);

			HICON icon;
			trayBitmap.GetHICON(&icon);
			return icon;
		}
		else if (m_MeterType == TRAY_METER_TYPE_BITMAP && (m_Bitmap || !m_TrayIcons.empty()))
		{
			if (!m_TrayIcons.empty())
			{
				size_t frame = 0;
				size_t frameCount = m_TrayIcons.size();

				// Select the correct frame linearly
				frame = (size_t)(value * frameCount);
				frame = min((frameCount - 1), frame);

				return CopyIcon(m_TrayIcons[frame]);
			}
			else
			{
				int frame = 0;
				int frameCount = 0;
				int newX, newY;

				if (m_Bitmap->GetWidth() > m_Bitmap->GetHeight())
				{
					frameCount = m_Bitmap->GetWidth() / TRAYICON_SIZE;
				}
				else
				{
					frameCount = m_Bitmap->GetHeight() / TRAYICON_SIZE;
				}

				// Select the correct frame linearly
				frame = (int)(value * frameCount);
				frame = min((frameCount - 1), frame);

				if (m_Bitmap->GetWidth() > m_Bitmap->GetHeight())
				{
					newX = frame * TRAYICON_SIZE;
					newY = 0;
				}
				else
				{
					newX = 0;
					newY = frame * TRAYICON_SIZE;
				}

				Bitmap trayBitmap(TRAYICON_SIZE, TRAYICON_SIZE);
				Graphics graphics(&trayBitmap);
				graphics.SetSmoothingMode(SmoothingModeAntiAlias);

				// Blit the image
				Rect r(0, 0, TRAYICON_SIZE, TRAYICON_SIZE);
				graphics.DrawImage(m_Bitmap, r, newX, newY, TRAYICON_SIZE, TRAYICON_SIZE, UnitPixel);

				HICON icon;
				trayBitmap.GetHICON(&icon);
				return icon;
			}
		}
	}

	// Return the default icon if there is no valid measure
	HINSTANCE hExe = GetModuleHandle(NULL);
	HINSTANCE hComctl = GetModuleHandle(L"Comctl32");
	if (hComctl)
	{
		// Try LoadIconMetric for better quality with high DPI
		FPLOADICONMETRIC loadIconMetric = (FPLOADICONMETRIC)GetProcAddress(hComctl, "LoadIconMetric");
		if (loadIconMetric)
		{
			HICON icon;
			HRESULT hr = loadIconMetric(hExe, MAKEINTRESOURCE(IDI_RAINMETER), LIM_SMALL, &icon);
			if (SUCCEEDED(hr))
			{
				return icon;
			}
		}
	}

	return (HICON)LoadImage(hExe, MAKEINTRESOURCE(IDI_RAINMETER), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
}

void CTrayWindow::ReadConfig(CConfigParser& parser)
{
	// Clear old Settings
	KillTimer(m_Window, TIMER_TRAY);

	delete m_Measure;
	m_Measure = NULL;

	delete m_Bitmap;
	m_Bitmap = NULL;

	std::vector<HICON>::const_iterator iter = m_TrayIcons.begin();
	for ( ; iter != m_TrayIcons.end(); ++iter)
	{
		DestroyIcon((*iter));
	}
	m_TrayIcons.clear();

	m_MeterType = TRAY_METER_TYPE_NONE;

	// Read tray settings
	m_TrayIconEnabled = 0!=parser.ReadInt(L"Rainmeter", L"TrayIcon", 1);
	if (m_TrayIconEnabled)
	{
		const std::wstring& measureName = parser.ReadString(L"TrayMeasure", L"Measure", L"");

		if (!measureName.empty())
		{
			CConfigParser* oldParser = Rainmeter->GetCurrentParser();
			Rainmeter->SetCurrentParser(&parser);

			try
			{
				m_Measure = CMeasure::Create(measureName.c_str(), NULL, L"TrayMeasure");
				if (m_Measure)
				{
					m_Measure->ReadConfig(parser);
				}
			}
			catch (CError& error)
			{
				delete m_Measure;
				m_Measure = NULL;
				LogError(error);
			}

			Rainmeter->SetCurrentParser(oldParser);
		}

		const WCHAR* type = parser.ReadString(L"TrayMeasure", L"TrayMeter", m_Measure ? L"HISTOGRAM" : L"NONE").c_str();
		if (_wcsicmp(type, L"NONE") == 0)
		{
			// Use main icon
		}
		else if (_wcsicmp(type, L"HISTOGRAM") == 0)
		{
			m_MeterType = TRAY_METER_TYPE_HISTOGRAM;
			m_TrayColor1 = parser.ReadColor(L"TrayMeasure", L"TrayColor1", Color::MakeARGB(255, 0, 100, 0));
			m_TrayColor2 = parser.ReadColor(L"TrayMeasure", L"TrayColor2", Color::MakeARGB(255, 0, 255, 0));
		}
		else if (_wcsicmp(type, L"BITMAP") == 0)
		{
			m_MeterType = TRAY_METER_TYPE_BITMAP;

			std::wstring imageName = parser.ReadString(L"TrayMeasure", L"TrayBitmap", L"");

			// Load the bitmaps if defined
			if (!imageName.empty())
			{
				imageName.insert(0, Rainmeter->GetSkinPath());
				const WCHAR* imagePath = imageName.c_str();
				if (_wcsicmp(imagePath + (imageName.size() - 4), L".ico") == 0)
				{
					int count = 1;
					HICON hIcon = NULL;

					// Load the icons
					do
					{
						WCHAR buffer[MAX_PATH];
						_snwprintf_s(buffer, _TRUNCATE, imagePath, count++);

						hIcon = (HICON)LoadImage(NULL, buffer, IMAGE_ICON, TRAYICON_SIZE, TRAYICON_SIZE, LR_LOADFROMFILE);
						if (hIcon) m_TrayIcons.push_back(hIcon);
						if (wcscmp(imagePath, buffer) == 0) break;
					}
					while(hIcon != NULL);
				}

				if (m_TrayIcons.empty())
				{
					// No icons found so load as bitmap
					delete m_Bitmap;
					m_Bitmap = new Bitmap(imagePath);
					Status status = m_Bitmap->GetLastStatus();
					if (Ok != status)
					{
						delete m_Bitmap;
						m_Bitmap = NULL;
						LogWithArgs(LOG_WARNING, L"Bitmap image not found: %s", imagePath);
					}
				}
			}
		}
		else
		{
			LogWithArgs(LOG_ERROR, L"No such TrayMeter: %s", type);
		}

		AddTrayIcon();

		if (m_Measure)
		{
			SetTimer(m_Window, TIMER_TRAY, INTERVAL_TRAY, NULL);		// Update the tray once per sec
		}
	}
	else
	{
		RemoveTrayIcon();
	}
}


LRESULT CALLBACK CTrayWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CTrayWindow* tray = Rainmeter->GetTrayWindow();

	switch (uMsg)
	{
	case WM_COMMAND:
		if (tray)
		{
			if (wParam == ID_CONTEXT_MANAGE)
			{
				CDialogManage::Open();
			}
			else if (wParam == ID_CONTEXT_ABOUT)
			{
				CDialogAbout::Open();
			}
			else if (wParam == ID_CONTEXT_SHOW_HELP)
			{
				RunCommand(NULL, revision_beta ? RAINMETER_MANUALBETA : RAINMETER_MANUAL, SW_SHOWNORMAL);
			}
			else if (wParam == ID_CONTEXT_NEW_VERSION)
			{
				RunCommand(NULL, RAINMETER_OFFICIAL, SW_SHOWNORMAL);
			}
			else if (wParam == ID_CONTEXT_REFRESH)
			{
				PostMessage(tray->GetWindow(), WM_TRAY_DELAYED_REFRESH_ALL, (WPARAM)NULL, (LPARAM)NULL);
			}
			else if (wParam == ID_CONTEXT_SHOWLOGFILE)
			{
				// Check if the file exists
				const std::wstring& log = Rainmeter->GetLogFile();
				if (_waccess(log.c_str(), 0) != -1)
				{
					std::wstring command = Rainmeter->GetLogViewer() + log;
					RunCommand(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
				}
			}
			else if (wParam == ID_CONTEXT_STARTLOG)
			{
				Rainmeter->StartLogging();
			}
			else if (wParam == ID_CONTEXT_STOPLOG)
			{
				Rainmeter->StopLogging();
			}
			else if (wParam == ID_CONTEXT_DELETELOGFILE)
			{
				Rainmeter->DeleteLogFile();
			}
			else if (wParam == ID_CONTEXT_DEBUGLOG)
			{
				Rainmeter->SetDebug(!Rainmeter->GetDebug());
			}
			else if (wParam == ID_CONTEXT_DISABLEDRAG)
			{
				Rainmeter->SetDisableDragging(!Rainmeter->GetDisableDragging());
			}
			else if (wParam == ID_CONTEXT_EDITCONFIG)
			{
				std::wstring command = Rainmeter->GetConfigEditor() + L" \"";
				command += Rainmeter->GetIniFile();
				command += L'"';
				RunCommand(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
			}
			else if (wParam == ID_CONTEXT_QUIT)
			{
				PostQuitMessage(0);
				Quit();
			}
			else if (wParam == ID_CONTEXT_OPENSKINSFOLDER)
			{
				std::wstring command = L'"' + Rainmeter->GetSkinPath();
				command += L'"';
				RunCommand(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
			}
			else if ((wParam & 0x0ffff) >= ID_THEME_FIRST && (wParam & 0x0ffff) <= ID_THEME_LAST)
			{
				int pos = (wParam & 0x0ffff) - ID_THEME_FIRST;

				const std::vector<std::wstring>& themes = Rainmeter->GetAllThemes();
				if (pos >= 0 && pos < (int)themes.size())
				{
					Rainmeter->LoadTheme(themes[pos]);
				}
			}
			else if ((wParam & 0x0ffff) >= ID_CONFIG_FIRST && (wParam & 0x0ffff) <= ID_CONFIG_LAST)
			{
				std::pair<int, int> indexes = Rainmeter->GetMeterWindowIndex((UINT)(wParam & 0x0ffff));
				if (indexes.first != -1 && indexes.second != -1)
				{
					Rainmeter->ToggleConfig(indexes.first, indexes.second);
				}
			}
			else
			{
				// Forward the message to correct window
				int index = (int)(wParam >> 16);
				const std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();

				if (index < (int)windows.size())
				{
					std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
					for ( ; iter != windows.end(); ++iter)
					{
						--index;
						if (index < 0)
						{
							CMeterWindow* meterWindow = (*iter).second;
							SendMessage(meterWindow->GetWindow(), WM_COMMAND, wParam & 0x0FFFF, NULL);
							break;
						}
					}
				}
			}
		}
		return 0;	// Don't send WM_COMMANDS any further

	case WM_TRAY_NOTIFYICON:
		{
			UINT uMouseMsg = (UINT)lParam;
			std::wstring bang;

			switch (uMouseMsg)
			{
			case WM_LBUTTONDOWN:
				bang = Rainmeter->GetTrayExecuteL();
				break;

			case WM_MBUTTONDOWN:
				bang = Rainmeter->GetTrayExecuteM();
				break;

			case WM_RBUTTONDOWN:
				bang = Rainmeter->GetTrayExecuteR();
				break;

			case WM_LBUTTONDBLCLK:
				bang = Rainmeter->GetTrayExecuteDL();
				break;

			case WM_MBUTTONDBLCLK:
				bang = Rainmeter->GetTrayExecuteDM();
				break;

			case WM_RBUTTONDBLCLK:
				bang = Rainmeter->GetTrayExecuteDR();
				break;
			}

			if (!(GetKeyState(VK_CONTROL) < 0) &&   // Ctrl is pressed, so only run default action
				!bang.empty())
			{
				Rainmeter->ExecuteCommand(bang.c_str(), NULL);
			}
			else if (uMouseMsg == WM_RBUTTONDOWN)
			{
				POINT point;
				GetCursorPos(&point);
				Rainmeter->ShowContextMenu(point, NULL);
			}
			else if (uMouseMsg == WM_LBUTTONDOWN || uMouseMsg == WM_LBUTTONDBLCLK)
			{
				CDialogManage::Open();
			}
		}
		break;

	case WM_QUERY_RAINMETER:
		if (IsWindow((HWND)lParam))
		{
			COPYDATASTRUCT cds;

			if (wParam == RAINMETER_QUERY_ID_SKINS_PATH)
			{
				const std::wstring& path = Rainmeter->GetSkinPath();

				cds.dwData = RAINMETER_QUERY_ID_SKINS_PATH;
				cds.cbData = (DWORD)((path.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) path.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_SETTINGS_PATH)
			{
				std::wstring path = Rainmeter->GetSettingsPath();

				cds.dwData = RAINMETER_QUERY_ID_SETTINGS_PATH;
				cds.cbData = (DWORD)((path.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) path.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_PLUGINS_PATH)
			{
				const std::wstring& path = Rainmeter->GetPluginPath();

				cds.dwData = RAINMETER_QUERY_ID_PLUGINS_PATH;
				cds.cbData = (DWORD)((path.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) path.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_PROGRAM_PATH)
			{
				const std::wstring& path = Rainmeter->GetPath();

				cds.dwData = RAINMETER_QUERY_ID_PROGRAM_PATH;
				cds.cbData = (DWORD)((path.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) path.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_LOG_PATH)
			{
				const std::wstring& path = Rainmeter->GetLogFile();

				cds.dwData = RAINMETER_QUERY_ID_LOG_PATH;
				cds.cbData = (DWORD)((path.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) path.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_CONFIG_EDITOR)
			{
				const std::wstring& editor = Rainmeter->GetConfigEditor();

				cds.dwData = RAINMETER_QUERY_ID_CONFIG_EDITOR;
				cds.cbData = (DWORD)((editor.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) editor.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_VERSION_CHECK)
			{
				UINT versioncheck = ((int)Rainmeter->GetDisableVersionCheck() * ((int)Rainmeter->GetDisableVersionCheck() + (int)Rainmeter->GetNewVersion()));

				SendMessage((HWND)lParam, WM_QUERY_RAINMETER_RETURN, (WPARAM)hWnd, (LPARAM)versioncheck);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_IS_DEBUGGING)
			{
				BOOL debug = Rainmeter->GetDebug();

				SendMessage((HWND)lParam, WM_QUERY_RAINMETER_RETURN, (WPARAM)hWnd, (LPARAM)debug);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_STATS_DATE)
			{
				const std::wstring& date = Rainmeter->GetStatsDate();

				cds.dwData = RAINMETER_QUERY_ID_STATS_DATE;
				cds.cbData = (DWORD)((date.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) date.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_TRAY_EX_L)
			{
				const std::wstring& tray = Rainmeter->GetTrayExecuteL();

				cds.dwData = RAINMETER_QUERY_ID_TRAY_EX_L;
				cds.cbData = (DWORD)((tray.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) tray.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_TRAY_EX_R)
			{
				const std::wstring& tray = Rainmeter->GetTrayExecuteR();

				cds.dwData = RAINMETER_QUERY_ID_TRAY_EX_R;
				cds.cbData = (DWORD)((tray.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) tray.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_TRAY_EX_M)
			{
				const std::wstring& tray = Rainmeter->GetTrayExecuteM();

				cds.dwData = RAINMETER_QUERY_ID_TRAY_EX_M;
				cds.cbData = (DWORD)((tray.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) tray.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_TRAY_EX_DL)
			{
				const std::wstring& tray = Rainmeter->GetTrayExecuteDL();

				cds.dwData = RAINMETER_QUERY_ID_TRAY_EX_DL;
				cds.cbData = (DWORD)((tray.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) tray.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_TRAY_EX_DR)
			{
				const std::wstring& tray = Rainmeter->GetTrayExecuteDR();

				cds.dwData = RAINMETER_QUERY_ID_TRAY_EX_DR;
				cds.cbData = (DWORD)((tray.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) tray.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_TRAY_EX_DM)
			{
				const std::wstring& tray = Rainmeter->GetTrayExecuteDM();

				cds.dwData = RAINMETER_QUERY_ID_TRAY_EX_DM;
				cds.cbData = (DWORD)((tray.size() + 1) * sizeof(wchar_t));
				cds.lpData = (LPVOID) tray.c_str();

				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);

				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_IS_LITESTEP)
			{
				SendMessage((HWND)lParam, WM_QUERY_RAINMETER_RETURN, (WPARAM)hWnd, (LPARAM)0);

				return 0;
			}
		}
		return 1;

	case WM_COPYDATA:
		{
			COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
			if (cds->dwData == RAINMETER_QUERY_ID_SKIN_WINDOWHANDLE)
			{
				std::wstring SkinName((LPTSTR)cds->lpData);
				std::map<std::wstring, CMeterWindow*> MeterWindows = Rainmeter->GetAllMeterWindows();
				std::map<std::wstring, CMeterWindow*>::const_iterator iter = MeterWindows.find(SkinName);
				if (iter != MeterWindows.end())
				{
					return (LRESULT)iter->second->GetWindow();
				}
				return NULL;
			}
		}
		return 1;

	case WM_TIMER:
		if (tray && tray->m_Measure)
		{
			tray->m_Measure->Update();
			tray->ModifyTrayIcon(tray->m_Measure->GetRelativeValue());
		}
		break;

	case WM_TRAY_DELAYED_REFRESH_ALL:
		Rainmeter->RefreshAll();
		return 0;

	case WM_TRAY_DELAYED_EXECUTE:
		if (lParam)
		{
			// Execute bang
			WCHAR* bang = (WCHAR*)lParam;
			Rainmeter->ExecuteCommand(bang, NULL);
			free(bang);  // _wcsdup()
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		if (uMsg == WM_TASKBARCREATED)
		{
			if (tray && tray->IsTrayIconEnabled())
			{
				tray->RemoveTrayIcon();
				tray->AddTrayIcon();
			}
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
