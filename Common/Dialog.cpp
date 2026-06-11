/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Dialog.h"

namespace {

UINT GetWindowDpi(HWND window)
{
	typedef UINT (WINAPI* GetDpiForWindowProc)(HWND);
	static auto s_GetDpiForWindow = (GetDpiForWindowProc)GetProcAddress(GetModuleHandle(L"user32"), "GetDpiForWindow");
	if (s_GetDpiForWindow)
	{
		const UINT dpi = s_GetDpiForWindow(window);
		if (dpi) return dpi;
	}

	HDC dc = GetDC(window);
	const UINT dpi = dc ? (UINT)GetDeviceCaps(dc, LOGPIXELSX) : 96U;
	if (dc) ReleaseDC(window, dc);
	return dpi ? dpi : 96U;
}

}  // namespace

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
	dt = nullptr;
}

void BaseDialog::CreateControls(const Control* cts, UINT ctCount, ControlTemplate::GetStringFunc getString)
{
	m_ControlTemplate.Initialize(cts, ctCount, m_Window, GetWindowDpi(m_Window), getString);
}

void BaseDialog::RelayoutControls()
{
	m_ControlTemplate.Relayout(GetWindowDpi(m_Window));
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
	m_TabControl()
{
}

Dialog::~Dialog()
{
	if (m_Window) DestroyWindow(m_Window);
}

void Dialog::ShowDialogWindow(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless)
{
	Show(title, x, y, w, h, style, exStyle, parent, modeless);
}

INT_PTR Dialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			// Automatic dialog scaling on Windows 10 Creators Update and later does not work for us
			// because it only works with dialogs that have immediate children. We have tabs so it will
			// not work for us.
			typedef BOOL (WINAPI* SetDialogDpiChangeBehaviorProc)(HWND, DIALOG_DPI_CHANGE_BEHAVIORS, DIALOG_DPI_CHANGE_BEHAVIORS);
			static auto s_SetDialogDpiChangeBehavior = (SetDialogDpiChangeBehaviorProc)GetProcAddress(GetModuleHandle(L"user32"), "SetDialogDpiChangeBehavior");
			if (s_SetDialogDpiChangeBehavior)
			{
				s_SetDialogDpiChangeBehavior(m_Window, DDC_DISABLE_ALL, DDC_DISABLE_ALL);
			}

			m_Dpi = GetWindowDpi(m_Window);
		}
		break;

	case WM_DPICHANGED:
		return HandleDpiChanged(wParam, lParam);

	case WM_NOTIFY:
		{
			LPNMHDR nm = (LPNMHDR)lParam;
			if (nm->hwndFrom == m_TabControl && nm->code == TCN_SELCHANGE)
			{
				ActivateTab();
				return TRUE;
			}
		}
		break;

	case WM_ACTIVATE:
		c_ActiveDialogWindow = wParam ? m_Window : nullptr;
		break;
	}

	return FALSE;
}

void Dialog::AddTab(WORD controlId, Tab& tab, const WCHAR* text)
{
	HWND tabControl = GetControl(controlId);
	if (!m_TabControl)
	{
		m_TabControl = tabControl;
	}
	else
	{
		assert(m_TabControl == tabControl);
	}

	AddPage(tab);
	m_Tabs.push_back(&tab);

	TCITEM tci = { 0 };
	tci.mask = TCIF_TEXT;
	tci.pszText = (WCHAR*)text;
	TabCtrl_InsertItem(m_TabControl, (int)m_Tabs.size() - 1, &tci);
}

void Dialog::AddPage(Tab& tab)
{
	tab.Create(m_Window);
	m_Pages.push_back(&tab);
}

void Dialog::SelectTab(int index)
{
	assert(index >= 0 && index < (int)m_Tabs.size());
	TabCtrl_SetCurSel(m_TabControl, index);
	ActivateTab();
}

Dialog::Tab& Dialog::GetActiveTab()
{
	int index = TabCtrl_GetCurSel(m_TabControl);
	assert(index >= 0 && index < (int)m_Tabs.size());
	return *m_Tabs[index];
}

void Dialog::ActivateTab()
{
	for (auto* tab : m_Tabs)
	{
		EnableWindow(tab->GetWindow(), FALSE);
	}

	GetActiveTab().Activate();
}

void Dialog::Relayout()
{
	RelayoutControls();

	for (auto* tab : m_Pages)
	{
		if (!tab->GetWindow()) continue;

		RECT rect = tab->GetLayoutRect();
		const int width = rect.right - rect.left;
		const int height = rect.bottom - rect.top;
		SetWindowPos(tab->GetWindow(), nullptr, rect.left, rect.top, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
		tab->Relayout();
		tab->Resize(width, height);
	}
}

INT_PTR Dialog::HandleDpiChanged(WPARAM wParam, LPARAM lParam)
{
	m_Dpi = LOWORD(wParam);

	const RECT* suggested = (const RECT*)lParam;
	auto w = suggested->right - suggested->left;
	auto h = suggested->bottom - suggested->top;
	SetWindowPos(m_Window, nullptr, suggested->left, suggested->top, w, h, SWP_NOACTIVATE | SWP_NOZORDER);

	Relayout();
	HandleDpiChange();
	RedrawWindow(m_Window, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	return TRUE;
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
			const UINT dpi = GetWindowDpi(hWnd);
			const int arrowOffset = MulDiv(18, (int)dpi, 96);
			const int arrowTop = MulDiv(4, (int)dpi, 96);
			const int arrowSize = MulDiv(14, (int)dpi, 96);
			int arrowX = buttonRect.right - arrowOffset;
			int arrowY = buttonRect.top + arrowTop;
			RECT arrowRect = { arrowX, arrowY, arrowX + arrowSize, arrowY + arrowSize };

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
	m_Initialized(false),
	m_InitialMargin(),
	m_InitialDpi()
{
}

void Dialog::Tab::CreateTabWindow(short x, short y, short w, short h, HWND parent)
{
	const DWORD style = DS_CONTROL | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
	const DWORD exStyle = WS_EX_CONTROLPARENT;
	Show(L"", x, y, w, h, style, exStyle, parent, true);

	RECT rect;
	GetWindowRect(m_Window, &rect);
	MapWindowPoints(nullptr, parent, (POINT*)&rect, 2);

	RECT parentRect;
	GetClientRect(parent, &parentRect);

	m_InitialMargin.left = rect.left;
	m_InitialMargin.top = rect.top;
	m_InitialMargin.right = parentRect.right - rect.right;
	m_InitialMargin.bottom = parentRect.bottom - rect.bottom;
	m_InitialDpi = GetWindowDpi(parent);

	EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);
}

RECT Dialog::Tab::GetLayoutRect()
{
	HWND parent = GetParent(m_Window);
	const UINT dpi = GetWindowDpi(parent);

	RECT rect;
	GetClientRect(parent, &rect);
	rect.left = MulDiv(m_InitialMargin.left, (int)dpi, (int)m_InitialDpi);
	rect.top = MulDiv(m_InitialMargin.top, (int)dpi, (int)m_InitialDpi);
	rect.right -= MulDiv(m_InitialMargin.right, (int)dpi, (int)m_InitialDpi);
	rect.bottom -= MulDiv(m_InitialMargin.bottom, (int)dpi, (int)m_InitialDpi);
	return rect;
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
