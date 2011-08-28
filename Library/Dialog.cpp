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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Dialog.h"

/*
** CDialog
**
** Constructor.
**
*/
CDialog::CDialog(HWND wnd) :
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
** CDialog
**
** Destructor.
**
*/
CDialog::~CDialog()
{
	DeleteObject(m_Font);
	DeleteObject(m_FontBold);
}

/*
** SetDialogFont
**
** Sets dialog font to UI font.
**
*/
void CDialog::SetDialogFont()
{
	EnumChildWindows(m_Window, SetFontProc, (WPARAM)m_Font);
}

/*
** SetFontProc
**
** Callback for EnumChildWindows().
**
*/
BOOL CALLBACK CDialog::SetFontProc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, 0);
	return TRUE;
}

/*
** OnColorDialog
**
** Paints dialog background.
**
*/
INT_PTR CDialog::OnColorDialog(WPARAM wParam, LPARAM lParam)
{
	return (INT_PTR)GetStockObject(WHITE_BRUSH);
}

/*
** OnColorStatic
**
** Paints control text and background.
**
*/
INT_PTR CDialog::OnColorStatic(WPARAM wParam, LPARAM lParam)
{
	HDC hdc = (HDC)wParam;
	SetTextColor(hdc, (COLORREF)GetSysColor(COLOR_WINDOWTEXT));
	SetBkMode(hdc, TRANSPARENT);

	return OnColorDialog(wParam, lParam);
}

/*
** CTab
**
** Constructor.
**
*/
CTab::CTab(HWND wnd) : CDialog(wnd),
	m_Initialized(false)
{
}

/*
** CTab
**
** Destructor.
**
*/
CTab::~CTab()
{
	DestroyWindow(m_Window);
}
