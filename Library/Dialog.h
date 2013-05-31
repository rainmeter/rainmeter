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

#ifndef __DIALOG_H__
#define __DIALOG_H__

class Dialog
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

	Dialog(HWND wnd);
	virtual ~Dialog();

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
