/*
  Copyright (C) 2012 Birunthan Mohanathas

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

#ifndef RM_COMMON_DIALOG_H_
#define RM_COMMON_DIALOG_H_

#include <Windows.h>
#include "ControlTemplate.h"

// Shared base class for Dialog and Tab.
class BaseDialog
{
public:
	HWND GetControl(WORD id) { return GetDlgItem(m_Window, id); }

protected:
	BaseDialog();
	virtual ~BaseDialog() {}

	void Show(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless);

	void CreateControls(const ControlTemplate::Control* cts, UINT ctCount, HFONT font, ControlTemplate::GetStringFunc getString);

	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) { return FALSE; }

	HWND m_Window;

private:
	BaseDialog(const BaseDialog& r);

	static INT_PTR CALLBACK InitialDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class Dialog : public BaseDialog
{
public:
	HWND GetWindow() { return m_Window; }

	static bool HandleMessage(MSG& msg);

protected:
	class Tab : public BaseDialog
	{
	public:
		HWND GetWindow() { return m_Window; }
		bool IsInitialized() { return m_Initialized; }
		void Activate();

		virtual void Initialize() {}
		virtual void Resize(int w, int h) {}

	protected:
		Tab();
		virtual ~Tab();

		void CreateTabWindow(short x, short y, short w, short h, HWND owner);

		bool m_Initialized;
	};

	Dialog();
	virtual ~Dialog();

	void ShowDialogWindow(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless = true);

	INT_PTR OnActivate(WPARAM wParam, LPARAM lParam);

	static void SetMenuButton(HWND button);

	HFONT m_Font;
	HFONT m_FontBold;

private:
	Dialog(const Dialog& r);

	static LRESULT CALLBACK MenuButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	static HWND c_ActiveDialogWindow;
};

#endif
