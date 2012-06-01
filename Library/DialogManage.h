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
	virtual CTab& GetActiveTab();

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

		static int PopulateTree(HWND tree, TVINSERTSTRUCT& tvi, int index = 0);

		std::wstring m_FileName;
		std::wstring m_SkinName;
		CMeterWindow* m_SkinWindow;
		bool m_HandleCommands;
		bool m_IgnoreUpdate;
	};

	// Themes tab
	class CTabThemes : public CTab
	{
	public:
		CTabThemes(HWND owner);

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

	CTabSkins m_TabSkins;
	CTabThemes m_TabThemes;
	CTabSettings m_TabSettings;
};

#endif

