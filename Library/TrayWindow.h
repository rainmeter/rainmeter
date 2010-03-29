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

#ifndef __TRAYWINDOW_H__
#define __TRAYWINDOW_H__

#pragma warning(disable: 4786)

#include <windows.h>
#include "Measure.h"

#define WM_DELAYED_REFRESH_ALL WM_APP + 0

#define WM_NOTIFYICON WM_USER + 101
#define TRAYICON_SIZE 16

enum TRAY_METER_TYPE
{
	TRAY_METER_TYPE_NONE,
	TRAY_METER_TYPE_HISTOGRAM,
	TRAY_METER_TYPE_BITMAP
};

class CTrayWindow
{
public:
	CTrayWindow(HINSTANCE instance);
	~CTrayWindow();

	void ReadConfig(CConfigParser& parser);
	HWND GetWindow() { return m_Window; }
	BOOL ShowBalloonHelp();

protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	BOOL AddTrayIcon();
	BOOL RemoveTrayIcon();
	BOOL ModifyTrayIcon(double value);
	HICON CreateTrayIcon(double value);

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
};

#endif
