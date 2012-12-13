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

class CDialogAbout : public CDialog
{
public:
	CDialogAbout();
	virtual ~CDialogAbout();

	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

	static void Open(int tab = 0);
	static void Open(const WCHAR* name);
	static void ShowAboutLog();

	static void AddLogItem(int level, LPCWSTR time, LPCWSTR message);
	static void UpdateSkins();
	static void UpdateMeasures(CMeterWindow* meterWindow);

	static WINDOWPLACEMENT c_WindowPlacement;
	static CDialogAbout* c_Dialog;

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	// Log tab
	class CTabLog : public CTab
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

		CTabLog();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

		void AddItem(int level, LPCWSTR time, LPCWSTR message);

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
	class CTabSkins : public CTab
	{
	public:
		enum Id
		{
			Id_SkinsListBox = 100,
			Id_ItemsListView
		};

		CTabSkins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);
	
		void UpdateSkinList();
		void UpdateMeasureList(CMeterWindow* meterWindow);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	private:
		static int CALLBACK ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

		CMeterWindow* m_SkinWindow;
	};

	// Plugins tab
	class CTabPlugins : public CTab
	{
	public:
		enum Id
		{
			Id_ItemsListView = 100
		};

		CTabPlugins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

	private:
		typedef LPCTSTR (*GETPLUGINAUTHOR)();
		typedef UINT (*GETPLUGINVERSION)();
	};

	// Version tab
	class CTabVersion : public CTab
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

		CTabVersion();

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

	CTab& GetActiveTab();

	CTabLog m_TabLog;
	CTabSkins m_TabSkins;
	CTabPlugins m_TabPlugins;
	CTabVersion m_TabVersion;
};

#endif
