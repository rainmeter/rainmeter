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
#include "resource.h"
#include "../Version.h"

#define RAINMETER_OFFICIAL		L"http://rainmeter.net/cms/"
#define RAINMETER_HELP			L"http://rainmeter.net/cms/Support"

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
	m_Icon(),
	m_Measure(),
	m_MeterType(TRAY_METER_TYPE_HISTOGRAM),
	m_Color1(0, 100, 0),
	m_Color2(0, 255, 0),
	m_Bitmap(),
	m_Values(),
	m_Pos(),
	m_Notification(TRAY_NOTIFICATION_NONE),
	m_IconEnabled(true)
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

	for (size_t i = 0, isize = m_Icons.size(); i < isize; ++i)
	{
		DestroyIcon(m_Icons[i]);
	}
	m_Icons.clear();

	if (m_Window) DestroyWindow(m_Window);
}

void CTrayWindow::AddTrayIcon()
{
	if (m_Icon)
	{
		DestroyIcon(m_Icon);
		m_Icon = NULL;
	}

	m_Icon = CreateTrayIcon(0);

	if (m_Icon)
	{
		NOTIFYICONDATA tnid = {sizeof(NOTIFYICONDATA)};
		tnid.hWnd = m_Window;
		tnid.uID = IDI_TRAY;
		tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		tnid.uCallbackMessage = WM_TRAY_NOTIFYICON;
		tnid.hIcon = m_Icon;
		wcsncpy_s(tnid.szTip, L"Rainmeter", _TRUNCATE);

		Shell_NotifyIcon(NIM_ADD, &tnid);
	}
}

void CTrayWindow::RemoveTrayIcon()
{
	if (m_Icon)
	{
		NOTIFYICONDATA tnid = {sizeof(NOTIFYICONDATA)};
		tnid.hWnd = m_Window;
		tnid.uID = IDI_TRAY;
		tnid.uFlags = 0;

		Shell_NotifyIcon(NIM_DELETE, &tnid);

		DestroyIcon(m_Icon);
		m_Icon = NULL;
	}
}

void CTrayWindow::ModifyTrayIcon(double value)
{
	if (m_Icon)
	{
		DestroyIcon(m_Icon);
		m_Icon = NULL;
	}

	m_Icon = CreateTrayIcon(value);

	NOTIFYICONDATA tnid = {sizeof(NOTIFYICONDATA)};
	tnid.hWnd = m_Window;
	tnid.uID = IDI_TRAY;
	tnid.uFlags = NIF_ICON;
	tnid.hIcon = m_Icon;

	Shell_NotifyIcon(NIM_MODIFY, &tnid);
}

HICON CTrayWindow::CreateTrayIcon(double value)
{
	if (m_Measure != NULL)
	{
		if (m_MeterType == TRAY_METER_TYPE_HISTOGRAM)
		{
			m_Values[m_Pos] = value;
			m_Pos = (m_Pos + 1) % TRAYICON_SIZE;

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
				points[i + 1].Y = (int)(TRAYICON_SIZE * (1.0 - m_Values[(m_Pos + i) % TRAYICON_SIZE]));
			}

			SolidBrush brush(m_Color1);
			graphics.FillRectangle(&brush, 0, 0, TRAYICON_SIZE, TRAYICON_SIZE);

			SolidBrush brush2(m_Color2);
			graphics.FillPolygon(&brush2, points, TRAYICON_SIZE + 2);

			HICON icon;
			trayBitmap.GetHICON(&icon);
			return icon;
		}
		else if (m_MeterType == TRAY_METER_TYPE_BITMAP && (m_Bitmap || !m_Icons.empty()))
		{
			if (!m_Icons.empty())
			{
				size_t frame = 0;
				size_t frameCount = m_Icons.size();

				// Select the correct frame linearly
				frame = (size_t)(value * frameCount);
				frame = min((frameCount - 1), frame);

				return CopyIcon(m_Icons[frame]);
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
			HRESULT hr = loadIconMetric(hExe, MAKEINTRESOURCE(IDI_TRAY), LIM_SMALL, &icon);
			if (SUCCEEDED(hr))
			{
				return icon;
			}
		}
	}

	return (HICON)LoadImage(hExe, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
}

void CTrayWindow::ShowNotification(TRAY_NOTIFICATION id, const WCHAR* title, const WCHAR* text)
{
	if (m_Notification == TRAY_NOTIFICATION_NONE)
	{
		m_Notification = id;

		NOTIFYICONDATA nid = {sizeof(NOTIFYICONDATA)};
		nid.hWnd = m_Window;
		nid.uID = IDI_TRAY;
		nid.uFlags = NIF_INFO;
		nid.uTimeout = 30000;
		nid.dwInfoFlags = NIIF_USER;
		nid.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_RAINMETER), IMAGE_ICON, 32, 32, LR_SHARED);
		wcsncpy_s(nid.szInfoTitle, title, _TRUNCATE);
		wcsncpy_s(nid.szInfo, text, _TRUNCATE);
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	}
}

void CTrayWindow::ShowWelcomeNotification()
{
	ShowNotification(TRAY_NOTIFICATION_WELCOME, GetString(ID_STR_WELCOME), GetString(ID_STR_CLICKTOMANAGE));
}

void CTrayWindow::ShowUpdateNotification(const WCHAR* newVersion)
{
	std::wstring text = GetFormattedString(ID_STR_CLICKTODOWNLOAD, newVersion);
	ShowNotification(TRAY_NOTIFICATION_UPDATE, GetString(ID_STR_UPDATEAVAILABLE), text.c_str());
}

void CTrayWindow::ReadConfig(CConfigParser& parser)
{
	// Clear old Settings
	KillTimer(m_Window, TIMER_TRAY);

	delete m_Measure;
	m_Measure = NULL;

	delete m_Bitmap;
	m_Bitmap = NULL;

	std::vector<HICON>::const_iterator iter = m_Icons.begin();
	for ( ; iter != m_Icons.end(); ++iter)
	{
		DestroyIcon((*iter));
	}
	m_Icons.clear();

	m_MeterType = TRAY_METER_TYPE_NONE;

	// Read tray settings
	m_IconEnabled = 0!=parser.ReadInt(L"Rainmeter", L"TrayIcon", 1);
	if (m_IconEnabled)
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
			m_Color1 = parser.ReadColor(L"TrayMeasure", L"TrayColor1", Color::MakeARGB(255, 0, 100, 0));
			m_Color2 = parser.ReadColor(L"TrayMeasure", L"TrayColor2", Color::MakeARGB(255, 0, 255, 0));
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
						if (hIcon) m_Icons.push_back(hIcon);
						if (wcscmp(imagePath, buffer) == 0) break;
					}
					while(hIcon != NULL);
				}

				if (m_Icons.empty())
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
			if (wParam == IDM_MANAGE)
			{
				CDialogManage::Open();
			}
			else if (wParam == IDM_ABOUT)
			{
				CDialogAbout::Open();
			}
			else if (wParam == IDM_SHOW_HELP)
			{
				RunCommand(NULL, RAINMETER_HELP, SW_SHOWNORMAL);
			}
			else if (wParam == IDM_NEW_VERSION)
			{
				RunCommand(NULL, RAINMETER_OFFICIAL, SW_SHOWNORMAL);
			}
			else if (wParam == IDM_REFRESH)
			{
				PostMessage(Rainmeter->GetWindow(), WM_RAINMETER_DELAYED_REFRESH_ALL, (WPARAM)NULL, (LPARAM)NULL);
			}
			else if (wParam == IDM_SHOWLOGFILE)
			{
				Rainmeter->ShowLogFile();
			}
			else if (wParam == IDM_STARTLOG)
			{
				Rainmeter->StartLogging();
			}
			else if (wParam == IDM_STOPLOG)
			{
				Rainmeter->StopLogging();
			}
			else if (wParam == IDM_DELETELOGFILE)
			{
				Rainmeter->DeleteLogFile();
			}
			else if (wParam == IDM_DEBUGLOG)
			{
				Rainmeter->SetDebug(!Rainmeter->GetDebug());
			}
			else if (wParam == IDM_DISABLEDRAG)
			{
				Rainmeter->SetDisableDragging(!Rainmeter->GetDisableDragging());
			}
			else if (wParam == IDM_EDITCONFIG)
			{
				Rainmeter->EditSettings();
			}
			else if (wParam == IDM_QUIT)
			{
				PostQuitMessage(0);
			}
			else if (wParam == IDM_OPENSKINSFOLDER)
			{
				Rainmeter->OpenSkinFolder();
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
			case WM_MBUTTONDOWN:
				bang = Rainmeter->GetTrayExecuteM();
				break;

			case WM_RBUTTONDOWN:
				bang = Rainmeter->GetTrayExecuteR();
				break;

			case WM_MBUTTONDBLCLK:
				bang = Rainmeter->GetTrayExecuteDM();
				break;

			case WM_RBUTTONDBLCLK:
				bang = Rainmeter->GetTrayExecuteDR();
				break;
			}

			if (!IsCtrlKeyDown() &&   // Ctrl is pressed, so only run default action
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
			else if (uMouseMsg == NIN_BALLOONUSERCLICK)
			{
				if (tray->m_Notification == TRAY_NOTIFICATION_WELCOME)
				{
					CDialogManage::Open();
				}
				else if (tray->m_Notification == TRAY_NOTIFICATION_UPDATE)
				{
					RunCommand(NULL, RAINMETER_OFFICIAL, SW_SHOWNORMAL);
				}

				tray->m_Notification = TRAY_NOTIFICATION_NONE;
			}
			else if (uMouseMsg == NIN_BALLOONHIDE || uMouseMsg == NIN_BALLOONTIMEOUT)
			{
				tray->m_Notification = TRAY_NOTIFICATION_NONE;
			}
		}
		break;

	case WM_QUERY_RAINMETER:
		if (IsWindow((HWND)lParam))
		{
			auto sendCopyData = [&](const std::wstring& data)
			{
				COPYDATASTRUCT cds;
				cds.dwData = wParam;
				cds.cbData = (DWORD)((data.length() + 1) * sizeof(WCHAR));
				cds.lpData = (PVOID)data.c_str();
				SendMessage((HWND)lParam, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);
			};

			if (wParam == RAINMETER_QUERY_ID_SKINS_PATH)
			{
				sendCopyData(Rainmeter->GetSkinPath());
				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_SETTINGS_PATH)
			{
				sendCopyData(Rainmeter->GetSettingsPath());
				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_PLUGINS_PATH)
			{
				sendCopyData(Rainmeter->GetPluginPath());
				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_PROGRAM_PATH)
			{
				sendCopyData(Rainmeter->GetPath());
				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_LOG_PATH)
			{
				sendCopyData(Rainmeter->GetLogFile());
				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_CONFIG_EDITOR)
			{
				sendCopyData(Rainmeter->GetConfigEditor());
				return 0;
			}
			else if (wParam == RAINMETER_QUERY_ID_IS_DEBUGGING)
			{
				BOOL debug = Rainmeter->GetDebug();
				SendMessage((HWND)lParam, WM_QUERY_RAINMETER_RETURN, (WPARAM)hWnd, (LPARAM)debug);
				return 0;
			}
		}
		return 1;

	case WM_COPYDATA:
		{
			COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
			if (cds->dwData == RAINMETER_QUERY_ID_SKIN_WINDOWHANDLE)
			{
				LPCWSTR configName = (LPCWSTR)cds->lpData;
				CMeterWindow* mw = Rainmeter->GetMeterWindow(configName);
				return (mw) ? (LRESULT)mw->GetWindow() : NULL;
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
