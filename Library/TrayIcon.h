/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_TRAYICON_H_
#define RM_LIBRARY_TRAYICON_H_

#include <windows.h>
#include <ole2.h>  // For Gdiplus.h.
#include <gdiplus.h>
#include <vector>

#define WM_TRAY_NOTIFYICON WM_USER + 101
#define TRAYICON_SIZE 16

enum TRAY_METER_TYPE
{
	TRAY_METER_TYPE_NONE,
	TRAY_METER_TYPE_HISTOGRAM,
	TRAY_METER_TYPE_BITMAP
};

class ConfigParser;
class Measure;

class TrayIcon
{
public:
	TrayIcon();
	~TrayIcon();

	TrayIcon(const TrayIcon& other) = delete;
	TrayIcon& operator=(TrayIcon other) = delete;

	void Initialize();

	void ReadOptions(ConfigParser& parser);
	HWND GetWindow() { return m_Window; }
	bool IsTrayIconEnabled() { return m_IconEnabled; }
	void SetTrayIcon(bool enabled, bool setTemporarily = false);

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

	bool AddTrayIcon();
	bool IsTrayIconReady();
	void TryAddTrayIcon();
	void CheckTrayIcon();
	void RemoveTrayIcon();
	void ModifyTrayIcon(double value);
	HICON CreateTrayIcon(double value);

	void ShowNotification(TRAY_NOTIFICATION id, const WCHAR* title, const WCHAR* text);

	HICON m_Icon;
	HWND m_Window;
	Measure* m_Measure;

	TRAY_METER_TYPE m_MeterType;
	Gdiplus::Color m_Color1;
	Gdiplus::Color m_Color2;
	Gdiplus::Bitmap* m_Bitmap;

	std::vector<HICON> m_Icons;

	double m_Values[TRAYICON_SIZE];
	int m_Pos;

	TRAY_NOTIFICATION m_Notification;

	bool m_TrayContextMenuEnabled;

	bool m_IconEnabled;
};

#endif
