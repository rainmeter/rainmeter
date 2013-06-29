/*
  Copyright (C) 2011 Birunthan Mohanathas

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

#ifndef __DIALOGABOUT_H__
#define __DIALOGABOUT_H__

#include "../Common/Dialog.h"
#include "Logger.h"
#include "MeterWindow.h"

class DialogAbout : public Dialog
{
public:
	DialogAbout();
	virtual ~DialogAbout();

	static Dialog* GetDialog() { return c_Dialog; }

	static void Open(int tab = 0);
	static void Open(const WCHAR* name);
	static void ShowAboutLog();

	static void AddLogItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message);
	static void UpdateSkins();
	static void UpdateMeasures(MeterWindow* meterWindow);

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

private:
	// Log tab
	class TabLog : public Tab
	{
	public:
		enum Id
		{
			Id_ItemsListView = 100,
			Id_ErrorCheckBox,
			Id_WarningCheckBox,
			Id_NoticeCheckBox,
			Id_DebugCheckBox
		};

		TabLog();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

		void AddItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	private:
		bool m_Error;
		bool m_Warning;
		bool m_Notice;
		bool m_Debug;
	};

	// Measures tab
	class TabSkins : public Tab
	{
	public:
		enum Id
		{
			Id_SkinsListBox = 100,
			Id_ItemsListView
		};

		TabSkins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);
	
		void UpdateSkinList();
		void UpdateMeasureList(MeterWindow* meterWindow);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	private:
		static int CALLBACK ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

		MeterWindow* m_SkinWindow;
	};

	// Plugins tab
	class TabPlugins : public Tab
	{
	public:
		enum Id
		{
			Id_ItemsListView = 100
		};

		TabPlugins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

	private:
		typedef LPCTSTR (*GETPLUGINAUTHOR)();
		typedef UINT (*GETPLUGINVERSION)();
	};

	// Version tab
	class TabVersion : public Tab
	{
	public:
		enum Id
		{
			Id_AppIcon = 100,
			Id_VersionLabel,
			Id_HomeLink,
			Id_LicenseLink,
			Id_PathLabel,
			Id_IniFileLabel,
			Id_SkinPathLabel,
			Id_CopyButton
		};

		TabVersion();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	enum Id
	{
		Id_CloseButton = IDCLOSE,
		Id_Tab = 100
	};

	Tab& GetActiveTab();

	TabLog m_TabLog;
	TabSkins m_TabSkins;
	TabPlugins m_TabPlugins;
	TabVersion m_TabVersion;

	static WINDOWPLACEMENT c_WindowPlacement;
	static DialogAbout* c_Dialog;
};

#endif
