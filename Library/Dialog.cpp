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

#include "StdAfx.h"
#include "Dialog.h"

HWND Dialog::c_ActiveDialogWindow = nullptr;
HWND Dialog::c_ActiveTabWindow = nullptr;

/*
** Constructor.
**
*/
Dialog::Dialog(HWND wnd) :
	m_Window(wnd),
	m_Font(),
	m_FontBold()
{
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(ncm.iPaddedBorderWidth);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
	m_Font = CreateFontIndirect(&ncm.lfMenuFont);
	ncm.lfMenuFont.lfWeight = FW_BOLD;
	ncm.lfMenuFont.lfHeight -= 2;
	m_FontBold = CreateFontIndirect(&ncm.lfMenuFont);
}

/*
** Destructor.
**
*/
Dialog::~Dialog()
{
	DeleteObject(m_Font);
	DeleteObject(m_FontBold);
}

INT_PTR Dialog::OnActivate(WPARAM wParam, LPARAM lParam)
{
	if (wParam)
	{
		c_ActiveDialogWindow = m_Window;
		c_ActiveTabWindow = GetActiveWindow();
	}
	else
	{
		c_ActiveDialogWindow = c_ActiveTabWindow = nullptr;
	}

	return FALSE;
}

/*
** Sets dialog font to UI font.
**
*/
void Dialog::SetDialogFont(HWND window)
{
	EnumChildWindows(window, SetFontProc, (WPARAM)m_Font);
}

BOOL CALLBACK Dialog::SetFontProc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, 0);
	return TRUE;
}

/*
** Subclass button control to draw arrow on the right.
**
*/
void Dialog::SetMenuButton(HWND button)
{
	SetWindowSubclass(button, MenuButtonProc, 0, 0);
}

LRESULT CALLBACK Dialog::MenuButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_PAINT:
		{
			// Draw arrow on top of the button
			RECT buttonRect;
			GetClientRect(hWnd, &buttonRect);
			int arrowX = buttonRect.right - 18;
			int arrowY = buttonRect.top + 4;
			RECT arrowRect = { arrowX, arrowY, arrowX + 14, arrowY + 14 };

			HDC dc = GetDC(hWnd);
			const WORD DFCS_MENUARROWDOWN = 0x0010;	// Undocumented
			DWORD drawFlags = DFCS_TRANSPARENT | DFCS_MENUARROWDOWN | (IsWindowEnabled(hWnd) ? 0 : DFCS_INACTIVE);
			DrawFrameControl(dc, &arrowRect, DFC_MENU, drawFlags);
			ReleaseDC(hWnd, dc);
		}
		break;

	case WM_GETTEXT:
		{
			// Append 3 spaces to the button text to move text to the left so
			// that it looks better with BS_CENTER.
			WCHAR* str = (WCHAR*)lParam + result;
			str[0] = str[1] = str[2] = L' ';
			str[3] = '\0';
			result += 3;
		}
		break;

	case WM_GETTEXTLENGTH:
		result += 3;
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, MenuButtonProc, uIdSubclass);
		break;
	}

	return result;
}

/*
** Constructor.
**
*/
Dialog::Tab::Tab(HINSTANCE instance, HWND owner, WORD tabId, DLGPROC tabProc) :
	m_Window(CreateDialog(instance, MAKEINTRESOURCE(tabId), owner, tabProc)),
	m_Initialized(false)
{
	EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);
}

/*
** Destructor.
**
*/
Dialog::Tab::~Tab()
{
	DestroyWindow(m_Window);
}

/*
** Activates the tab.
**
*/
void Dialog::Tab::Activate()
{
	c_ActiveTabWindow = m_Window;

	if (!m_Initialized)
	{
		Initialize();
	}

	EnableWindow(m_Window, TRUE);
	BringWindowToTop(m_Window);
}
