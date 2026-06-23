/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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

	void CreateControls(const Control* cts, UINT ctCount, ControlTemplate::GetStringFunc getString);
	void RelayoutControls();

	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) { return FALSE; }

	HWND m_Window;
	UINT m_Dpi;
	ControlTemplate m_ControlTemplate;

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
		virtual void Create(HWND owner) = 0;
		void Activate();
		void UpdateDpi(UINT dpi) { m_Dpi = dpi; }
		RECT GetLayoutRect(UINT dpi);

		virtual void Initialize() {}
		virtual void Relayout(int w, int h) { RelayoutControls(); }
		virtual void HandleDpiChange() {}

	protected:
		Tab();
		virtual ~Tab();

		void CreateTabWindow(short x, short y, short w, short h, HWND parent);

		bool m_Initialized;
		RECT m_InitialMargin;
		UINT m_InitialDpi;
	};

	Dialog();
	virtual ~Dialog();

	void ShowDialogWindow(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless = true);

	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Relayout();
	virtual void HandleDpiChange() {}

	void AddTab(WORD controlId, Tab& tab, const WCHAR* text);
	void AddPage(Tab& tab);
	void SelectTab(int index);
	Tab& GetActiveTab();

	static void SetMenuButton(HWND button);

private:
	Dialog(const Dialog& r);
	void ActivateTab();

	static LRESULT CALLBACK MenuButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	INT_PTR HandleDpiChanged(WPARAM wParam, LPARAM lParam);

	static HWND c_ActiveDialogWindow;

	HWND m_TabControl;
	std::vector<Tab*> m_Pages;
	std::vector<Tab*> m_Tabs;
};

#endif
