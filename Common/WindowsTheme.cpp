// Derived from https://github.com/ozone10/win32-darkmodelib with additional changes.
// Original source code licensed under MPL v2 and MIT
// Copyright (c) 2025-2026 ozone10, 2019 Richard Yu, 2018 Stephen Eckels.

#include "StdAfx.h"
#include "WindowsTheme.h"
#include "Dialog.h"
#include "Platform.h"

#include <atomic>
#include <dwmapi.h>
#include <vsstyle.h>
#include <vssym32.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

namespace {

constexpr WCHAR g_MessageBoxProp[] = L"RainmeterDarkModeMessageBox";

constexpr COLORREF g_PageBackground = RGB(37, 37, 37);
constexpr COLORREF g_Background = RGB(32, 32, 32);
constexpr COLORREF g_ControlBackground = RGB(56, 56, 56);
constexpr COLORREF g_HotBackground = RGB(69, 69, 69);
constexpr COLORREF g_Text = RGB(224, 224, 224);
constexpr COLORREF g_Accent = RGB(96, 205, 255);
constexpr COLORREF g_DisabledText = RGB(128, 128, 128);
constexpr COLORREF g_Edge = RGB(75, 75, 75);

enum class ControlType
{
	Button,
	Tab,
	ComboBox,
	Header,
	Edit,
	Spinner,
	ListView,
	Separator,
	ListBox
};

BOOL CALLBACK ApplyControl(HWND window, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);

typedef bool (WINAPI* AllowDarkModeForWindowProc)(HWND, bool);
typedef int (WINAPI* SetPreferredAppModeProc)(int);
typedef void (WINAPI* RefreshColorPolicyProc)();
typedef HTHEME (WINAPI* OpenNcThemeDataProc)(HWND, LPCWSTR);

struct ThemeState
{
	std::atomic<bool> enabled = false;
	AllowDarkModeForWindowProc allowDarkModeForWindowFunc = nullptr;
	SetPreferredAppModeProc setPreferredAppModeFunc = nullptr;
	RefreshColorPolicyProc refreshImmersiveColorPolicyStateFunc = nullptr;
	RefreshColorPolicyProc flushMenuThemesFunc = nullptr;
	OpenNcThemeDataProc openNcThemeDataFunc = nullptr;
	ULONG_PTR* scrollbarThunk = nullptr;
	ULONG_PTR originalScrollbarThunk = 0;
	HBRUSH pageBackground = CreateSolidBrush(g_PageBackground);
	HBRUSH background = CreateSolidBrush(g_Background);
	HBRUSH controlBackground = CreateSolidBrush(g_ControlBackground);
	HBRUSH hotBackground = CreateSolidBrush(g_HotBackground);
	HBRUSH edge = CreateSolidBrush(g_Edge);
	HBRUSH separator = CreateSolidBrush(RGB(48, 48, 48));
	HBRUSH focus = CreateSolidBrush(RGB(0, 120, 215));

	~ThemeState()
	{
		if (scrollbarThunk)
		{
			DWORD protection;
			if (VirtualProtect(scrollbarThunk, sizeof(*scrollbarThunk), PAGE_READWRITE, &protection))
			{
				*scrollbarThunk = originalScrollbarThunk;
				VirtualProtect(scrollbarThunk, sizeof(*scrollbarThunk), protection, &protection);
			}
		}

		DeleteObject(pageBackground);
		DeleteObject(background);
		DeleteObject(controlBackground);
		DeleteObject(hotBackground);
		DeleteObject(edge);
		DeleteObject(separator);
		DeleteObject(focus);
	}
};

ThemeState& GetState()
{
	static ThemeState state;
	return state;
}

bool HasClass(HWND window, const WCHAR* name)
{
	WCHAR className[64];
	return GetClassName(window, className, _countof(className)) && wcscmp(className, name) == 0;
}

bool IsTabSurface(HWND window)
{
	for (HWND parent = window; parent; parent = GetParent(parent))
	{
		if (HasClass(parent, L"#32770")) return (GetWindowLongPtr(parent, GWL_STYLE) & WS_CHILD) != 0;
	}
	return false;
}

HBRUSH GetSurfaceBrush(HWND window)
{
	const bool messageText = GetProp(GetParent(window), g_MessageBoxProp) && !HasClass(window, WC_BUTTON);
	return (IsTabSurface(window) || messageText) ? GetState().pageBackground : GetState().background;
}

// Keep the message area separate from the button footer, as in the native light theme.
// Derive the footer size from the buttons so it follows the dialog layout and DPI.
void PaintMessageBox(HWND window, HDC dc)
{
	RECT rect;
	GetClientRect(window, &rect);
	FillRect(dc, &rect, GetState().background);
	int buttonTop = rect.bottom;
	int buttonBottom = 0;
	HWND button = nullptr;
	while ((button = FindWindowEx(window, button, WC_BUTTON, nullptr)) != nullptr)
	{
		if (!IsWindowVisible(button)) continue;

		RECT bounds;
		GetWindowRect(button, &bounds);
		MapWindowPoints(nullptr, window, (POINT*)&bounds, 2);
		buttonTop = std::min(buttonTop, (int)bounds.top);
		buttonBottom = std::max(buttonBottom, (int)bounds.bottom);
	}
	if (buttonBottom) rect.bottom = std::max(0, buttonTop - (int)(rect.bottom - buttonBottom));
	FillRect(dc, &rect, GetState().pageBackground);
}

int Scale(HWND window, int value)
{
	return MulDiv(value, (int)GetDpiForWindow(window), 96);
}

bool IsHot(HWND window, const RECT& rect)
{
	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(window, &cursor);
	return PtInRect(&rect, cursor) != FALSE;
}

void DrawArrow(HDC dc, RECT rect, bool up, COLORREF color, HWND window)
{
	const int x = (rect.left + rect.right) / 2;
	const int y = (rect.top + rect.bottom) / 2;
	const int size = Scale(window, 3);
	const int direction = up ? -1 : 1;
	const POINT points[] = { { x - size, y - direction * size / 2 }, { x + size, y - direction * size / 2 }, { x, y + direction * size } };
	HBRUSH brush = CreateSolidBrush(color);
	HGDIOBJ oldBrush = SelectObject(dc, brush);
	HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
	Polygon(dc, points, _countof(points));
	SelectObject(dc, oldPen);
	SelectObject(dc, oldBrush);
	DeleteObject(brush);
}

void DrawLabel(HWND window, HDC dc, RECT rect, UINT flags)
{
	WCHAR text[4096];
	GetWindowText(window, text, _countof(text));
	if (SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL) flags |= DT_HIDEPREFIX;
	DrawText(dc, text, -1, &rect, flags);
}

void DrawControlFrame(HWND window, HDC dc, const RECT& rect, const WCHAR* className, int part, int status, HBRUSH background, RECT* content = nullptr)
{
	// Keep the native outline and content padding, but replace the theme's colors.
	// Fall back to a rectangular frame if the theme cannot supply an outline.
	HTHEME theme = OpenThemeData(window, className);
	HRGN region = nullptr;
	if (theme && SUCCEEDED(GetThemeBackgroundRegion(theme, dc, part, status, &rect, &region)))
	{
		FillRgn(dc, region, background);
		FrameRgn(dc, region, GetState().edge, 1, 1);
		DeleteObject(region);
	}
	else
	{
		FillRect(dc, &rect, background);
		FrameRect(dc, &rect, GetState().edge);
	}

	if (content)
	{
		*content = rect;
		if (theme) GetThemeBackgroundContentRect(theme, dc, part, status, &rect, content);
	}

	if (theme) CloseThemeData(theme);
}

// Group boxes surround sibling controls, so painting their background must leave those controls intact.
void ClipGroupBoxSiblings(HWND window, HDC dc)
{
	for (HWND sibling = GetWindow(GetParent(window), GW_CHILD); sibling; sibling = GetWindow(sibling, GW_HWNDNEXT))
	{
		if (sibling == window || !IsWindowVisible(sibling)) continue;

		if (HasClass(sibling, WC_BUTTON) && (GetWindowLongPtr(sibling, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX) continue;

		RECT rect;
		GetWindowRect(sibling, &rect);
		MapWindowPoints(nullptr, window, (POINT*)&rect, 2);
		ExcludeClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
	}
}

void DrawButton(HWND window, HDC dc, RECT rect)
{
	auto& state = GetState();
	const DWORD style = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	const DWORD type = style & BS_TYPEMASK;
	const bool enabled = IsWindowEnabled(window) != FALSE;
	const LRESULT buttonState = SendMessage(window, BM_GETSTATE, 0, 0);
	const bool pressed = (buttonState & BST_PUSHED) != 0;
	const bool hot = enabled && IsHot(window, rect);
	FillRect(dc, &rect, GetSurfaceBrush(window));

	if (type == BS_GROUPBOX)
	{
		// Center the top border on the text height, then erase it behind the caption.
		TEXTMETRIC metrics;
		GetTextMetrics(dc, &metrics);
		RECT frame = rect;
		frame.top += metrics.tmHeight / 2;
		FrameRect(dc, &frame, state.edge);
		WCHAR text[4096];
		const int length = GetWindowText(window, text, _countof(text));
		SIZE size;
		GetTextExtentPoint32(dc, text, length, &size);
		RECT label = { Scale(window, 8), 0, Scale(window, 12) + size.cx, metrics.tmHeight };
		FillRect(dc, &label, GetSurfaceBrush(window));
		DrawLabel(window, dc, label, DT_LEFT | DT_SINGLELINE);
		return;
	}

	const bool radio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
	const bool check = radio || type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE;
	if (check)
	{
		// BS_LEFTTEXT places the checkbox or radio glyph on the right of its label.
		const int size = Scale(window, 13);
		RECT glyph = { rect.left, (rect.bottom - size) / 2, rect.left + size, (rect.bottom + size) / 2 };
		if (style & BS_LEFTTEXT) OffsetRect(&glyph, rect.right - size, 0);
		const int checked = (int)SendMessage(window, BM_GETCHECK, 0, 0);
		HTHEME theme = OpenThemeData(window, L"DarkMode_Explorer::Button");
		if (!theme) theme = OpenThemeData(window, L"Button");
		if (theme)
		{
			const int part = radio ? BP_RADIOBUTTON : BP_CHECKBOX;
			// Theme states come in groups of four: normal, hot, pressed, and disabled,
			// repeated for unchecked, checked, and mixed values.
			const int status = 1 + checked * 4 + (!enabled ? 3 : pressed ? 2 : hot ? 1 : 0);
			DrawThemeBackground(theme, dc, part, status, &glyph, nullptr);
			CloseThemeData(theme);
		}

		// Remove the glyph and its gap from the label's drawing area.
		if (style & BS_LEFTTEXT)
		{
			rect.right -= size + Scale(window, 5);
		}
		else
		{
			rect.left += size + Scale(window, 5);
		}

		DrawLabel(window, dc, rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		const int status = !enabled ? PBS_DISABLED : pressed ? PBS_PRESSED : hot ? PBS_HOT : type == BS_DEFPUSHBUTTON || GetFocus() == window ? PBS_DEFAULTED : PBS_NORMAL;
		RECT content;
		DrawControlFrame(window, dc, rect, L"Button", BP_PUSHBUTTON, status, pressed || hot ? state.hotBackground : state.controlBackground, &content);
		// Match the native push button's slight label shift while pressed.
		if (pressed) OffsetRect(&content, 1, 1);
		DrawLabel(window, dc, content, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// Honor Windows' keyboard focus visibility. Check controls outline their text;
	// push buttons get an inset accent outline around the whole button.
	if (GetFocus() == window && !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
	{
		if (check)
		{
			WCHAR text[4096];
			GetWindowText(window, text, _countof(text));
			RECT label = rect;
			DrawText(dc, text, -1, &label, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
			const LONG height = label.bottom - label.top;
			label.top = (rect.top + rect.bottom - height) / 2;
			label.bottom = label.top + height;
			rect = label;
			InflateRect(&rect, 1, 1);
			DrawFocusRect(dc, &rect);
		}
		else
		{
			InflateRect(&rect, -1, -1);
			HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
			HGDIOBJ oldPen = SelectObject(dc, pen);
			HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
			const int roundness = Scale(window, 4);
			RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, roundness, roundness);
			SelectObject(dc, oldBrush);
			SelectObject(dc, oldPen);
			DeleteObject(pen);
		}
	}
}

void DrawTabs(HWND window, HDC dc, RECT rect)
{
	auto& state = GetState();
	FillRect(dc, &rect, GetSurfaceBrush(window));

	// Align the page border with the tab row so the selected tab can join the page.
	RECT page = rect;
	TabCtrl_AdjustRect(window, FALSE, &page);
	const int padding = 2;
	InflateRect(&page, padding, padding);
	const int count = TabCtrl_GetItemCount(window);
	RECT first;
	if (count && TabCtrl_GetItemRect(window, 0, &first))
	{
		page.left = first.left - padding;
		page.top = first.bottom - 1;
	}

	FillRect(dc, &page, state.pageBackground);
	FrameRect(dc, &page, state.edge);
	const int selected = TabCtrl_GetCurSel(window);

	// Draw the selected tab last because its enlarged border overlaps its neighbors.
	for (int position = 0; position <= count; ++position)
	{
		const int i = position == count ? selected : position;
		if (i < 0 || i >= count || (position != count && i == selected)) continue;

		RECT item;
		if (!TabCtrl_GetItemRect(window, i, &item)) continue;

		const RECT label = item;
		const bool active = i == selected;
		if (active)
		{
			item.left -= padding;
			item.right += padding;
			item.top -= padding;
		}

		item.bottom = page.top + 1;
		FillRect(dc, &item, active ? state.pageBackground : IsHot(window, item) ? state.hotBackground : state.background);
		FrameRect(dc, &item, state.edge);
		if (active)
		{
			RECT bottom = { item.left + 1, page.top, item.right - 1, page.top + 1 };

			// Hide the shared border so the selected tab appears connected to the page.
			FillRect(dc, &bottom, state.pageBackground);
		}

		WCHAR text[512];
		TCITEM entry = { 0 };
		entry.mask = TCIF_TEXT;
		entry.pszText = text;
		entry.cchTextMax = _countof(text);
		RECT textRect = label;
		if (TabCtrl_GetItem(window, i, &entry)) DrawText(dc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		if (active && GetFocus() == window && !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
		{
			InflateRect(&item, -Scale(window, 3), -Scale(window, 3));
			DrawFocusRect(dc, &item);
		}
	}
}

void DrawComboBox(HWND window, HDC dc, RECT rect)
{
	auto& state = GetState();
	FillRect(dc, &rect, GetSurfaceBrush(window));
	COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
	if (!GetComboBoxInfo(window, &info)) return;
	const DWORD style = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	const bool list = (style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST;
	const bool enabled = IsWindowEnabled(window) != FALSE;
	const bool hot = enabled && IsHot(window, info.rcButton);
	const int status = !enabled ? CBRO_DISABLED : hot ? CBRO_HOT : CBRO_NORMAL;
	DrawControlFrame(window, dc, rect, L"Combobox", CP_READONLY, status, enabled ? state.controlBackground : GetSurfaceBrush(window));

	// Editable combo boxes have a child edit control that paints its own text.
	// Only drop-down lists need us to draw the selected value and focus rectangle.
	if (list)
	{
		RECT label = info.rcItem;
		DrawLabel(window, dc, label, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		if (GetFocus() == window && !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) DrawFocusRect(dc, &info.rcItem);
	}

	if (hot)
	{
		RECT button = info.rcButton;
		InflateRect(&button, -1, -1);
		FillRect(dc, &button, state.hotBackground);
	}

	DrawArrow(dc, info.rcButton, false, enabled ? g_Text : g_DisabledText, window);
}

void DrawListBox(HWND window, HDC dc, RECT rect)
{
	FillRect(dc, &rect, GetState().background);
	const int count = (int)SendMessage(window, LB_GETCOUNT, 0, 0);
	const int selected = (int)SendMessage(window, LB_GETCURSEL, 0, 0);
	const int top = (int)SendMessage(window, LB_GETTOPINDEX, 0, 0);

	// Use native item bounds to preserve scrolling, including a partially visible first item.
	for (int i = std::max(0, top); i < count; ++i)
	{
		RECT item;
		if (SendMessage(window, LB_GETITEMRECT, i, (LPARAM)&item) == LB_ERR) continue;

		if (item.top >= rect.bottom) break;
		const bool highlight = i == selected;

		// Keep the system selection colors so selected text retains its usual contrast.
		if (highlight) FillRect(dc, &item, GetSysColorBrush(COLOR_HIGHLIGHT));
		SetTextColor(dc, !IsWindowEnabled(window) ? g_DisabledText : highlight ? GetSysColor(COLOR_HIGHLIGHTTEXT) : g_Text);

		const int length = (int)SendMessage(window, LB_GETTEXTLEN, i, 0);
		if (length == LB_ERR) continue;

		WCHAR text[8192];
		if (length >= _countof(text)) continue;

		SendMessage(window, LB_GETTEXT, i, (LPARAM)text);
		item.left += 2;
		DrawText(dc, text, -1, &item, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
}

void DrawHeader(HWND window, HDC dc, RECT rect)
{
	auto& state = GetState();
	FillRect(dc, &rect, state.background);
	const bool clickable = (GetWindowLongPtr(window, GWL_STYLE) & HDS_BUTTONS) != 0;
	for (int i = 0; i < Header_GetItemCount(window); ++i)
	{
		RECT item;
		if (!Header_GetItemRect(window, i, &item)) continue;

		FillRect(dc, &item, clickable && IsWindowEnabled(window) && IsHot(window, item) ? state.hotBackground : state.background);
		WCHAR text[512];
		HDITEM entry = { 0 };
		entry.mask = HDI_TEXT | HDI_FORMAT;
		entry.pszText = text;
		entry.cchTextMax = _countof(text);
		if (!Header_GetItem(window, i, &entry)) continue;

		const UINT format = entry.fmt & HDF_CENTER ? DT_CENTER : entry.fmt & HDF_RIGHT ? DT_RIGHT : DT_LEFT;

		// Respect the native header padding and reserve room for a sort arrow.
		RECT content = item;
		HTHEME theme = GetWindowTheme(window);
		if (theme) GetThemeBackgroundContentRect(theme, dc, HP_HEADERITEM, HIS_NORMAL, &item, &content);
		const int margin = (int)SendMessage(window, HDM_GETBITMAPMARGIN, 0, 0);
		InflateRect(&content, -margin, 0);
		item = content;
		if (entry.fmt & (HDF_SORTUP | HDF_SORTDOWN))
		{
			RECT arrow = item;
			arrow.left = arrow.right - Scale(window, 16);
			DrawArrow(dc, arrow, (entry.fmt & HDF_SORTUP) != 0, g_Text, window);
			item.right = arrow.left;
		}

		DrawText(dc, text, -1, &item, format | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
	}

	// Draw dividers after all cells so adjacent backgrounds cannot cover them.
	for (int i = 0; i < Header_GetItemCount(window); ++i)
	{
		RECT item;
		if (!Header_GetItemRect(window, i, &item)) continue;

		RECT divider = { item.right - 1, item.top, item.right, item.bottom };
		FillRect(dc, &divider, state.separator);
	}
}

void DrawSpinner(HWND window, HDC dc, RECT rect)
{
	auto& state = GetState();
	const bool horizontal = (GetWindowLongPtr(window, GWL_STYLE) & UDS_HORZ) != 0;
	const bool enabled = IsWindowEnabled(window) != FALSE;
	for (int i = 0; i < 2; ++i)
	{
		RECT item = rect;
		if (horizontal)
		{
			if (i == 0)
			{
				item.right = (rect.left + rect.right) / 2;
			}
			else
			{
				item.left = (rect.left + rect.right) / 2;
			}
		}
		else
		{
			if (i == 0)
			{
				item.bottom = (rect.top + rect.bottom) / 2;
			}
			else
			{
				item.top = (rect.top + rect.bottom) / 2;
			}
		}

		FillRect(dc, &item, enabled && IsHot(window, item) ? state.hotBackground : state.controlBackground);
		FrameRect(dc, &item, state.edge);
		if (horizontal)
		{
			const WCHAR* arrow = i == 0 ? L"\x2039" : L"\x203A";
			DrawText(dc, arrow, 1, &item, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		else
		{
			DrawArrow(dc, item, i == 0, enabled ? g_Text : g_DisabledText, window);
		}
	}
}

void DrawSeparator(HWND window, HDC dc, RECT rect)
{
	FillRect(dc, &rect, GetSurfaceBrush(window));
	if ((GetWindowLongPtr(window, GWL_STYLE) & SS_TYPEMASK) == SS_ETCHEDHORZ)
	{
		rect.bottom = rect.top + 1;
	}
	else
	{
		rect.right = rect.left + 1;
	}

	FillRect(dc, &rect, GetState().edge);
}

void PaintControl(HWND window, HDC dc, ControlType type)
{
	RECT rect;
	GetClientRect(window, &rect);
	if (rect.right <= 0 || rect.bottom <= 0) return;

	const int saved = SaveDC(dc);
	auto font = (HFONT)SendMessage(window, WM_GETFONT, 0, 0);
	if (font) SelectObject(dc, font);
	SetTextColor(dc, IsWindowEnabled(window) ? g_Text : g_DisabledText);
	SetBkMode(dc, TRANSPARENT);

	switch (type)
	{
	case ControlType::Button:
		DrawButton(window, dc, rect);

		// Include the menu arrow in the buffer so hover repaints cannot briefly erase it.
		Dialog::DrawMenuButtonArrow(window, dc);
		break;

	case ControlType::Tab:
		DrawTabs(window, dc, rect);
		break;

	case ControlType::ComboBox:
		DrawComboBox(window, dc, rect);
		break;

	case ControlType::Header:
		DrawHeader(window, dc, rect);
		break;

	case ControlType::Spinner:
		DrawSpinner(window, dc, rect);
		break;

	case ControlType::ListBox:
		DrawListBox(window, dc, rect);
		break;

	case ControlType::Separator:
		DrawSeparator(window, dc, rect);
		break;
	}

	RestoreDC(dc, saved);
}

LRESULT ControlColor(UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC dc = (HDC)wParam;
	HWND control = (HWND)lParam;
	auto& state = GetState();
	SetTextColor(dc, IsWindowEnabled(control) ? g_Text : g_DisabledText);
	const DWORD style = (DWORD)GetWindowLongPtr(control, GWL_STYLE);
	const bool edit = HasClass(control, WC_EDIT);
	const bool borderlessReadOnly = edit && (style & ES_READONLY) && !(style & WS_BORDER) && !(GetWindowLongPtr(control, GWL_EXSTYLE) & WS_EX_CLIENTEDGE);
	const bool input = !borderlessReadOnly && (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || edit);
	SetBkColor(dc, !input && GetSurfaceBrush(control) == state.pageBackground ? g_PageBackground : g_Background);
	return (LRESULT)(input ? state.background : GetSurfaceBrush(control));
}

// Control color messages affect the client area but leave the native border in its light theme.
void PaintEditBorder(HWND window)
{
	RECT rect;
	GetWindowRect(window, &rect);
	OffsetRect(&rect, -rect.left, -rect.top);
	const RECT bounds = rect;
	HDC dc = GetWindowDC(window);
	if (!dc) return;
	FrameRect(dc, &rect, GetState().edge);
	if (GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)
	{
		InflateRect(&rect, -1, -1);
		FrameRect(dc, &rect, GetFocus() == window ? GetState().edge : GetState().background);
	}

	if (GetFocus() == window && IsWindowEnabled(window) && HasClass(window, WC_EDIT))
	{
		RECT accent = { bounds.left, bounds.bottom - Scale(window, 1), bounds.right, bounds.bottom };
		FillRect(dc, &accent, GetState().focus);
	}

	ReleaseDC(window, dc);
}

// The native list-view theme does not give group headers our dark colors.
// Paint over the native headers while keeping their layout, collapse state, and focus indication.
void PaintGroupHeaders(HWND window, HDC dc)
{
	if (!ListView_IsGroupViewEnabled(window)) return;

	// Keep scrolled group headers from painting over the column header or outside the list.
	const int saved = SaveDC(dc);
	RECT client;
	GetClientRect(window, &client);
	HWND header = ListView_GetHeader(window);
	if (header && IsWindowVisible(header))
	{
		RECT rect;
		GetWindowRect(header, &rect);
		MapWindowPoints(nullptr, window, (POINT*)&rect, 2);
		client.top = std::max(client.top, rect.bottom);
	}

	IntersectClipRect(dc, client.left, client.top, client.right, client.bottom);
	HTHEME theme = GetWindowTheme(window);
	HFONT font = (HFONT)SendMessage(window, WM_GETFONT, 0, 0);
	if (font) SelectObject(dc, font);
	SetBkMode(dc, TRANSPARENT);
	const COLORREF color = IsWindowEnabled(window) ? RGB(100, 150, 220) : g_DisabledText;
	SetTextColor(dc, color);
	const int count = (int)ListView_GetGroupCount(window);
	for (int i = 0; i < count; ++i)
	{
		// Ask the list view for its current group state and bounds so scrolling and
		// collapsing still use the native layout. Empty and hidden groups need no header.
		WCHAR text[1024];
		LVGROUP group = { 0 };
		group.cbSize = sizeof(group);
		group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_ALIGN | LVGF_STATE | LVGF_ITEMS;
		group.pszHeader = text;
		group.cchHeader = _countof(text);
		group.stateMask = LVGS_COLLAPSED | LVGS_FOCUSED | LVGS_COLLAPSIBLE | LVGS_HIDDEN | LVGS_NOHEADER;
		if (!ListView_GetGroupInfoByIndex(window, i, &group)) continue;

		if (!group.cItems || (group.state & (LVGS_HIDDEN | LVGS_NOHEADER))) continue;

		RECT headerRect;
		if (!ListView_GetGroupRect(window, group.iGroupId, LVGGR_HEADER, &headerRect)) continue;

		const bool hot = IsWindowEnabled(window) && IsHot(window, headerRect);
		// Cover the native header before drawing its replacement in our dark colors.
		HBRUSH highlight = hot ? CreateSolidBrush(RGB(16, 54, 74)) : nullptr;
		FillRect(dc, &headerRect, highlight ? highlight : GetState().background);
		if (highlight) DeleteObject(highlight);

		// Use the theme's padding and the native label's vertical position to keep
		// the replacement aligned with the list view's own header and hit targets.
		RECT content = headerRect;
		const int status = group.state & LVGS_COLLAPSED ? (hot ? LVGH_CLOSEHOT : LVGH_CLOSE) : (hot ? LVGH_OPENHOT : LVGH_OPEN);
		if (!theme || FAILED(GetThemeBackgroundContentRect(theme, dc, LVP_GROUPHEADER, status, &headerRect, &content))) InflateRect(&content, -Scale(window, 6), 0);
		content.left = std::max(content.left, headerRect.left + Scale(window, 6));
		RECT nativeLabel;
		if (ListView_GetGroupRect(window, group.iGroupId, LVGGR_LABEL, &nativeLabel))
		{
			content.top = nativeLabel.top;
			content.bottom = nativeLabel.bottom;
		}

		// Measure with the theme's group font, falling back to the list-view font.
		LOGFONT fontInfo;
		HFONT groupFont = theme && SUCCEEDED(GetThemeFont(theme, dc, LVP_GROUPHEADER, status, TMT_FONT, &fontInfo)) ? CreateFontIndirect(&fontInfo) : nullptr;
		HGDIOBJ oldFont = groupFont ? SelectObject(dc, groupFont) : nullptr;
		SIZE textSize;
		GetTextExtentPoint32(dc, text, (int)wcslen(text), &textSize);

		// Reserve space on the right for the collapse arrow before aligning the text.
		RECT arrow = content;
		SIZE arrowSize = { Scale(window, 13), Scale(window, 13) };
		if (theme) GetThemePartSize(theme, dc, LVP_COLLAPSEBUTTON, 1, nullptr, TS_TRUE, &arrowSize);
		arrow.left = arrow.right - arrowSize.cx;
		arrow.top = (content.top + content.bottom - arrowSize.cy) / 2;
		arrow.bottom = arrow.top + arrowSize.cy;
		RECT label = content;
		if (group.state & LVGS_COLLAPSIBLE) label.right = arrow.left - Scale(window, 8);
		const int width = std::min(textSize.cx, label.right - label.left);
		if (group.uAlign & LVGA_HEADER_CENTER)
		{
			label.left += (label.right - label.left - width) / 2;
		}
		else if (group.uAlign & LVGA_HEADER_RIGHT)
		{
			label.left = label.right - width;
		}

		label.right = label.left + width;
		DrawText(dc, text, -1, &label, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

		// Fill the remaining space with a divider, leaving gaps beside the text and arrow.
		HPEN linePen = CreatePen(PS_SOLID, 1, IsWindowEnabled(window) ? RGB(55, 95, 120) : g_Edge);
		HGDIOBJ oldPen = SelectObject(dc, linePen);
		const int lineY = (content.top + content.bottom) / 2;
		const int lineStart = label.right + Scale(window, 2);
		const int lineEnd = group.state & LVGS_COLLAPSIBLE ? arrow.left - Scale(window, 8) : content.right;
		if (lineStart < lineEnd)
		{
			MoveToEx(dc, lineStart, lineY, nullptr);
			LineTo(dc, lineEnd, lineY);
		}

		SelectObject(dc, oldPen);
		DeleteObject(linePen);

		// Draw a downward chevron for a collapsed group and an upward one for an open group.
		if (group.state & LVGS_COLLAPSIBLE)
		{
			HPEN arrowPen = CreatePen(PS_SOLID, Scale(window, 2), color);
			oldPen = SelectObject(dc, arrowPen);
			const int x = (arrow.left + arrow.right) / 2;
			const int y = (arrow.top + arrow.bottom) / 2;
			const int size = Scale(window, 3);
			const int direction = group.state & LVGS_COLLAPSED ? 1 : -1;
			MoveToEx(dc, x - size, y - direction * size / 2, nullptr);
			LineTo(dc, x, y + direction * size / 2);
			LineTo(dc, x + size, y - direction * size / 2);
			SelectObject(dc, oldPen);
			DeleteObject(arrowPen);
		}

		// Match the native keyboard focus rules; hovering alone must not show this outline.
		if ((group.state & LVGS_FOCUSED) && GetFocus() == window && !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
		{
			HPEN pen = CreatePen(PS_DOT, 1, RGB(75, 95, 120));
			HGDIOBJ oldPen = SelectObject(dc, pen);
			HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
			Rectangle(dc, label.left, label.top, label.right, label.bottom);
			SelectObject(dc, oldBrush);
			SelectObject(dc, oldPen);
			DeleteObject(pen);
		}

		// A selected font must be restored before deleting it. RestoreDC below also
		// restores the caller's font, colors, and clipping region after all groups are drawn.
		if (groupFont)
		{
			SelectObject(dc, oldFont);
			DeleteObject(groupFont);
		}
	}
	RestoreDC(dc, saved);
}

// Combine native item painting and custom group headers in one buffer to avoid flicker.
void PaintListView(HWND window)
{
	PAINTSTRUCT paint = { 0 };
	HDC dc = BeginPaint(window, &paint);
	RECT rect;
	GetClientRect(window, &rect);
	HDC buffer = CreateCompatibleDC(dc);
	HBITMAP bitmap = CreateCompatibleBitmap(dc, std::max(1L, rect.right), std::max(1L, rect.bottom));
	if (buffer && bitmap)
	{
		HGDIOBJ oldBitmap = SelectObject(buffer, bitmap);
		FillRect(buffer, &rect, GetState().background);
		DefSubclassProc(window, WM_PRINTCLIENT, (WPARAM)buffer, PRF_CLIENT | PRF_ERASEBKGND);
		PaintGroupHeaders(window, buffer);
		HWND header = ListView_GetHeader(window);
		if (header && IsWindowVisible(header))
		{
			RECT headerRect;
			GetWindowRect(header, &headerRect);
			MapWindowPoints(nullptr, window, (POINT*)&headerRect, 2);
			ExcludeClipRect(dc, headerRect.left, headerRect.top, headerRect.right, headerRect.bottom);
		}

		BitBlt(dc, 0, 0, rect.right, rect.bottom, buffer, 0, 0, SRCCOPY);
		SelectObject(buffer, oldBitmap);
	}
	else
	{
		DefSubclassProc(window, WM_PRINTCLIENT, (WPARAM)dc, PRF_CLIENT | PRF_ERASEBKGND);
		PaintGroupHeaders(window, dc);
	}

	if (bitmap) DeleteObject(bitmap);
	if (buffer) DeleteDC(buffer);
	EndPaint(window, &paint);
}

LRESULT HandleListViewMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	// The group-view query sends a message back through this subclass. Forward
	// non-paint messages before making that query to avoid recursive queries.
	if (message != WM_PAINT && message != WM_ERASEBKGND && message != WM_PRINTCLIENT) return DefSubclassProc(window, message, wParam, lParam);

	// Lists without group headers need no replacement painting. Keep their native
	// paint path so scrolling can use the control's own double buffering.
	if (!ListView_IsGroupViewEnabled(window)) return DefSubclassProc(window, message, wParam, lParam);

	if (message == WM_PAINT)
	{
		PaintListView(window);
		return 0;
	}

	if (message == WM_ERASEBKGND) return TRUE;
	const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
	if (message == WM_PRINTCLIENT)
	{
		PaintGroupHeaders(window, (HDC)wParam);
	}

	return result;
}

LRESULT HandleEditMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
	if (message == WM_NCPAINT || message == WM_SETFOCUS || message == WM_KILLFOCUS) PaintEditBorder(window);
	return result;
}

// Buffer the custom painting so background and content appear together without flicker.
void PaintControlMessage(HWND window, UINT message, WPARAM wParam, ControlType type)
{
	PAINTSTRUCT paint = { 0 };
	HDC dc = message == WM_PAINT ? BeginPaint(window, &paint) : (HDC)wParam;
	const int saved = SaveDC(dc);
	if (type == ControlType::Button && (GetWindowLongPtr(window, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX) ClipGroupBoxSiblings(window, dc);
	RECT rect;
	GetClientRect(window, &rect);
	HDC buffer = CreateCompatibleDC(dc);
	HBITMAP bitmap = CreateCompatibleBitmap(dc, std::max(1L, rect.right), std::max(1L, rect.bottom));
	if (buffer && bitmap)
	{
		HGDIOBJ oldBitmap = SelectObject(buffer, bitmap);
		PaintControl(window, buffer, type);
		BitBlt(dc, 0, 0, rect.right, rect.bottom, buffer, 0, 0, SRCCOPY);
		SelectObject(buffer, oldBitmap);
	}
	else
	{
		PaintControl(window, dc, type);
	}

	if (bitmap) DeleteObject(bitmap);
	if (buffer) DeleteDC(buffer);
	RestoreDC(dc, saved);
	if (message == WM_PAINT) EndPaint(window, &paint);
}

LRESULT HandleControlMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam, ControlType type)
{
	if (message == WM_ERASEBKGND) return TRUE;
	if (message == WM_PAINT || message == WM_PRINTCLIENT)
	{
		PaintControlMessage(window, message, wParam, type);
		return 0;
	}

	if (message == WM_MOUSEMOVE)
	{
		TRACKMOUSEEVENT track = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, window, 0 };
		TrackMouseEvent(&track);
	}

	const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
	switch (message)
	{
	case WM_MOUSEMOVE:
	case WM_MOUSELEAVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_ENABLE:
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_UPDATEUISTATE:
	case WM_SETTEXT:
	case WM_SETFONT:
	case BM_SETCHECK:
	case BM_SETSTATE:
	case TCM_SETCURSEL:
	case CB_SETCURSEL:
	case LB_SETCURSEL:
	case LB_SETTOPINDEX:
	case WM_KEYDOWN:
	case WM_MOUSEWHEEL:
	case WM_VSCROLL:
		InvalidateRect(window, nullptr, FALSE);
		break;
	}

	return result;
}

bool ShouldDeferControlPaint(HWND window, UINT message, ControlType type)
{
	if (type == ControlType::Edit || type == ControlType::ListView || !IsWindowVisible(window)) return false;

	switch (message)
	{
	case WM_ENABLE:
	case WM_SETTEXT:
	case WM_SETFONT:
	case BM_SETCHECK:
	case BM_SETSTATE:
	case CB_SETCURSEL:
		return true;

	case WM_MOUSEMOVE:
		return type == ControlType::ListBox;
	}

	return false;
}

// These messages can make the native control paint immediately. Suppress that painting
// while its state changes, then invalidate it so our paint handler draws the updated control.
LRESULT HandleDeferredControlPaint(HWND window, UINT message, WPARAM wParam, LPARAM lParam, ControlType type)
{
	DefSubclassProc(window, WM_SETREDRAW, FALSE, 0);
	const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
	DefSubclassProc(window, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(window, nullptr, FALSE);

	if (type == ControlType::ListBox && WindowsTheme::IsUsingDarkMode()) UpdateWindow(window);

	return result;
}

LRESULT CALLBACK ControlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	const auto type = (ControlType)data;
	if (message == WM_PARENTNOTIFY && LOWORD(wParam) == WM_CREATE) ApplyControl((HWND)lParam, 0);
	if (message == WM_NCDESTROY) RemoveWindowSubclass(window, ControlProc, id);
	if (ShouldDeferControlPaint(window, message, type)) return HandleDeferredControlPaint(window, message, wParam, lParam, type);
	if (!WindowsTheme::IsUsingDarkMode()) return DefSubclassProc(window, message, wParam, lParam);
	if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC) return ControlColor(message, wParam, lParam);
	if (type == ControlType::ListView) return HandleListViewMessage(window, message, wParam, lParam);
	if (type == ControlType::Edit) return HandleEditMessage(window, message, wParam, lParam);
	return HandleControlMessage(window, message, wParam, lParam, type);
}

void SetControlSubclass(HWND window, ControlType type)
{
	SetWindowSubclass(window, ControlProc, 0, (DWORD_PTR)type);
}

BOOL CALLBACK ApplyControl(HWND window, LPARAM lParam)
{
	auto& state = GetState();
	WindowsTheme::ApplyToControl(window);

	WCHAR className[64];
	if (!GetClassName(window, className, _countof(className))) return TRUE;

	if (wcscmp(className, WC_BUTTON) == 0)
	{
		const DWORD style = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
		if ((style & BS_TYPEMASK) != BS_OWNERDRAW) SetControlSubclass(window, ControlType::Button);
	}
	else if (wcscmp(className, WC_TABCONTROL) == 0)
	{
		SetControlSubclass(window, ControlType::Tab);
		WindowsTheme::ApplyToTooltip(TabCtrl_GetToolTips(window));
	}
	else if (wcscmp(className, WC_COMBOBOX) == 0)
	{
		const DWORD style = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
		if (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) && (style & CBS_DROPDOWNLIST) != CBS_SIMPLE)
		{
			if ((style & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST) SetWindowLongPtr(window, GWL_STYLE, style | WS_CLIPCHILDREN);
			SetControlSubclass(window, ControlType::ComboBox);
		}

		COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
		if (GetComboBoxInfo(window, &info))
		{
			WindowsTheme::ApplyToControl(info.hwndList);
			SetWindowTheme(info.hwndList, state.enabled ? L"" : L"Explorer", state.enabled ? L"" : nullptr);
			if (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) SetControlSubclass(info.hwndList, ControlType::ListBox);
		}
	}
	else if (wcscmp(className, WC_EDIT) == 0)
	{
		const DWORD style = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
		if ((style & WS_BORDER) || (GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)) SetControlSubclass(window, ControlType::Edit);
	}
	else if (wcscmp(className, WC_LISTBOX) == 0)
	{
		SetWindowTheme(window, state.enabled ? L"" : L"Explorer", state.enabled ? L"" : nullptr);
		SetControlSubclass(window, ControlType::Edit);
	}
	else if (wcscmp(className, WC_LISTVIEW) == 0)
	{
		SetControlSubclass(window, ControlType::ListView);
		const COLORREF background = state.enabled ? g_Background : GetSysColor(COLOR_WINDOW);
		const COLORREF text = WindowsTheme::GetTextColor();
		ListView_SetBkColor(window, background);
		ListView_SetTextBkColor(window, background);
		ListView_SetTextColor(window, text);
		WindowsTheme::ApplyToTooltip(ListView_GetToolTips(window));
	}
	else if (wcscmp(className, WC_TREEVIEW) == 0)
	{
		TreeView_SetBkColor(window, state.enabled ? g_Background : GetSysColor(COLOR_WINDOW));
		TreeView_SetTextColor(window, WindowsTheme::GetTextColor());
		WindowsTheme::ApplyToTooltip(TreeView_GetToolTips(window));
	}
	else if (wcscmp(className, WC_HEADER) == 0)
	{
		SetControlSubclass(window, ControlType::Header);
	}
	else if (wcscmp(className, UPDOWN_CLASS) == 0)
	{
		SetControlSubclass(window, ControlType::Spinner);
	}
	else if (wcscmp(className, WC_STATIC) == 0)
	{
		const DWORD style = (DWORD)GetWindowLongPtr(window, GWL_STYLE) & SS_TYPEMASK;
		if (style == SS_ETCHEDHORZ || style == SS_ETCHEDVERT) SetControlSubclass(window, ControlType::Separator);
	}
	else if (wcscmp(className, WC_LINK) == 0)
	{
		LITEM item = { 0 };
		item.mask = LIF_ITEMINDEX | LIF_STATE;
		item.stateMask = LIS_DEFAULTCOLORS;
		item.state = state.enabled ? LIS_DEFAULTCOLORS : 0;
		while (SendMessage(window, LM_SETITEM, 0, (LPARAM)&item)) ++item.iLink;
	}

	return TRUE;
}

LRESULT CustomDraw(NMHDR* header)
{
	if (header->code != NM_CUSTOMDRAW) return -1;
	NMCUSTOMDRAW* draw = (NMCUSTOMDRAW*)header;

	WCHAR className[64];
	if (!GetClassName(header->hwndFrom, className, _countof(className))) return -1;

	if (wcscmp(className, WC_LINK) == 0)
	{
		if (draw->dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
		if (draw->dwDrawStage != CDDS_ITEMPREPAINT) return CDRF_DODEFAULT;
		LITEM item = { 0 };
		item.mask = LIF_ITEMINDEX | LIF_STATE;
		item.iLink = (int)draw->dwItemSpec;
		item.stateMask = LIS_ENABLED;
		const bool link = SendMessage(header->hwndFrom, LM_GETITEM, 0, (LPARAM)&item) != 0;
		const bool enabled = IsWindowEnabled(header->hwndFrom) && (!link || (item.state & LIS_ENABLED));
		SetTextColor(draw->hdc, !enabled ? g_DisabledText : link ? g_Accent : g_Text);
		return CDRF_NEWFONT;
	}

	if (wcscmp(className, WC_TREEVIEW) != 0) return -1;
	if (draw->dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
	if (draw->dwDrawStage != CDDS_ITEMPREPAINT) return CDRF_DODEFAULT;
	if (draw->uItemState & (CDIS_SELECTED | CDIS_DROPHILITED | CDIS_HOT)) return CDRF_DODEFAULT;

	NMTVCUSTOMDRAW* item = (NMTVCUSTOMDRAW*)draw;
	item->clrText = IsWindowEnabled(header->hwndFrom) ? g_Text : g_DisabledText;
	item->clrTextBk = g_Background;
	return CDRF_DODEFAULT;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	if (message == WM_NCDESTROY)
	{
		RemoveProp(window, g_MessageBoxProp);
		RemoveWindowSubclass(window, WindowProc, id);
	}
	else if (message == WM_SETTINGCHANGE)
	{
		WindowsTheme::HandleSettingChange(lParam);
	}
	else if (message == WM_PARENTNOTIFY && LOWORD(wParam) == WM_CREATE)
	{
		ApplyControl((HWND)lParam, 0);
	}

	if (WindowsTheme::IsUsingDarkMode())
	{
		if (message == WM_PAINT && GetProp(window, g_MessageBoxProp))
		{
			PAINTSTRUCT paint = { 0 };
			HDC dc = BeginPaint(window, &paint);
			PaintMessageBox(window, dc);
			EndPaint(window, &paint);
			return 0;
		}

		if (message == WM_ERASEBKGND)
		{
			if (GetProp(window, g_MessageBoxProp))
			{
				PaintMessageBox(window, (HDC)wParam);
				return TRUE;
			}

			RECT rect;
			GetClientRect(window, &rect);
			FillRect((HDC)wParam, &rect, GetSurfaceBrush(window));
			return TRUE;
		}

		if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC) return ControlColor(message, wParam, lParam);
		if (message == WM_NOTIFY)
		{
			const LRESULT result = CustomDraw((NMHDR*)lParam);
			if (result != -1) return result;
		}
	}

	return DefSubclassProc(window, message, wParam, lParam);
}

BOOL CALLBACK RefreshWindow(HWND window, LPARAM lParam)
{
	DWORD_PTR data;
	if (GetWindowSubclass(window, WindowProc, 0, &data))
	{
		WindowsTheme::ApplyToWindow(window);
		RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	}

	return TRUE;
}

BOOL CALLBACK RefreshTopLevelWindow(HWND window, LPARAM lParam)
{
	DWORD_PTR data;
	if (HasClass(window, TOOLTIPS_CLASS) && GetWindowSubclass(GetWindow(window, GW_OWNER), WindowProc, 0, &data)) WindowsTheme::ApplyToTooltip(window);
	RefreshWindow(window, lParam);
	EnumChildWindows(window, RefreshWindow, lParam);
	return TRUE;
}

HTHEME WINAPI OpenScrollbarTheme(HWND window, const WCHAR* className)
{
	if (WindowsTheme::IsUsingDarkMode() && wcscmp(className, L"ScrollBar") == 0)
	{
		window = nullptr;
		className = L"Explorer::ScrollBar";
	}

	return GetState().openNcThemeDataFunc(window, className);
}

void HookScrollbars()
{
	auto& state = GetState();
	if (!state.openNcThemeDataFunc) return;

	BYTE* module = (BYTE*)GetModuleHandle(L"comctl32.dll");
	if (!module) return;

	const auto* dos = (const IMAGE_DOS_HEADER*)module;
	const auto* nt = (const IMAGE_NT_HEADERS*)(module + dos->e_lfanew);
	const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
	if (!directory.VirtualAddress) return;

	const auto* imports = (const IMAGE_DELAYLOAD_DESCRIPTOR*)(module + directory.VirtualAddress);
	for (; imports->DllNameRVA; ++imports)
	{
		if (!imports->Attributes.RvaBased || _stricmp((const char*)(module + imports->DllNameRVA), "uxtheme.dll") != 0) continue;

		auto* names = (IMAGE_THUNK_DATA*)(module + imports->ImportNameTableRVA);
		auto* addresses = (IMAGE_THUNK_DATA*)(module + imports->ImportAddressTableRVA);
		for (; names->u1.Ordinal; ++names, ++addresses)
		{
			if (!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal) || IMAGE_ORDINAL(names->u1.Ordinal) != 49) continue;

			DWORD protection;
			if (VirtualProtect(&addresses->u1.Function, sizeof(addresses->u1.Function), PAGE_READWRITE, &protection))
			{
				state.scrollbarThunk = &addresses->u1.Function;
				state.originalScrollbarThunk = addresses->u1.Function;
				addresses->u1.Function = (ULONG_PTR)OpenScrollbarTheme;
				VirtualProtect(&addresses->u1.Function, sizeof(addresses->u1.Function), protection, &protection);
			}

			return;
		}
	}
}

bool UpdateMode()
{
	auto& state = GetState();

	HIGHCONTRAST contrast = { sizeof(HIGHCONTRAST) };
	SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);

	DWORD light = 1;
	DWORD size = sizeof(light);
	RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);

	const bool enabled = state.setPreferredAppModeFunc && light == 0 && !(contrast.dwFlags & HCF_HIGHCONTRASTON);
	if (enabled == state.enabled) return false;

	state.enabled = enabled;
	state.setPreferredAppModeFunc(enabled ? 2 : 0);

	if (state.flushMenuThemesFunc) state.flushMenuThemesFunc();

	return true;
}

BOOL CALLBACK ApplyMessageDialog(HWND window, LPARAM lParam)
{
	if (HasClass(window, L"#32770"))
	{
		WindowsTheme::ApplyToWindow(window);
		SetProp(window, g_MessageBoxProp, (HANDLE)1);
		RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
	}

	return TRUE;
}

LRESULT CALLBACK MessageBoxHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HCBT_ACTIVATE)
	{
		ApplyMessageDialog((HWND)wParam, 0);
		EnumChildWindows((HWND)wParam, ApplyMessageDialog, 0);
	}

	return CallNextHookEx(nullptr, code, wParam, lParam);
}

}  // namespace

namespace WindowsTheme {

void Initialize()
{
	static const bool initialized = []()
	{
		auto& state = GetState();
		if (GetPlatform().GetBuildNumber() >= 18362)
		{
			HMODULE theme = GetModuleHandle(L"uxtheme.dll");
			state.allowDarkModeForWindowFunc = (AllowDarkModeForWindowProc)GetProcAddress(theme, MAKEINTRESOURCEA(133));
			state.setPreferredAppModeFunc = (SetPreferredAppModeProc)GetProcAddress(theme, MAKEINTRESOURCEA(135));
			state.refreshImmersiveColorPolicyStateFunc = (RefreshColorPolicyProc)GetProcAddress(theme, MAKEINTRESOURCEA(104));
			state.flushMenuThemesFunc = (RefreshColorPolicyProc)GetProcAddress(theme, MAKEINTRESOURCEA(136));
			state.openNcThemeDataFunc = (OpenNcThemeDataProc)GetProcAddress(theme, MAKEINTRESOURCEA(49));
			HookScrollbars();
			UpdateMode();
		}

		return true;
	}();
	(void)initialized;
}

bool IsUsingDarkMode()
{
	return GetState().enabled;
}

void ApplyToControl(HWND window)
{
	if (!window) return;

	auto& state = GetState();
	if (state.allowDarkModeForWindowFunc) state.allowDarkModeForWindowFunc(window, state.enabled);

	SetWindowTheme(window, L"Explorer", nullptr);
}

void ApplyToTooltip(HWND window)
{
	if (window) SetWindowTheme(window, IsUsingDarkMode() ? L"DarkMode_Explorer" : L"Explorer", nullptr);
}

void EnableDialogTexture(HWND window, bool enable)
{
	EnableThemeDialogTexture(window, enable && !IsUsingDarkMode() ? ETDT_ENABLETAB : ETDT_DISABLE);
}

void ApplyToWindow(HWND window)
{
	if (!window) return;

	Initialize();
	ApplyToControl(window);

	const BOOL enabled = IsUsingDarkMode();
	DwmSetWindowAttribute(window, GetPlatform().GetBuildNumber() >= 19041 ? 20 : 19, &enabled, sizeof(enabled));
	SetWindowSubclass(window, WindowProc, 0, 0);
	EnableDialogTexture(window, (GetWindowLongPtr(window, GWL_STYLE) & WS_CHILD) != 0);
	EnumChildWindows(window, ApplyControl, 0);
}

void HandleSettingChange(LPARAM lParam)
{
	auto& state = GetState();
	const bool colorScheme = lParam && _wcsicmp((const WCHAR*)lParam, L"ImmersiveColorSet") == 0;
	if (colorScheme && state.refreshImmersiveColorPolicyStateFunc) state.refreshImmersiveColorPolicyStateFunc();
	if (UpdateMode() || colorScheme) EnumThreadWindows(GetCurrentThreadId(), RefreshTopLevelWindow, 0);
}

int ShowMessageBox(HWND parent, const WCHAR* text, const WCHAR* caption, UINT type)
{
	Initialize();

	HHOOK hook = IsUsingDarkMode() ? SetWindowsHookEx(WH_CBT, MessageBoxHook, nullptr, GetCurrentThreadId()) : nullptr;
	const int result = MessageBox(parent, text, caption, type);
	if (hook) UnhookWindowsHookEx(hook);

	return result;
}

COLORREF GetTextColor()
{
	return IsUsingDarkMode() ? g_Text : GetSysColor(COLOR_WINDOWTEXT);
}

COLORREF GetDisabledTextColor()
{
	return IsUsingDarkMode() ? g_DisabledText : GetSysColor(COLOR_GRAYTEXT);
}

COLORREF GetBackgroundColor()
{
	return IsUsingDarkMode() ? g_PageBackground : GetSysColor(COLOR_BTNFACE);
}

COLORREF GetHotBackgroundColor()
{
	return IsUsingDarkMode() ? g_HotBackground : GetSysColor(COLOR_INFOBK);
}

HBRUSH GetBackgroundBrush()
{
	return IsUsingDarkMode() ? GetState().pageBackground : GetSysColorBrush(COLOR_BTNFACE);
}

HBRUSH GetHotBackgroundBrush()
{
	return IsUsingDarkMode() ? GetState().hotBackground : GetSysColorBrush(COLOR_INFOBK);
}

}  // namespace WindowsTheme
