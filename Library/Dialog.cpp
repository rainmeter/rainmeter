/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Dialog.h"

HWND OldDialog::c_ActiveDialogWindow = nullptr;
HWND OldDialog::c_ActiveTabWindow = nullptr;

OldDialog::OldDialog(HWND wnd) :
	m_Window(wnd),
	m_Font(),
	m_FontBold()
{
	NONCLIENTMETRICS ncm = { 0 };
	ncm.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(ncm.iPaddedBorderWidth);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
	m_Font = CreateFontIndirect(&ncm.lfMenuFont);
	ncm.lfMenuFont.lfWeight = FW_BOLD;
	ncm.lfMenuFont.lfHeight -= 2L;
	m_FontBold = CreateFontIndirect(&ncm.lfMenuFont);
}

/*
** Destructor.
**
*/
OldDialog::~OldDialog()
{
	DeleteObject(m_Font);
	DeleteObject(m_FontBold);
}

INT_PTR OldDialog::OnActivate(WPARAM wParam, LPARAM lParam)
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
void OldDialog::SetDialogFont(HWND window)
{
	EnumChildWindows(window, SetFontProc, (WPARAM)m_Font);
}

BOOL CALLBACK OldDialog::SetFontProc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, 0);
	return TRUE;
}

/*
** Subclass button control to draw arrow on the right.
**
*/
void OldDialog::SetMenuButton(HWND button)
{
	SetWindowSubclass(button, MenuButtonProc, 0, 0);
}

LRESULT CALLBACK OldDialog::MenuButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_PAINT:
		{
			// Draw arrow on top of the button
			RECT buttonRect = { 0 };
			GetClientRect(hWnd, &buttonRect);
			LONG arrowX = buttonRect.right - 18L;
			LONG arrowY = buttonRect.top + 4L;
			RECT arrowRect = { arrowX, arrowY, arrowX + 14L, arrowY + 14L };

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

OldDialog::Tab::Tab(HINSTANCE instance, HWND owner, WORD tabId, DLGPROC tabProc) :
	m_Window(CreateDialog(instance, MAKEINTRESOURCE(tabId), owner, tabProc)),
	m_Initialized(false)
{
	EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);
}

/*
** Destructor.
**
*/
OldDialog::Tab::~Tab()
{
	DestroyWindow(m_Window);
}

/*
** Activates the tab.
**
*/
void OldDialog::Tab::Activate()
{
	c_ActiveTabWindow = m_Window;

	if (!m_Initialized)
	{
		Initialize();
	}

	EnableWindow(m_Window, TRUE);
	BringWindowToTop(m_Window);
}
