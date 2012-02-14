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

class CDialog
{
public:
	HWND GetWindow() { return m_Window; }

	static HWND GetActiveDialog() { return c_ActiveDialog; }

protected:
	class CTab
	{
	public:
		HWND GetWindow() { return m_Window; }
		bool IsInitialized() { return m_Initialized; }
		void Activate();

		virtual void Initialize() {}
		virtual void Resize(int w, int h) {}

	protected:
		CTab(HINSTANCE instance, HWND owner, WORD tabId, DLGPROC tabProc);
		virtual ~CTab();

		HWND m_Window;
		bool m_Initialized;
	};

	CDialog(HWND wnd);
	virtual ~CDialog();

	INT_PTR OnActivate(WPARAM wParam, LPARAM lParam);

	void SetDialogRTL();
	void SetDialogFont();

	HWND m_Window;
	HFONT m_Font;
	HFONT m_FontBold;

private:
	static BOOL CALLBACK SetFontProc(HWND hWnd, LPARAM lParam);

	static HWND c_ActiveDialog;
};

#endif
