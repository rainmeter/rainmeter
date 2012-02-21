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

HWND CDialog::c_ActiveDialogWindow = NULL;
HWND CDialog::c_ActiveTabWindow = NULL;

/*
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
** Destructor.
**
*/
CDialog::~CDialog()
{
	DeleteObject(m_Font);
	DeleteObject(m_FontBold);
}

INT_PTR CDialog::OnActivate(WPARAM wParam, LPARAM lParam)
{
	if (wParam)
	{
		c_ActiveDialogWindow = m_Window;
		c_ActiveTabWindow = GetActiveTab().GetWindow();
	}
	else
	{
		c_ActiveDialogWindow = c_ActiveTabWindow = NULL;
	}

	return FALSE;
}

void CDialog::SetDialogRTL()
{
	SetWindowLong(m_Window, GWL_EXSTYLE, GetWindowLong(m_Window, GWL_EXSTYLE) | WS_EX_LAYOUTRTL);
}

/*
** Sets dialog font to UI font.
**
*/
void CDialog::SetDialogFont()
{
	EnumChildWindows(m_Window, SetFontProc, (WPARAM)m_Font);
}

/*
** Callback for EnumChildWindows().
**
*/
BOOL CALLBACK CDialog::SetFontProc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, 0);
	return TRUE;
}

/*
** Constructor.
**
*/
CDialog::CTab::CTab(HINSTANCE instance, HWND owner, WORD tabId, DLGPROC tabProc) :
	m_Window(CreateDialog(instance, MAKEINTRESOURCE(tabId), owner, tabProc)),
	m_Initialized(false)
{
	EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);
}

/*
** Destructor.
**
*/
CDialog::CTab::~CTab()
{
	DestroyWindow(m_Window);
}

/*
** Activates the tab.
**
*/
void CDialog::CTab::Activate()
{
	c_ActiveTabWindow = m_Window;

	if (!m_Initialized)
	{
		Initialize();
	}

	EnableWindow(m_Window, TRUE);
	BringWindowToTop(m_Window);
}
