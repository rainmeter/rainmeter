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

#include "Dialog.h"
#include <Commctrl.h>
#include <Uxtheme.h>

HWND Dialog::c_ActiveDialogWindow = nullptr;

//
// BaseDialog
//

BaseDialog::BaseDialog() :
	m_Window()
{
}

/*
** Create (if not already) and show the dialog.
**
*/
void BaseDialog::Show(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless)
{
	if (m_Window)
	{
		// Show existing window.
		ShowWindow(m_Window, SW_SHOW);
		SetForegroundWindow(m_Window);
		return;
	}

	const WCHAR* font = L"MS Shell Dlg 2";
	const size_t titleSize = (wcslen(title) + 1) * sizeof(WCHAR);
	const size_t fontSize = (wcslen(font) + 1) * sizeof(WCHAR);

	const size_t dataSize = sizeof(DLGTEMPLATE) +
		sizeof(WCHAR) + // Menu array.
		sizeof(WCHAR) + // Class array.
		titleSize + // Title array.
		sizeof(WORD) + // Font point size.
		fontSize; // Font array.

	DLGTEMPLATE* dt = (DLGTEMPLATE*)new BYTE[dataSize];
	dt->style = style | DS_SHELLFONT | WS_VISIBLE;
	dt->dwExtendedStyle = exStyle;
	dt->cdit = 0;
	dt->x = x;
	dt->y = y;
	dt->cx = w;
	dt->cy = h;

	BYTE* dtExtra = (BYTE*)dt + sizeof(DLGTEMPLATE);

	// Menu array.
	*(WCHAR*)dtExtra = L'\0';
	dtExtra += sizeof(WCHAR);

	// Class array.
	*(WCHAR*)dtExtra = L'\0';
	dtExtra += sizeof(WCHAR);

	// Title array.
	memcpy(dtExtra, title, titleSize);
	dtExtra += titleSize;

	// Font point size.
	*(WORD*)dtExtra = 8;
	dtExtra += sizeof(WORD);

	// Font array.
	memcpy(dtExtra, font, fontSize);

	if (modeless)
	{
		CreateDialogIndirectParam(nullptr, dt, parent, InitialDlgProc, (LPARAM)this);
	}
	else
	{
		DialogBoxIndirectParam(nullptr, dt, parent, InitialDlgProc, (LPARAM)this);
	}

	delete [] dt;
}

void BaseDialog::CreateControls(const ControlTemplate::Control* cts, UINT ctCount, HFONT font, ControlTemplate::GetStringFunc getString)
{
	ControlTemplate::CreateControls(cts, ctCount, m_Window, font, getString);
}

INT_PTR CALLBACK BaseDialog::InitialDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		BaseDialog* dialog = (BaseDialog*)lParam;
		dialog->m_Window = hWnd;
		SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dialog);
		SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)MainDlgProc);
		return dialog->HandleMessage(uMsg, wParam, lParam);
	}

	return FALSE;
}

INT_PTR CALLBACK BaseDialog::MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BaseDialog* dialog = (BaseDialog*)GetWindowLongPtr(hWnd, DWLP_USER);
	return dialog->HandleMessage(uMsg, wParam, lParam);
}

//
// Dialog
//

Dialog::Dialog() : BaseDialog(),
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

Dialog::~Dialog()
{
	DestroyWindow(m_Window);
	DeleteObject(m_Font);
	DeleteObject(m_FontBold);
}

void Dialog::ShowDialogWindow(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless)
{
	Show(title, x, y, w, h, style, exStyle, parent, modeless);
}

INT_PTR Dialog::OnActivate(WPARAM wParam, LPARAM lParam)
{
	c_ActiveDialogWindow = wParam ? m_Window : nullptr;
	return FALSE;
}

bool Dialog::HandleMessage(MSG& msg)
{
	if (c_ActiveDialogWindow)
	{
		if (IsDialogMessage(c_ActiveDialogWindow, &msg))
		{
			return true;
		}
	}

	return false;
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

//
// Tab
//

Dialog::Tab::Tab() : BaseDialog(),
	m_Initialized(false)
{
}

void Dialog::Tab::CreateTabWindow(short x, short y, short w, short h, HWND owner)
{
	const DWORD style = DS_CONTROL | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
	const DWORD exStyle = WS_EX_CONTROLPARENT;
	Show(L"", x, y, w, h, style, exStyle, owner, true);

	EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);
}

Dialog::Tab::~Tab()
{
	DestroyWindow(m_Window);
}

void Dialog::Tab::Activate()
{
	if (!m_Initialized)
	{
		Initialize();
	}

	EnableWindow(m_Window, TRUE);
	BringWindowToTop(m_Window);
}
