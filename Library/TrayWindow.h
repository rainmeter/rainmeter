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

#ifndef __TRAYWINDOW_H__
#define __TRAYWINDOW_H__

#include <windows.h>
#include <gdiplus.h>
#include <vector>

typedef HRESULT (WINAPI * FPLOADICONMETRIC)(HINSTANCE hinst, PCWSTR pszName, int lims, HICON* phico);

#define WM_TRAY_NOTIFYICON WM_USER + 101
#define TRAYICON_SIZE 16

enum TRAY_METER_TYPE
{
	TRAY_METER_TYPE_NONE,
	TRAY_METER_TYPE_HISTOGRAM,
	TRAY_METER_TYPE_BITMAP
};

class CConfigParser;
class CMeasure;

class CTrayWindow
{
public:
	CTrayWindow(HINSTANCE instance);
	~CTrayWindow();

	void ReadConfig(CConfigParser& parser);
	HWND GetWindow() { return m_Window; }
	bool IsTrayIconEnabled() { return m_TrayIconEnabled; }

	void ShowWelcomeNotification();
	void ShowUpdateNotification(const WCHAR* newVersion);

protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	enum TRAY_NOTIFICATION
	{
		TRAY_NOTIFICATION_NONE,
		TRAY_NOTIFICATION_WELCOME,
		TRAY_NOTIFICATION_UPDATE
	};

	void AddTrayIcon();
	void RemoveTrayIcon();
	void ModifyTrayIcon(double value);
	HICON CreateTrayIcon(double value);

	void ShowNotification(TRAY_NOTIFICATION id, const WCHAR* title, const WCHAR* text);

	HICON m_TrayIcon;
	HWND m_Window;
	HINSTANCE m_Instance;
	CMeasure* m_Measure;

	TRAY_METER_TYPE m_MeterType;
	Gdiplus::Color m_TrayColor1;
	Gdiplus::Color m_TrayColor2;
	Gdiplus::Bitmap* m_Bitmap;

	std::vector<HICON> m_TrayIcons;

	double m_TrayValues[TRAYICON_SIZE];
	int m_TrayPos;

	TRAY_NOTIFICATION m_Notification;

	bool m_TrayIconEnabled;
};

#endif
