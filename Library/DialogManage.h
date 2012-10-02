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

#ifndef __DIALOGMANAGE_H__
#define __DIALOGMANAGE_H__

#include "Dialog.h"

class CDialogManage : public CDialog
{
public:
	CDialogManage(HWND window);
	virtual ~CDialogManage();

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

	static void Open(const WCHAR* name);
	static void Open(int tab = 0);
	static void OpenSkin(CMeterWindow* meterWindow);

	static void UpdateSkins(CMeterWindow* meterWindow, bool deleted = false);

	static WINDOWPLACEMENT c_WindowPlacement;
	static CDialogManage* c_Dialog;

protected:
	virtual HWND GetActiveWindow() { return GetActiveTab().GetWindow(); }

private:
	// Skins tab
	class CTabSkins : public CTab
	{
	public:
		CTabSkins(HWND owner);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

		void Update(CMeterWindow* meterWindow, bool deleted);

		static void SelectTreeItem(HWND tree, HTREEITEM item, LPCWSTR name);

	private:
		void SetControls();
		void DisableControls(bool clear = false);
		void ReadSkin();

		static std::wstring GetTreeSelectionPath(HWND tree);
		static int PopulateTree(HWND tree, TVINSERTSTRUCT& tvi, int index = 0);

		std::wstring m_SkinFileName;
		std::wstring m_SkinFolderPath;
		CMeterWindow* m_SkinWindow;
		bool m_HandleCommands;
		bool m_IgnoreUpdate;
	};

	// Layouts tab
	class CTabLayouts : public CTab
	{
	public:
		CTabLayouts(HWND owner);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};
	
	// Settings tab
	class CTabSettings : public CTab
	{
	public:
		CTabSettings(HWND owner);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	CTab& GetActiveTab();

	CTabSkins m_TabSkins;
	CTabLayouts m_TabLayouts;
	CTabSettings m_TabSettings;
};

#endif

