/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureInputText.h"
#include "ConfigParser.h"
#include "Rainmeter.h"
#include "Skin.h"

#include <cwctype>

namespace
{
const WCHAR* c_InputWindowClass = L"RainmeterInputTextWindow";
const WCHAR* c_TempKey = L"__InputText_ParseInline_TemporaryKey__";

COLORREF ToColorRef(const D2D1_COLOR_F& color)
{
	return RGB(
		(BYTE)(color.r * 255.0f),
		(BYTE)(color.g * 255.0f),
		(BYTE)(color.b * 255.0f));
}

BYTE ToAlpha(const D2D1_COLOR_F& color)
{
	return (BYTE)(color.a * 255.0f);
}

bool GetOption(
	const MeasureInputText::Options& options,
	const MeasureInputText::Options& overrides,
	const WCHAR* option,
	std::wstring& value)
{
	auto iter = overrides.find(option);
	if (iter != overrides.end())
	{
		value = iter->second;
		return true;
	}

	iter = options.find(option);
	if (iter != options.end())
	{
		value = iter->second;
		return true;
	}

	return false;
}

bool ParseInt(const std::wstring& str, int& value)
{
	const WCHAR* start = str.c_str();
	WCHAR* end = nullptr;
	errno = 0;
	long result = wcstol(start, &end, 10);
	if (start == end || errno == ERANGE || *end != L'\0')
	{
		return false;
	}

	value = (int)result;
	return true;
}

class InputWindow
{
public:
	InputWindow(Skin* skin, const MeasureInputText::Options& options, const MeasureInputText::Options& overrides) :
		m_Skin(skin),
		m_Window(),
		m_Edit(),
		m_EditProc(),
		m_Font(),
		m_BackBrush(),
		m_Done(false),
		m_Accepted(false),
		m_FocusDismiss(true),
		m_Numeric(false),
		m_Password(false),
		m_TopMost(false),
		m_X(),
		m_Y(),
		m_W(200),
		m_H(22),
		m_TextColor(RGB(0, 0, 0)),
		m_BackColor(RGB(255, 255, 255)),
		m_Alpha(255),
		m_AlignStyle(),
		m_FontStyle(FW_NORMAL),
		m_FontSize(8.25f),
		m_InputLimit()
	{
		Configure(options, overrides);
	}

	~InputWindow()
	{
		if (m_Window)
		{
			DestroyWindow(m_Window);
			m_Window = nullptr;
		}

		if (m_BackBrush)
		{
			DeleteObject(m_BackBrush);
			m_BackBrush = nullptr;
		}

		if (m_Font)
		{
			DeleteObject(m_Font);
			m_Font = nullptr;
		}
	}

	bool Show(std::wstring& result)
	{
		RegisterWindowClass();

		DWORD exStyle = WS_EX_TOOLWINDOW;
		if (m_TopMost)
		{
			exStyle |= WS_EX_TOPMOST;
		}
		if (m_Alpha < 255)
		{
			exStyle |= WS_EX_LAYERED;
		}

		HWND owner = m_Skin->GetWindow();
		m_Window = CreateWindowEx(
			exStyle,
			c_InputWindowClass,
			L"",
			WS_POPUP,
			m_X,
			m_Y,
			m_W,
			m_H,
			owner,
			nullptr,
			GetModuleHandle(nullptr),
			this);

		if (!m_Window)
		{
			return false;
		}

		if (m_Alpha < 255)
		{
			SetLayeredWindowAttributes(m_Window, 0, m_Alpha, LWA_ALPHA);
		}

		DWORD editStyle = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | m_AlignStyle;
		if (m_Password)
		{
			editStyle |= ES_PASSWORD;
		}
		else
		{
			editStyle |= ES_MULTILINE;
		}

		m_Edit = CreateWindowEx(
			0,
			L"EDIT",
			m_DefaultValue.c_str(),
			editStyle,
			0,
			0,
			m_W,
			m_H,
			m_Window,
			nullptr,
			GetModuleHandle(nullptr),
			nullptr);

		if (!m_Edit)
		{
			return false;
		}

		if (m_InputLimit > 0)
		{
			SendMessage(m_Edit, EM_LIMITTEXT, m_InputLimit, 0);
		}

		CreateInputFont();
		if (m_Font)
		{
			SendMessage(m_Edit, WM_SETFONT, (WPARAM)m_Font, TRUE);
		}

		SetWindowLongPtr(m_Edit, GWLP_USERDATA, (LONG_PTR)this);
		m_EditProc = (WNDPROC)SetWindowLongPtr(m_Edit, GWLP_WNDPROC, (LONG_PTR)EditProc);

		ShowWindow(m_Window, SW_SHOWNORMAL);
		BringWindowToTop(m_Window);
		SetFocus(m_Edit);
		if (!m_DefaultValue.empty())
		{
			SendMessage(m_Edit, EM_SETSEL, 0, -1);
			SendMessage(m_Edit, EM_SCROLLCARET, 0, 0);
		}

		MSG msg = { 0 };
		BOOL ret = FALSE;
		while (!m_Done && (ret = GetMessage(&msg, nullptr, 0, 0)) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (ret == 0)
		{
			PostQuitMessage((int)msg.wParam);
		}

		if (m_Accepted)
		{
			result = m_Result;
		}

		return m_Accepted;
	}

private:
	void Configure(const MeasureInputText::Options& options, const MeasureInputText::Options& overrides)
	{
		HWND owner = m_Skin->GetWindow();
		RECT ownerRect = { 0 };
		GetWindowRect(owner, &ownerRect);

		m_X = ownerRect.left;
		m_Y = ownerRect.top;

		std::wstring value;
		if (GetOption(options, overrides, L"X", value))
		{
			m_X = ownerRect.left + ConfigParser::ParseInt(value.c_str(), 0);
		}
		if (GetOption(options, overrides, L"Y", value))
		{
			m_Y = ownerRect.top + ConfigParser::ParseInt(value.c_str(), 0);
		}
		if (GetOption(options, overrides, L"W", value))
		{
			m_W = ConfigParser::ParseInt(value.c_str(), m_W);
			if (m_W < 1) m_W = 1;
		}
		if (GetOption(options, overrides, L"H", value))
		{
			m_H = ConfigParser::ParseInt(value.c_str(), m_H);
			if (m_H < 1) m_H = 1;
		}
		if (GetOption(options, overrides, L"DefaultValue", value))
		{
			m_DefaultValue = value;
		}
		if (GetOption(options, overrides, L"FocusDismiss", value))
		{
			StringUtil::Trim(value);
			m_FocusDismiss = value == L"1";
		}
		if (GetOption(options, overrides, L"Password", value))
		{
			StringUtil::Trim(value);
			m_Password = value == L"1";
		}
		if (GetOption(options, overrides, L"InputNumber", value))
		{
			StringUtil::Trim(value);
			m_Numeric = value == L"1";
		}
		if (GetOption(options, overrides, L"InputLimit", value))
		{
			int limit = ConfigParser::ParseInt(value.c_str(), 0);
			m_InputLimit = (UINT)(limit > 0 ? limit : 0);
		}

		m_X = ownerRect.left + m_Skin->ScaleToDevicePixels(m_X - ownerRect.left);
		m_Y = ownerRect.top + m_Skin->ScaleToDevicePixels(m_Y - ownerRect.top);
		m_W = m_Skin->ScaleToDevicePixels(m_W);
		m_H = m_Skin->ScaleToDevicePixels(m_H);

		if (GetOption(options, overrides, L"StringAlign", value))
		{
			StringUtil::Trim(value);
			StringUtil::ToUpperCase(value);
			if (value == L"CENTER")
			{
				m_AlignStyle = ES_CENTER;
			}
			else if (value == L"RIGHT")
			{
				m_AlignStyle = ES_RIGHT;
			}
		}
		if (GetOption(options, overrides, L"StringStyle", value))
		{
			StringUtil::Trim(value);
			StringUtil::ToUpperCase(value);
			if (value == L"BOLD")
			{
				m_FontStyle = FW_BOLD;
			}
			else if (value == L"ITALIC")
			{
				m_Italic = true;
			}
			else if (value == L"BOLDITALIC")
			{
				m_FontStyle = FW_BOLD;
				m_Italic = true;
			}
		}
		if (GetOption(options, overrides, L"FontFace", value))
		{
			m_FontFace = value;
		}
		if (GetOption(options, overrides, L"FontSize", value))
		{
			m_FontSize = (float)ConfigParser::ParseDouble(value.c_str(), m_FontSize);
		}
		if (GetOption(options, overrides, L"FontColor", value))
		{
			m_TextColor = ToColorRef(ConfigParser::ParseColor(value.c_str()));
		}
		if (GetOption(options, overrides, L"SolidColor", value))
		{
			D2D1_COLOR_F color = ConfigParser::ParseColor(value.c_str());
			m_BackColor = ToColorRef(color);
			m_Alpha = ToAlpha(color);
		}
		if (GetOption(options, overrides, L"TopMost", value))
		{
			StringUtil::Trim(value);
			if (value == L"1")
			{
				m_TopMost = true;
			}
			else if (value != L"0")
			{
				m_TopMost = (GetWindowLongPtr(owner, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
			}
		}
		else
		{
			m_TopMost = (GetWindowLongPtr(owner, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
		}

		m_BackBrush = CreateSolidBrush(m_BackColor);
	}

	void CreateInputFont()
	{
		LOGFONT lf = { 0 };
		HFONT defaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (defaultFont)
		{
			GetObject(defaultFont, sizeof(LOGFONT), &lf);
		}

		if (!m_FontFace.empty())
		{
			wcsncpy_s(lf.lfFaceName, m_FontFace.c_str(), _TRUNCATE);
		}

		HDC hdc = GetDC(m_Edit);
		lf.lfHeight = -MulDiv((int)(m_FontSize * 10.0f), GetDeviceCaps(hdc, LOGPIXELSY), 720);
		ReleaseDC(m_Edit, hdc);

		lf.lfWeight = m_FontStyle;
		lf.lfItalic = m_Italic ? TRUE : FALSE;
		m_Font = CreateFontIndirect(&lf);
	}

	void Accept()
	{
		if (m_Done)
		{
			return;
		}

		int len = GetWindowTextLength(m_Edit);
		std::wstring text(len + 1, L'\0');
		if (len > 0)
		{
			GetWindowText(m_Edit, &text[0], len + 1);
		}
		text.resize(len);

		StringUtil::Trim(text);
		m_Result = text;
		m_Accepted = true;
		m_Done = true;
		if (m_Window)
		{
			HWND window = m_Window;
			m_Window = nullptr;
			DestroyWindow(window);
		}
	}

	void Cancel()
	{
		if (m_Done)
		{
			return;
		}

		m_Accepted = false;
		m_Done = true;
		if (m_Window)
		{
			HWND window = m_Window;
			m_Window = nullptr;
			DestroyWindow(window);
		}
	}

	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
			if (!m_Done && m_FocusDismiss && LOWORD(wParam) == WA_INACTIVE)
			{
				Cancel();
				return 0;
			}
			break;

		case WM_CTLCOLOREDIT:
			SetTextColor((HDC)wParam, m_TextColor);
			SetBkColor((HDC)wParam, m_BackColor);
			return (LRESULT)m_BackBrush;

		case WM_CLOSE:
			Cancel();
			return 0;

		case WM_DESTROY:
			m_Done = true;
			return 0;
		}

		return DefWindowProc(m_Window, msg, wParam, lParam);
	}

	LRESULT HandleEditMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_GETDLGCODE:
			{
				LRESULT code = CallWindowProc(m_EditProc, m_Edit, msg, wParam, lParam);
				MSG* keyMsg = (MSG*)lParam;
				if (keyMsg && keyMsg->message == WM_KEYDOWN &&
					(keyMsg->wParam == VK_RETURN || keyMsg->wParam == VK_ESCAPE))
				{
					return code | DLGC_WANTMESSAGE;
				}
				return code;
			}

		case WM_KEYDOWN:
			if (wParam == VK_RETURN)
			{
				Accept();
				return 0;
			}
			if (wParam == VK_ESCAPE)
			{
				Cancel();
				return 0;
			}
			break;

		case WM_CHAR:
			if (wParam == L'\r')
			{
				Accept();
				return 0;
			}
			if (wParam == VK_ESCAPE)
			{
				Cancel();
				return 0;
			}
			if (m_Numeric && !IsAllowedNumericChar((WCHAR)wParam))
			{
				return 0;
			}
			break;
		}

		return CallWindowProc(m_EditProc, m_Edit, msg, wParam, lParam);
	}

	bool IsAllowedNumericChar(WCHAR c)
	{
		if (iswcntrl(c) || iswdigit(c))
		{
			return true;
		}

		if (c == L'.')
		{
			int len = GetWindowTextLength(m_Edit);
			std::wstring text(len + 1, L'\0');
			if (len > 0)
			{
				GetWindowText(m_Edit, &text[0], len + 1);
			}
			text.resize(len);
			return text.find(L'.') == std::wstring::npos;
		}

		if (c == L'-')
		{
			DWORD selection = (DWORD)SendMessage(m_Edit, EM_GETSEL, 0, 0);
			return LOWORD(selection) == 0;
		}

		return false;
	}

	static void RegisterWindowClass()
	{
		static ATOM atom = 0;
		if (atom)
		{
			return;
		}

		WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
		wc.lpfnWndProc = WndProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursor(nullptr, IDC_IBEAM);
		wc.lpszClassName = c_InputWindowClass;
		atom = RegisterClassEx(&wc);
	}

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		InputWindow* self = (InputWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
			self = (InputWindow*)cs->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
			self->m_Window = hwnd;
		}

		return self ? self->HandleMessage(msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
	}

	static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		InputWindow* self = (InputWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		return self ? self->HandleEditMessage(msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
	}

	Skin* m_Skin;
	HWND m_Window;
	HWND m_Edit;
	WNDPROC m_EditProc;
	HFONT m_Font;
	HBRUSH m_BackBrush;
	bool m_Done;
	bool m_Accepted;
	bool m_FocusDismiss;
	bool m_Numeric;
	bool m_Password;
	bool m_TopMost;
	bool m_Italic = false;
	int m_X;
	int m_Y;
	int m_W;
	int m_H;
	COLORREF m_TextColor;
	COLORREF m_BackColor;
	BYTE m_Alpha;
	DWORD m_AlignStyle;
	int m_FontStyle;
	float m_FontSize;
	UINT m_InputLimit;
	std::wstring m_DefaultValue;
	std::wstring m_FontFace;
	std::wstring m_Result;
};
}

MeasureInputText::MeasureInputText(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Executing(false),
	m_StringValue()
{
}

MeasureInputText::~MeasureInputText()
{
}

void MeasureInputText::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);
}

void MeasureInputText::UpdateValue()
{
	m_Value = 0.0;
}

const WCHAR* MeasureInputText::GetStringValue()
{
	return CheckSubstitute(m_StringValue.c_str());
}

void MeasureInputText::Command(const std::wstring& command)
{
	if (m_Executing)
	{
		return;
	}

	m_Executing = true;

	CommandParam param;
	param.command = command;
	StringUtil::Trim(param.command);

	if (ReadCommandOptions(param))
	{
		switch (param.type)
		{
		case CommandType::SetVariable:
			{
				std::wstring input;
				if (GetUserInput(param.options, Options(), input))
				{
					std::wstring bang = L"!SetVariable ";
					bang += param.command;
					bang += L" \"";
					bang += input;
					bang += L"\"";
					ExecuteRainmeterCommand(bang);
				}
				else
				{
					ExecuteDismissAction(param.dismissAction);
				}
			}
			break;

		case CommandType::ExecuteBatch:
			{
				for (size_t i = 0; i < param.commands.size(); ++i)
				{
					if (!ExecuteLine(param.commands[i], param.options, param.overrideOptions[i]))
					{
						ExecuteDismissAction(param.dismissAction);
						break;
					}
				}
			}
			break;
		}
	}

	m_Executing = false;
}

bool MeasureInputText::ReadCommandOptions(CommandParam& param)
{
	ReadOption(L"DefaultValue", param.options);
	ReadOption(L"X", param.options, true);
	ReadOption(L"Y", param.options, true);
	ReadOption(L"W", param.options, true);
	ReadOption(L"H", param.options, true);
	ReadOption(L"StringStyle", param.options);
	ReadOption(L"StringAlign", param.options);
	ReadOption(L"FocusDismiss", param.options);
	ReadOption(L"FontColor", param.options);
	ReadOption(L"FontFace", param.options);
	ReadOption(L"FontSize", param.options, true);
	ReadOption(L"SolidColor", param.options);
	ReadOption(L"Password", param.options);
	ReadOption(L"TopMost", param.options);
	ReadOption(L"InputLimit", param.options, true);
	ReadOption(L"InputNumber", param.options);

	param.dismissAction = m_Skin->GetParser().ReadString(m_Name.c_str(), L"OnDismissAction", L"", false);

	if (param.command.find(L' ') == std::wstring::npos)
	{
		param.type = CommandType::SetVariable;
		return true;
	}

	std::vector<std::wstring> parts = ConfigParser::Tokenize(param.command, L" ");
	if (!parts.empty() && _wcsicmp(parts[0].c_str(), L"ExecuteBatch") == 0)
	{
		param.type = CommandType::ExecuteBatch;

		int min = 1;
		int max = 1000000000;
		if (parts.size() > 1 && _wcsicmp(parts[1].c_str(), L"All") != 0)
		{
			bool validRange = true;
			const size_t dash = parts[1].find(L'-');
			if (dash != std::wstring::npos)
			{
				std::wstring minPart = parts[1].substr(0, dash);
				std::wstring maxPart = parts[1].substr(dash + 1);
				StringUtil::Trim(minPart);
				StringUtil::Trim(maxPart);
				if (!minPart.empty() && !maxPart.empty())
				{
					validRange = ParseInt(minPart, min) && ParseInt(maxPart, max);
				}
				else
				{
					validRange = false;
				}
			}
			else
			{
				validRange = ParseInt(parts[1], min);
				max = min;
			}

			if (!validRange)
			{
				min = 1;
				max = 1000000000;
			}
			else if (min < 1 || max < min)
			{
				min = 1;
				max = 1000000000;
			}
		}

		for (int i = min; i <= max; ++i)
		{
			WCHAR key[32];
			_snwprintf_s(key, _TRUNCATE, L"Command%i", i);

			std::wstring line = m_Skin->GetParser().ReadString(m_Name.c_str(), key, L"", false);
			if (line.empty())
			{
				break;
			}

			Options overrides;
			line = ScanAndReplace(line, L"DefaultValue", overrides);
			line = ScanAndReplace(line, L"X", overrides, true);
			line = ScanAndReplace(line, L"Y", overrides, true);
			line = ScanAndReplace(line, L"W", overrides, true);
			line = ScanAndReplace(line, L"H", overrides, true);
			line = ScanAndReplace(line, L"StringStyle", overrides);
			line = ScanAndReplace(line, L"StringAlign", overrides);
			line = ScanAndReplace(line, L"FocusDismiss", overrides);
			line = ScanAndReplace(line, L"FontColor", overrides);
			line = ScanAndReplace(line, L"FontFace", overrides);
			line = ScanAndReplace(line, L"FontSize", overrides, true);
			line = ScanAndReplace(line, L"SolidColor", overrides);
			line = ScanAndReplace(line, L"Password", overrides);
			line = ScanAndReplace(line, L"TopMost", overrides);
			line = ScanAndReplace(line, L"InputLimit", overrides, true);
			line = ScanAndReplace(line, L"InputNumber", overrides);

			param.overrideOptions.push_back(overrides);
			param.commands.push_back(line);
		}

		return !param.commands.empty();
	}

	param.type = CommandType::Unknown;
	if (GetRainmeter().GetDebug() && !parts.empty())
	{
		LogDebugF(this, L"InputText: Received command \"%s\", left unhandled", parts[0].c_str());
	}

	return false;
}

void MeasureInputText::ReadOption(const WCHAR* option, Options& options, bool formula)
{
	ConfigParser& parser = m_Skin->GetParser();
	std::wstring value = parser.ReadString(m_Name.c_str(), option, L"");
	if (!value.empty())
	{
		if (formula && value[0] == L'(')
		{
			WCHAR buffer[64];
			_snwprintf_s(buffer, _TRUNCATE, L"%i", parser.ReadInt(m_Name.c_str(), option, 0));
			value = buffer;
		}
		options[option] = value;
	}
}

bool MeasureInputText::ExecuteLine(std::wstring line, const Options& options, const Options& overrides)
{
	if (StringUtil::CaseInsensitiveFind(line, std::wstring(L"$UserInput$")) != std::wstring::npos)
	{
		std::wstring input;
		if (!GetUserInput(options, overrides, input))
		{
			return false;
		}

		ReplaceInsensitive(line, L"$UserInput$", input);
	}

	ExecuteRainmeterCommand(line);
	return true;
}

bool MeasureInputText::GetUserInput(const Options& options, const Options& overrides, std::wstring& result)
{
	InputWindow input(m_Skin, options, overrides);
	if (input.Show(result))
	{
		m_StringValue = result;
		return true;
	}

	return false;
}

void MeasureInputText::ExecuteDismissAction(const std::wstring& action)
{
	if (!action.empty())
	{
		ExecuteRainmeterCommand(action);
	}
}

void MeasureInputText::ExecuteRainmeterCommand(const std::wstring& command)
{
	if (!command.empty())
	{
		GetRainmeter().ExecuteCommand(command.c_str(), m_Skin);
	}
}

std::wstring MeasureInputText::ParseInlineOption(const std::wstring& data, bool formula)
{
	ConfigParser& parser = m_Skin->GetParser();
	parser.SetValue(m_Name, c_TempKey, data);

	std::wstring value;
	if (formula)
	{
		WCHAR buffer[64];
		_snwprintf_s(buffer, _TRUNCATE, L"%i", parser.ReadInt(m_Name.c_str(), c_TempKey, 0));
		value = buffer;
	}
	else
	{
		value = parser.ReadString(m_Name.c_str(), c_TempKey, L"");
	}

	parser.DeleteValue(m_Name, c_TempKey);
	return value;
}

std::wstring MeasureInputText::ScanAndReplace(std::wstring line, const WCHAR* tagName, Options& overrides, bool formula)
{
	int loc = FindTag(line, tagName);
	if (loc >= 0)
	{
		std::wstring tagData = ReadTagData(line, tagName, loc);
		std::wstring data = tagData;
		if (!data.empty() && data[0] == L'"' && data.size() >= 2)
		{
			data = data.substr(1, data.size() - 2);
		}

		if (!data.empty())
		{
			int index = -1;
			if (formula && data[0] == L'(')
			{
				data = ParseInlineOption(data, true);
			}
			else if ((index = (int)data.find(L'[')) >= 0 && data.find(L']', index) != std::wstring::npos)
			{
				data = ParseInlineOption(data, false);
			}
		}

		overrides[tagName] = data;
		line.replace(loc - 1, 1 + wcslen(tagName) + 1 + tagData.length(), L"");
	}

	return line;
}

int MeasureInputText::FindTag(const std::wstring& line, const WCHAR* tagName)
{
	if (!line.empty() && tagName && *tagName)
	{
		std::wstring tag = L" ";
		tag += tagName;
		tag += L"=";

		size_t loc = StringUtil::CaseInsensitiveFind(line, tag);
		if (loc != std::wstring::npos)
		{
			return (int)loc + 1;
		}
	}

	return -1;
}

std::wstring MeasureInputText::ReadTagData(const std::wstring& line, const WCHAR* tagName, int start)
{
	if (start < 0)
	{
		return L"";
	}

	start += (int)wcslen(tagName) + 1;

	std::wstring data;
	bool inQuote = false;
	for (size_t i = start; i < line.length(); ++i)
	{
		if (i == (size_t)start && line[i] == L'"')
		{
			inQuote = true;
			continue;
		}

		if (line[i] == L'"')
		{
			break;
		}

		if (!inQuote && iswspace(line[i]))
		{
			break;
		}

		data += line[i];
	}

	if (inQuote)
	{
		data.insert(data.begin(), L'"');
		data += L'"';
	}

	return data;
}

void MeasureInputText::ReplaceInsensitive(std::wstring& str, const std::wstring& find, const std::wstring& replace)
{
	size_t pos = StringUtil::CaseInsensitiveFind(str, find);
	if (pos != std::wstring::npos)
	{
		str.replace(pos, find.length(), replace);
	}
}
