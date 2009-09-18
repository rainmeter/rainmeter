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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define _WIN32_IE 0x0600

#pragma warning(disable: 4996)

#include "TrayWindow.h"
#include "Resource.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "AboutDialog.h"
#include "Error.h"
#include <io.h>
#include <ShellAPI.h>

#define TRAYTIMER 3

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

CTrayWindow::CTrayWindow(HINSTANCE instance)
{
	WNDCLASS  wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); 
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = L"RainmeterTrayClass";

	RegisterClass(&wc);
	
	m_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		L"RainmeterTrayClass",
		NULL,
		WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		this);

	m_Instance = instance;
	m_Measure = NULL;
	m_TrayIcon = NULL;

	m_MeterType = TRAY_METER_TYPE_HISTOGRAM;
	m_TrayColor1 = Color(0, 100, 0);
	m_TrayColor2 = Color(0, 255, 0);

	m_Bitmap = NULL;

#ifndef _WIN64
	SetWindowLong(m_Window, GWL_USERDATA, magicDWord);
#endif

	m_TrayPos = 0;
	memset(m_TrayValues, 0, sizeof(double) * TRAYICON_SIZE);
}

CTrayWindow::~CTrayWindow()
{
	KillTimer(m_Window, TRAYTIMER);
	RemoveTrayIcon();

	if (m_Bitmap) delete m_Bitmap;
	if (m_Measure) delete m_Measure;

	for (size_t i = 0; i < m_TrayIcons.size(); i++) 
	{
		DestroyIcon(m_TrayIcons[i]);
	}
	m_TrayIcons.clear();

	if (m_Window) DestroyWindow(m_Window);
}

BOOL CTrayWindow::AddTrayIcon() 
{ 
    BOOL res = FALSE; 
    NOTIFYICONDATA tnid; 
	
	if (m_TrayIcon)
	{
		DestroyIcon(m_TrayIcon);
		m_TrayIcon = NULL;
	}

	m_TrayIcon = CreateTrayIcon(0);

	if (m_TrayIcon)
	{
		tnid.cbSize = sizeof(NOTIFYICONDATA); 
		tnid.hWnd = m_Window; 
		tnid.uID = IDI_TRAY;
		tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
		tnid.uCallbackMessage = WM_NOTIFYICON; 
		tnid.hIcon = m_TrayIcon;
		lstrcpyn(tnid.szTip, L"Rainmeter", sizeof(tnid.szTip)); 
		
		res = Shell_NotifyIcon(NIM_ADD, &tnid); 
	}
    return res; 
}

BOOL CTrayWindow::RemoveTrayIcon() 
{ 
	BOOL res = FALSE; 
	
	if (m_TrayIcon)
	{
		NOTIFYICONDATA tnid; 
		
		tnid.cbSize = sizeof(NOTIFYICONDATA); 
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
    NOTIFYICONDATA tnid; 

	if (m_TrayIcon)
	{
		DestroyIcon(m_TrayIcon);
		m_TrayIcon = NULL;
	}

	m_TrayIcon = CreateTrayIcon(value);
	
	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = m_Window; 
	tnid.uID = IDI_TRAY;
	tnid.uFlags = NIF_ICON; 
	tnid.hIcon = m_TrayIcon;

    res = Shell_NotifyIcon(NIM_MODIFY, &tnid); 
    return res; 
}


BOOL CTrayWindow::ShowBalloonHelp()
{
    BOOL res = FALSE; 

	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof(NOTIFYICONDATA));

	nid.hWnd = m_Window; 
	nid.uID = IDI_TRAY;
	nid.cbSize=sizeof(NOTIFYICONDATA);
	nid.uFlags = NIF_INFO;
	nid.uTimeout = 30000;
	nid.dwInfoFlags = 4; // NIIF_USER;
	nid.hIcon = LoadIcon(m_Instance, MAKEINTRESOURCE(IDI_TRAY));
	wcscpy(nid.szInfo, L"There aren't any configs active at the moment. Open the context menu from Rainmeter's tray icon and select a config you want to use.");
	wcscpy(nid.szInfoTitle, L"Rainmeter");
	res = Shell_NotifyIcon(NIM_MODIFY, &nid);

	return res;
}

HICON CTrayWindow::CreateTrayIcon(double value)
{
	if (m_Measure == NULL)
	{
		return LoadIcon(m_Instance, MAKEINTRESOURCE(IDI_TRAY));
	}
	else
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

			for (int i = 0; i < TRAYICON_SIZE; i++)
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
		else if (m_MeterType == TRAY_METER_TYPE_BITMAP && (m_Bitmap || m_TrayIcons.size() > 0)) 
		{
			if (m_TrayIcons.size() > 0) 
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

	return NULL;
}

void CTrayWindow::ReadConfig(CConfigParser& parser)
{
	for (size_t i = 0; i < m_TrayIcons.size(); i++) 
	{
		DestroyIcon(m_TrayIcons[i]);
	}
	m_TrayIcons.clear();

	std::wstring measureName = parser.ReadString(L"TrayMeasure", L"Measure", L"");

	CConfigParser* oldParser = Rainmeter->GetCurrentParser();
	Rainmeter->SetCurrentParser(&parser);
	if (!measureName.empty())
	{
		m_Measure = CMeasure::Create(measureName.c_str(), NULL);
		m_Measure->SetName(L"TrayMeasure");
		m_Measure->ReadConfig(parser, L"TrayMeasure");
	}
	Rainmeter->SetCurrentParser(oldParser);

	m_MeterType = TRAY_METER_TYPE_NONE;

	std::wstring type = parser.ReadString(L"TrayMeasure", L"TrayMeter", L"HISTOGRAM");
	if (wcsicmp(type.c_str(), L"HISTOGRAM") == 0) 
	{
		m_MeterType = TRAY_METER_TYPE_HISTOGRAM;
		m_TrayColor1 = parser.ReadColor(L"TrayMeasure", L"TrayColor1", Color(0, 100, 0));
		m_TrayColor2 = parser.ReadColor(L"TrayMeasure", L"TrayColor2", Color(0, 255, 0));
	}
	else if (wcsicmp(type.c_str(), L"BITMAP") == 0) 
	{
		m_MeterType = TRAY_METER_TYPE_BITMAP;

		std::wstring imageName = parser.ReadString(L"TrayMeasure", L"TrayBitmap", L"");

		// Load the bitmaps if defined
		if (!imageName.empty())
		{
			imageName = Rainmeter->GetSkinPath() + imageName;
			if (imageName.size() > 3) 
			{
				std::wstring extension = imageName.substr(imageName.size() - 3);
				if (extension == L"ico" || extension == L"ICO") 
				{
					int count = 1;
					HICON hIcon = NULL;

					// Load the icons
					do 
					{
						WCHAR buffer[MAX_PATH];
						wsprintf(buffer, imageName.c_str(), count++);

						hIcon = (HICON)LoadImage(NULL, buffer, IMAGE_ICON, TRAYICON_SIZE, TRAYICON_SIZE, LR_LOADFROMFILE);
						if (hIcon) m_TrayIcons.push_back(hIcon);
						if (imageName == buffer) break;
					} while(hIcon != NULL);
				}
			}

			if (m_TrayIcons.size() == 0) 
			{
				// No icons found so load as bitmap
				if (m_Bitmap) 
				{
					delete m_Bitmap;
				}
				m_Bitmap = new Bitmap(imageName.c_str());
				Status status = m_Bitmap->GetLastStatus();
				if(Ok != status)
				{
					DebugLog(L"Bitmap image not found:  %s", imageName.c_str());
					delete m_Bitmap;
					m_Bitmap = NULL;
				}
			}
		}
	}
	else
	{
		DebugLog(L"No such TrayMeter: %s", type.c_str());
	}

	int trayIcon = parser.ReadInt(L"Rainmeter", L"TrayIcon", 1);
	if (trayIcon != 0)
	{
		AddTrayIcon();
		SetTimer(m_Window, TRAYTIMER, 1000, NULL);		// Update the tray once per sec
	}
}


LRESULT CALLBACK CTrayWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CTrayWindow* tray = NULL;

	if(uMsg == WM_CREATE) 
	{
		tray=(CTrayWindow*)((LPCREATESTRUCT)lParam)->lpCreateParams;
	}

	switch(uMsg) 
	{
	case WM_COMMAND:
		if (Rainmeter && tray)
		{
			if(wParam == ID_CONTEXT_ABOUT)
			{
				OpenAboutDialog(tray->GetWindow(), Rainmeter->GetInstance());
			} 
			else if(wParam == ID_CONTEXT_SHOW_HELP)
			{
				std::wstring help = Rainmeter->GetPath();
				help = L"\"" + help + L"Rainmeter.chm\"";
				LSExecute(NULL, help.c_str(), SW_SHOWNORMAL);
			}
			else if(wParam == ID_CONTEXT_REFRESH)
			{
				// Read skins and settings
				Rainmeter->ReloadSettings();

				// Refresh all
				RainmeterRefresh(tray->GetWindow(), NULL);
			} 
			else if(wParam == ID_CONTEXT_SHOWLOGFILE)
			{
				// Check if the file exists
				std::wstring log = Rainmeter->GetLogFile();
				if (_waccess(log.c_str(), 0) != -1)
				{
					std::wstring command = L"Notepad.exe ";
					command += log;
					LSExecute(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
				}
				else
				{
					int res = MessageBox(tray->GetWindow(), L"The log file doesn't exist.\nDo you want to create it?", L"Rainmeter", MB_YESNO | MB_ICONQUESTION);
					if (res == IDYES)
					{
						HANDLE file = CreateFile(log.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
						if (file != INVALID_HANDLE_VALUE)
						{
							CloseHandle(file);
							ResetLoggingFlag();	// Re-enable logging

							std::wstring message;
							message = L"Created file: ";
							message += log;
							message += L"\nDelete it to disable Rainmeter's logging";
							MessageBox(tray->GetWindow(), message.c_str(), L"Rainmeter", MB_OK | MB_ICONINFORMATION);

							std::wstring command = L"Notepad.exe ";
							command += log;
							LSExecute(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
						}
						else
						{
							std::wstring message;
							message = L"Unable to create the file: ";
							message += log;
							MessageBox(tray->GetWindow(), message.c_str(), L"Rainmeter", MB_OK | MB_ICONERROR);
						}
					}
				}
			}
			else if(wParam == ID_CONTEXT_EDITCONFIG)
			{
				std::wstring command = Rainmeter->GetConfigEditor();
				command += L" \"";
				command += Rainmeter->GetIniFile();
				command += L"\"";
				LSExecute(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
			}
			else if(wParam == ID_CONTEXT_MANAGETHEMES)
			{
				std::wstring command = L"\"" + Rainmeter->GetPath();
				command += L"\\Addons\\RainThemes\\RainThemes.exe\"";
				LSExecute(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
			}
			else if(wParam == ID_CONTEXT_QUIT)
			{
				if (Rainmeter->GetDummyLitestep()) PostQuitMessage(0);
				quitModule(Rainmeter->GetInstance());
			}
			else if(wParam == ID_CONTEXT_OPENSKINSFOLDER)
			{
				std::wstring command;
				command += L"\"";
				command += Rainmeter->GetSkinPath();
				command += L"\"";
				LSExecute(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
			}
			else if((wParam & 0x0ffff) >= ID_THEME_FIRST)
			{
				int pos = (wParam & 0x0ffff) - ID_THEME_FIRST;

				const std::vector<std::wstring>& themes = Rainmeter->GetAllThemes();
				if (pos >= 0 && pos < (int)themes.size())
				{
					std::wstring command = L"\"" + Rainmeter->GetPath();
					command += L"\\Addons\\RainThemes\\RainThemes.exe\" /load \"";
					command += themes[pos];
					command += L"\"";
					LSExecute(tray->GetWindow(), command.c_str(), SW_SHOWNORMAL);
				}
			}
			else if((wParam & 0x0ffff) >= ID_CONFIG_FIRST)
			{
				wParam = wParam & 0x0ffff;

				// Check which config was selected
				const std::vector<CRainmeter::CONFIG>& configs = Rainmeter->GetAllConfigs();

				for (size_t i = 0; i < configs.size(); i++)
				{
					for (size_t j = 0; j < configs[i].commands.size(); j++)
					{
						if (configs[i].commands[j] == wParam)
						{
							if (configs[i].active == j + 1)
							{
								CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(configs[i].config);
								Rainmeter->DeactivateConfig(meterWindow, i);
							}
							else
							{
								if (configs[i].active != 0)
								{
									CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(configs[i].config);
									if (meterWindow)
									{
										Rainmeter->DeactivateConfig(meterWindow, i);
									}
								}
								Rainmeter->ActivateConfig(i, j);
							}
							return 0;
						}
					}
				}
			}
			else
			{
				// Forward the message to correct window
				int index = (int)(wParam >> 16);
				std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();

				if (index < (int)windows.size())
				{
					std::map<std::wstring, CMeterWindow*>::iterator iter = windows.begin();
					for( ; iter != windows.end(); iter++)
					{
						index--;
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

	case WM_NOTIFYICON:
		{
			UINT uMouseMsg = (UINT)lParam; 

			std::wstring bang;

			switch(uMouseMsg) 
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

			if (!bang.empty())
			{
				std::wstring arg;
				size_t pos = bang.find(L' ');
				if (pos != std::wstring::npos) 
				{
					arg = bang;
					arg.erase(0, pos + 1);
					bang = bang.substr(0, pos);
				}
				Rainmeter->ExecuteBang(bang, arg, NULL);
			}
			else if	(uMouseMsg == WM_RBUTTONDOWN)
			{
				POINT point;
				GetCursorPos(&point);
				SetForegroundWindow(tray->m_Window);
				Rainmeter->ShowContextMenu(point, NULL);
				SetForegroundWindow(tray->m_Window);
			}
		}
		break;

	case WM_TIMER:
		if (tray && tray->m_Measure)
		{
			tray->m_Measure->Update();
			tray->ModifyTrayIcon(tray->m_Measure->GetRelativeValue());
		}
		break;

	case WM_DESTROY:
		if (Rainmeter->GetDummyLitestep()) PostQuitMessage(0);
		break;

	case LM_GETREVID:
		if(lParam != NULL)
		{
			char* Buffer=(char*)lParam;
			if(wParam==0) 
			{
				sprintf(Buffer, "Rainmeter.dll: %s", APPVERSION);
			} 
			else if(wParam==1) 
			{
				sprintf(Buffer, "Rainmeter.dll: %s %s, Rainy", APPVERSION, __DATE__);
			} 
			else
			{
				Buffer[0] = 0;
			}

			return strlen(Buffer);
		}
		return 0;

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
