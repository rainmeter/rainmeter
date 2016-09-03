/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __DIALOG_H__
#define __DIALOG_H__

class OldDialog
{
public:
	HWND GetWindow() { return m_Window; }

	static HWND GetActiveDialogWindow() { return c_ActiveDialogWindow; }
	static HWND GetActiveTabWindow() { return c_ActiveTabWindow; }

protected:
	class Tab
	{
	public:
		HWND GetWindow() { return m_Window; }
		bool IsInitialized() { return m_Initialized; }
		void Activate();

		virtual void Initialize() {}
		virtual void Resize(int w, int h) {}

	protected:
		Tab(HINSTANCE instance, HWND owner, WORD tabId, DLGPROC tabProc);
		virtual ~Tab();

		HWND m_Window;
		bool m_Initialized;
	};

	OldDialog(HWND wnd);
	virtual ~OldDialog();

	virtual HWND GetActiveWindow() { return m_Window; }

	INT_PTR OnActivate(WPARAM wParam, LPARAM lParam);

	void SetDialogFont(HWND window);
	void SetDialogFont() { SetDialogFont(m_Window); }

	static void SetMenuButton(HWND button);

	HWND m_Window;
	HFONT m_Font;
	HFONT m_FontBold;

private:
	static BOOL CALLBACK SetFontProc(HWND hWnd, LPARAM lParam);
	static LRESULT CALLBACK MenuButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	
	static HWND c_ActiveDialogWindow;
	static HWND c_ActiveTabWindow;
};

#endif
