// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureInputText.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"
#include <Commctrl.h>

namespace {

const WCHAR* c_ClassName = L"RainmeterInputText";
const WCHAR* c_UserInputToken = L"$UserInput$";

constexpr UINT WM_INPUTTEXT_CLOSE = WM_APP + 0;
constexpr UINT WM_INPUTTEXT_MENUDONE = WM_APP + 1;

// Whitespace as a skin file can write it. Nothing here parses prose, so the Unicode spaces are not
// worth the call it would take to recognise them.
bool IsSpace(WCHAR ch)
{
	return ch == L' ' || ch == L'\t';
}

std::wstring Trim(const std::wstring& text)
{
	size_t start = 0;
	while (start < text.size() && IsSpace(text[start])) ++start;

	size_t end = text.size();
	while (end > start && IsSpace(text[end - 1])) --end;

	return text.substr(start, end - start);
}

// A skin unit in pixels. Rounded away from zero, so that a box a unit wide is never scaled away to
// nothing.
int ScaleCoordinate(int value, float scale)
{
	const double scaled = value * (double)scale;
	return (int)((value >= 0) ? ceil(scaled) : floor(scaled));
}

// A Rainmeter color into what GDI takes: the color, and the alpha that goes to the window rather
// than into the brush.
void SplitColor(const D2D1_COLOR_F& color, COLORREF& result, BYTE& alpha)
{
	result = RGB((BYTE)(color.r * 255.0f + 0.5f), (BYTE)(color.g * 255.0f + 0.5f),
		(BYTE)(color.b * 255.0f + 0.5f));
	alpha = (BYTE)(color.a * 255.0f + 0.5f);
}

// Everything a box can be built from. The name is how a skin writes it, and the value is what the
// switch below dispatches on.
enum class Option : uint8_t
{
	DefaultValue,
	X,
	Y,
	W,
	H,
	FontFace,
	FontSize,
	FontColor,
	SolidColor,
	StringStyle,
	StringAlign,
	InputLimit,
	InputNumber,
	Password,
	FocusDismiss,
	TopMost
};

struct OptionName
{
	const WCHAR* name;
	Option option;
};

// Both places an option can be written are read from this one list: the measure's own section, and
// the settings put among a Command line.
//
// DefaultValue leads it for a reason. A value quoted onto a Command line may hold anything, the
// name of another setting included, and taking this one off the line before the rest are looked for
// is what keeps a setting written inside it from being read as one in its own right.
const OptionName c_Options[] =
{
	{ L"DefaultValue", Option::DefaultValue },
	{ L"X",            Option::X },
	{ L"Y",            Option::Y },
	{ L"W",            Option::W },
	{ L"H",            Option::H },
	{ L"FontFace",     Option::FontFace },
	{ L"FontSize",     Option::FontSize },
	{ L"FontColor",    Option::FontColor },
	{ L"SolidColor",   Option::SolidColor },
	{ L"StringStyle",  Option::StringStyle },
	{ L"StringAlign",  Option::StringAlign },
	{ L"InputLimit",   Option::InputLimit },
	{ L"InputNumber",  Option::InputNumber },
	{ L"Password",     Option::Password },
	{ L"FocusDismiss", Option::FocusDismiss },
	{ L"TopMost",      Option::TopMost }
};

// Reads one option into |options|, wherever it was written. Both callers come through here, so that
// "W=200" in a section and "W=200" among a Command line cannot come to mean two different things.
//
// A number that cannot be read falls to zero rather than to what the option already held, which is
// what the box has always done with one: a size written as a formula Rainmeter cannot parse
// collapses the box rather than quietly drawing it at some other size. The formula error reaches
// the log either way, but a box that comes up wrong is what sends anyone looking for it.
void ApplyOption(InputTextOptions& options, ConfigParser& parser, Option option, const std::wstring& value)
{
	switch (option)
	{
	case Option::DefaultValue:
		options.text = value;
		break;

	case Option::X:
		options.x = parser.ParseInt(value, 0);
		break;

	case Option::Y:
		options.y = parser.ParseInt(value, 0);
		break;

	case Option::W:
		options.w = parser.ParseInt(value, 0);
		break;

	case Option::H:
		options.h = parser.ParseInt(value, 0);
		break;

	case Option::FontFace:
		options.fontFace = value;
		break;

	case Option::FontSize:
		{
			// The one number that keeps what it had: a font of no size is one the box cannot draw
			// with, and asking for one has always left the font it already had alone.
			const double size = parser.ParseDouble(value, 0.0);
			if (size > 0.0) options.fontSize = size;
		}
		break;

	case Option::FontColor:
		{
			// The alpha of a font color has nowhere to go: the text is drawn at whatever opacity
			// the box as a whole is drawn at.
			BYTE ignored = 255;
			SplitColor(parser.ParseColor(value), options.fontColor, ignored);
		}
		break;

	case Option::SolidColor:
		// This alpha does have somewhere to go: it dims the whole box, as it always has, rather
		// than only what is behind the text.
		SplitColor(parser.ParseColor(value), options.backColor, options.opacity);
		break;

	case Option::StringStyle:
		{
			const std::wstring style = Trim(value);
			const bool boldItalic = _wcsicmp(style.c_str(), L"BOLDITALIC") == 0;
			options.bold = boldItalic || _wcsicmp(style.c_str(), L"BOLD") == 0;
			options.italic = boldItalic || _wcsicmp(style.c_str(), L"ITALIC") == 0;
		}
		break;

	case Option::StringAlign:
		{
			const std::wstring align = Trim(value);
			if (_wcsicmp(align.c_str(), L"CENTER") == 0) options.align = ES_CENTER;
			else if (_wcsicmp(align.c_str(), L"RIGHT") == 0) options.align = ES_RIGHT;
			else options.align = ES_LEFT;
		}
		break;

	case Option::InputLimit:
		options.maxLength = parser.ParseInt(value, 0);
		break;

	case Option::InputNumber:
		options.numeric = parser.ParseInt(value, 0) != 0;
		break;

	case Option::Password:
		options.password = parser.ParseInt(value, 0) != 0;
		break;

	case Option::FocusDismiss:
		options.focusDismiss = parser.ParseInt(value, 0) != 0;
		break;

	case Option::TopMost:
		{
			// Three states rather than two, so this one is read as written rather than as a
			// number: anything that is neither is AUTO, and AUTO is the skin's own answer.
			const std::wstring topMost = Trim(value);
			if (topMost == L"1") options.topMost = true;
			else if (topMost == L"0") options.topMost = false;
			else options.topMost.reset();
		}
		break;
	}
}

// Where " Name=" starts in |line|, or npos. The leading space is what keeps a setting from being
// found inside the bang it was written among.
size_t TagLoc(const std::wstring& line, const WCHAR* name)
{
	std::wstring needle = L" ";
	needle += name;
	needle += L'=';

	const size_t loc = StringUtil::CaseInsensitiveFind(line, needle);
	return (loc == std::wstring::npos) ? std::wstring::npos : loc + 1;
}

// The value written at |loc|, which runs to the next space, or to the closing quote where it
// opened with one. The quotes are left on: the caller has to know how much of the line the setting
// took up in order to cut it back out.
std::wstring TagData(const std::wstring& line, const WCHAR* name, size_t loc)
{
	size_t i = loc + wcslen(name) + 1;

	bool quoted = false;
	if (i < line.size() && line[i] == L'"')
	{
		quoted = true;
		++i;
	}

	std::wstring data;
	for (; i < line.size(); ++i)
	{
		const WCHAR ch = line[i];
		if (ch == L'"') break;
		if (!quoted && IsSpace(ch)) break;

		data += ch;
	}

	if (quoted) return L'"' + data + L'"';
	return data;
}

// Takes the settings written among a Command line off it and into |options|, and hands back the
// bang that is left standing.
std::wstring ScanOverrides(ConfigParser& parser, std::wstring line, InputTextOptions& options)
{
	for (const OptionName& entry : c_Options)
	{
		const size_t loc = TagLoc(line, entry.name);
		if (loc == std::wstring::npos) continue;

		const std::wstring tag = TagData(line, entry.name, loc);

		// Out goes " Name=Value", the space that found it included: what is left is the bang, and
		// the settings are no part of what it does.
		line.erase(loc - 1, 1 + wcslen(entry.name) + 1 + tag.size());

		std::wstring value = tag;
		if (value.size() >= 2 && value.front() == L'"') value = value.substr(1, value.size() - 2);

		// The line itself is read with its section variables left alone, so that the bang keeps
		// its own until it runs. A setting is read here and now, and wants them resolved.
		parser.ReplaceMeasures(value);

		if (!value.empty()) ApplyOption(options, parser, entry.option, value);
	}

	return line;
}

}  // namespace

class MeasureInputText::InputBox : public std::enable_shared_from_this<InputBox>
{
public:
	explicit InputBox(const std::shared_ptr<MeasureInputText*>& measure) : m_Measure(measure) {}
	~InputBox();

	bool Open(const InputTextOptions& options, HWND skinWindow, float scale);
	void Close(bool submitted);
	void Abort();

private:
	static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK EditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR id, DWORD_PTR data);

	static std::wstring GetControlText(HWND control);

	bool AcceptsChar(WCHAR ch) const;

	void Complete();
	std::optional<std::wstring> FinishClose();
	void RestoreSkinEnabledState();

	std::shared_ptr<MeasureInputText*> m_Measure;
	InputTextOptions m_Options;
	HWND m_SkinWindow = nullptr;
	HWND m_Window = nullptr;
	HWND m_Edit = nullptr;
	HFONT m_Font = nullptr;
	HBRUSH m_BackBrush = nullptr;
	bool m_Submitted = false;
	bool m_SkinDisabled = false;

	// Set once closing starts, since tearing the window down deactivates it and being deactivated is
	// itself one of the ways out of the box.
	bool m_Closing = false;

	// Set while the copy and paste menu of the edit control is up, since the menu takes the
	// activation of the box with it and giving that up is one of the ways out of the box.
	bool m_MenuOpen = false;
};

MeasureInputText::InputBox::~InputBox()
{
	Abort();
	DeleteObject(m_Font);
	DeleteObject(m_BackBrush);
}

bool MeasureInputText::InputBox::Open(const InputTextOptions& options, HWND skinWindow, float scale)
{
	std::shared_ptr<InputBox> keepAlive = shared_from_this();

	// Positions are relative to the skin, which is where the skin wrote them: the box is drawn
	// over the skin rather than in it, but a skin author places it against what they can see.
	RECT skinRect = { 0 };
	GetWindowRect(skinWindow, &skinRect);

	const bool topMost = options.topMost.value_or(
		(GetWindowLong(skinWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);

	WNDCLASSEX wndClass = { sizeof(WNDCLASSEX) };
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = GetRainmeter().GetModuleInstance();
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.lpszClassName = c_ClassName;

	// Registered on first use, and only ever once: two skins prompting at once share the class,
	// and the second registration is the one that fails.
	if (RegisterClassEx(&wndClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	{
		return false;
	}

	m_Options = options;
	m_SkinWindow = skinWindow;

	// The scale is applied here rather than where the options were read, so that a box opening now
	// opens at the scale the skin is at now.
	const int width = ScaleCoordinate(options.w, scale);
	const int height = ScaleCoordinate(options.h, scale);

	// A point size against 96 DPI and not against the DPI of the screen: the scale above is the
	// whole of what this has to answer to, and asking the screen as well would apply it twice.
	const LONG fontHeight = -(LONG)((options.fontSize * scale * 96.0) / 72.0 + 0.5);
	m_Font = CreateFont(fontHeight, 0, 0, 0, options.bold ? FW_BOLD : FW_NORMAL, options.italic,
		FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, options.fontFace.c_str());
	m_BackBrush = CreateSolidBrush(options.backColor);

	DWORD exStyle = WS_EX_TOOLWINDOW;
	if (topMost) exStyle |= WS_EX_TOPMOST;
	if (options.opacity < 255) exStyle |= WS_EX_LAYERED;

	m_Window = CreateWindowEx(exStyle, c_ClassName, L"", WS_POPUP,
		skinRect.left + ScaleCoordinate(options.x, scale),
		skinRect.top + ScaleCoordinate(options.y, scale),
		width, height, skinWindow, nullptr, GetRainmeter().GetModuleInstance(), this);

	if (m_Window)
	{
		if (options.opacity < 255)
		{
			SetLayeredWindowAttributes(m_Window, 0, options.opacity, LWA_ALPHA);
		}

		// FocusDismiss=0 is the modal case, and a modal dialog disables the window it belongs to.
		// The box cannot be dismissed by clicking away from it, so leaving the skin clickable
		// would let a click land on whatever is under a box that is still waiting to be answered.
		m_SkinDisabled = !options.focusDismiss && IsWindowEnabled(skinWindow);
		if (m_SkinDisabled) EnableWindow(skinWindow, FALSE);

		ShowWindow(m_Window, SW_SHOW);
		if (!m_Window) return false;

		SendMessage(m_Edit, EM_SETSEL, 0, (LPARAM)-1);

		SetForegroundWindow(m_Window);
		if (!m_Window) return false;
		SetFocus(m_Edit);
	}

	return keepAlive->m_Window;
}

void MeasureInputText::InputBox::Close(bool submitted)
{
	if (!m_Window || m_Closing) return;

	m_Closing = true;
	m_Submitted = submitted;

	// Completion can unload the skin, so let the input or menu callback that requested it unwind.
	PostMessage(m_Window, WM_INPUTTEXT_CLOSE, 0, 0);
}

void MeasureInputText::InputBox::Abort()
{
	HWND window = m_Window;
	m_Window = nullptr;
	if (window)
	{
		m_Closing = true;
		DestroyWindow(window);
	}

	RestoreSkinEnabledState();
	m_Measure.reset();
}

void MeasureInputText::InputBox::Complete()
{
	std::shared_ptr<MeasureInputText*> measureHandle = m_Measure;
	std::optional<std::wstring> input = FinishClose();
	MeasureInputText* measure = measureHandle ? *measureHandle : nullptr;

	if (measure && measure->m_Box.get() == this)
	{
		measure->m_Box.reset();
		measure->HandleInput(input);
	}
}

std::optional<std::wstring> MeasureInputText::InputBox::FinishClose()
{
	std::optional<std::wstring> input;
	if (m_Submitted)
	{
		// Trimmed, as the box has always trimmed it, so that a skin reading the text back does not
		// have to strip what a stray space at either end would leave in a path or a URL.
		input = Trim(GetControlText(m_Edit));
	}

	HWND window = m_Window;
	m_Window = nullptr;
	if (window) DestroyWindow(window);
	RestoreSkinEnabledState();
	return input;
}

void MeasureInputText::InputBox::RestoreSkinEnabledState()
{
	if (m_SkinDisabled && IsWindow(m_SkinWindow)) EnableWindow(m_SkinWindow, TRUE);
	m_SkinDisabled = false;
}

std::wstring MeasureInputText::InputBox::GetControlText(HWND control)
{
	const int length = GetWindowTextLength(control);
	if (length <= 0) return std::wstring();

	std::wstring text(length + 1, L'\0');
	const int copied = GetWindowText(control, text.data(), length + 1);
	text.resize(copied);
	return text;
}

LRESULT CALLBACK MeasureInputText::InputBox::WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	InputBox* box = (InputBox*)GetWindowLongPtr(wnd, GWLP_USERDATA);

	switch (msg)
	{
	case WM_NCCREATE:
		box = (InputBox*)((CREATESTRUCT*)lParam)->lpCreateParams;
		SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)box);
		break;

	default:
		if (!box) return DefWindowProc(wnd, msg, wParam, lParam);
		break;
	}

	std::shared_ptr<InputBox> keepAlive = box->shared_from_this();

	switch (msg)
	{
	case WM_CREATE:
		{
			const InputTextOptions& options = box->m_Options;

			RECT client = { 0 };
			GetClientRect(wnd, &client);

			// A password field is the one that cannot be multiline: a multiline edit control
			// ignores the mask and draws the text as it was typed, which is how Password=1 came to
			// do nothing at all in the WinForms box this grew out of.
			//
			// A multiline field wraps a line longer than the box onto a line below, and without
			// ES_AUTOVSCROLL it never scrolls to that line of its own accord: the arrow keys move
			// the caret onto text the box has scrolled away from, and a character that would wrap
			// is refused with a beep rather than typed. Only PgUp and PgDn, which scroll the box
			// themselves, reach any of it. The WinForms box this grew out of set the style on
			// every multiline field, and so does this one.
			DWORD style = WS_CHILD | WS_VISIBLE | options.align;
			style |= options.password ? (ES_PASSWORD | ES_AUTOHSCROLL) : (ES_MULTILINE | ES_AUTOVSCROLL);

			// A multiline edit control draws no line it cannot draw in full, so a box shorter than
			// one line of the font it was given comes up empty rather than cropped - and how tall
			// a line is depends on the face, which is what makes one face work where another does
			// not. The control is given the height a line needs and the box crops it, which is
			// what a box too short for its font ought to look like.
			int editHeight = client.bottom;
			if (!options.password)
			{
				HDC dc = GetDC(wnd);
				HFONT oldFont = (HFONT)SelectObject(dc, box->m_Font);

				TEXTMETRIC metrics = { 0 };
				if (GetTextMetrics(dc, &metrics) && metrics.tmHeight > editHeight)
				{
					editHeight = metrics.tmHeight;
				}

				SelectObject(dc, oldFont);
				ReleaseDC(wnd, dc);
			}

			box->m_Edit = CreateWindowEx(0L, WC_EDIT, options.text.c_str(), style,
				0, 0, client.right, editHeight, wnd, nullptr,
				GetRainmeter().GetModuleInstance(), nullptr);
			if (!box->m_Edit) return -1;

			SendMessage(box->m_Edit, WM_SETFONT, (WPARAM)box->m_Font, FALSE);
			if (options.maxLength > 0)
			{
				SendMessage(box->m_Edit, EM_SETLIMITTEXT, (WPARAM)options.maxLength, 0);
			}
			if (options.password)
			{
				SendMessage(box->m_Edit, EM_SETPASSWORDCHAR, (WPARAM)L'*', 0);
			}

			SetWindowSubclass(box->m_Edit, EditProc, 0, (DWORD_PTR)box);
		}
		return 0;

	case WM_ERASEBKGND:
		{
			RECT client = { 0 };
			GetClientRect(wnd, &client);
			FillRect((HDC)wParam, &client, box->m_BackBrush);
		}
		return 1;

	case WM_CTLCOLOREDIT:
		SetTextColor((HDC)wParam, box->m_Options.fontColor);
		SetBkColor((HDC)wParam, box->m_Options.backColor);
		return (LRESULT)box->m_BackBrush;

	case WM_INPUTTEXT_CLOSE:
		box->Complete();
		return 0;

	case WM_INPUTTEXT_MENUDONE:
		// The menu is gone. Whatever holds the foreground now is where the user went while it was
		// up, and if that is not the box then they went somewhere else and the box is done.
		if (box->m_Options.focusDismiss && GetForegroundWindow() != wnd)
		{
			box->Close(false);
		}
		else
		{
			SetFocus(box->m_Edit);
		}
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE && box->m_Options.focusDismiss && !box->m_MenuOpen)
		{
			box->Close(false);
		}
		return 0;

	case WM_CLOSE:
		box->Close(false);
		return 0;

	case WM_DESTROY:
		box->m_Window = nullptr;
		box->m_Edit = nullptr;
		return 0;
	}

	return DefWindowProc(wnd, msg, wParam, lParam);
}

LRESULT CALLBACK MeasureInputText::InputBox::EditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	InputBox* box = (InputBox*)data;
	std::shared_ptr<InputBox> keepAlive = box->shared_from_this();

	switch (msg)
	{
	case WM_KEYDOWN:
		// The box has no buttons to press, so these stand in for them. Enter submits even where
		// the field wraps onto several lines: what it holds is one value, and a skin has no way to
		// read a second line back.
		if (wParam == VK_RETURN)
		{
			box->Close(true);
			return 0;
		}
		if (wParam == VK_ESCAPE)
		{
			box->Close(false);
			return 0;
		}
		break;

	case WM_CHAR:
		// Both keys were acted on above; letting the character through as well would only make the
		// edit control beep at one it has nowhere to put.
		if (wParam == VK_RETURN || wParam == VK_ESCAPE) return 0;
		if (!box->AcceptsChar((WCHAR)wParam)) return 0;
		break;

	case WM_ENTERMENULOOP:
		box->m_MenuOpen = true;
		break;

	case WM_EXITMENULOOP:
		box->m_MenuOpen = false;

		// Asked from outside the menu loop, which is still on the stack here: closing the box now
		// would tear the edit control out from under the very call that is showing the menu.
		PostMessage(box->m_Window, WM_INPUTTEXT_MENUDONE, 0, 0);
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(wnd, EditProc, id);
		break;
	}

	return DefSubclassProc(wnd, msg, wParam, lParam);
}

bool MeasureInputText::InputBox::AcceptsChar(WCHAR ch) const
{
	if (!m_Options.numeric) return true;

	// Backspace and the rest of the control characters are how the field is edited at all.
	if (ch < 0x20 || ch == 0x7F) return true;

	// ASCII digits alone: a skin reading the field back as a number could not parse the digits of
	// any other script.
	if (ch != L'.' && ch != L'-' && (ch < L'0' || ch > L'9')) return false;

	if (ch == L'.')
	{
		// One decimal point per field, counting the one already in it even where the selection
		// about to be replaced is what holds it.
		if (GetControlText(m_Edit).find(L'.') != std::wstring::npos) return false;
	}

	if (ch == L'-')
	{
		// A sign only where a sign can go.
		DWORD start = 0L;
		SendMessage(m_Edit, EM_GETSEL, (WPARAM)&start, 0);
		if (start != 0L) return false;
	}

	return true;
}

MeasureInputText::MeasureInputText(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_MeasureRef(std::make_shared<MeasureInputText*>(this))
{
}

MeasureInputText::~MeasureInputText()
{
	*m_MeasureRef = nullptr;

	if (m_Box)
	{
		m_Box->Abort();
		m_Box.reset();
	}
}

void MeasureInputText::HandleSkinScaleChange(Skin* skin)
{
	for (auto* measure : skin->GetMeasures())
	{
		if (measure->GetTypeID() == TypeID<MeasureInputText>())
		{
			((MeasureInputText*)measure)->CloseBox();
		}
	}
}

void MeasureInputText::CloseBox()
{
	if (m_Box) m_Box->Close(false);
}

const WCHAR* MeasureInputText::GetStringValue()
{
	return CheckSubstitute(m_Input.c_str());
}

void MeasureInputText::Command(const std::wstring& command)
{
	// One box at a time: a second one over the same skin would be waiting for the same keyboard as
	// the first, so a bang arriving while one is open is dropped rather than queued.
	if (m_Box) return;

	m_Steps.clear();
	m_StepIndex = 0;

	auto& parser = m_Skin->GetParser();
	m_Options = InputTextOptions();
	for (const OptionName& entry : c_Options)
	{
		const std::wstring value = parser.ReadString(GetName(), entry.name, L"");
		if (!value.empty()) ApplyOption(m_Options, parser, entry.option, value);
	}

	parser.ReadString(m_DismissAction, GetName(), L"OnDismissAction", L"", { .sectionVariables = false });

	if (ReadSteps(command)) RunSteps();
}

bool MeasureInputText::ReadSteps(const std::wstring& command)
{
	ConfigParser& parser = m_Skin->GetParser();
	const std::wstring& section = GetOriginalName();
	const std::wstring args = Trim(command);

	// One word is the whole of the simple form, and the word names a variable: the box opens, and
	// what is typed into it is what the variable is set to.
	const size_t space = args.find(L' ');
	if (space == std::wstring::npos)
	{
		Step& step = m_Steps.emplace_back();
		step.variable = args;
		step.options = m_Options;
		step.prompts = true;
		return true;
	}

	const std::wstring verb = args.substr(0, space);
	if (_wcsicmp(verb.c_str(), L"ExecuteBatch") != 0)
	{
		LogWarningF(this, L"!CommandMeasure: Unknown command: %s", verb.c_str());
		return false;
	}

	// ExecuteBatch [All|#|#-#]. A range that cannot be read is taken as All, which is what it has
	// always fallen back to.
	std::wstring range = args.substr(space + 1);
	const size_t rangeEnd = range.find(L' ');
	if (rangeEnd != std::wstring::npos) range.erase(rangeEnd);
	range = Trim(range);

	int first = 1;
	int last = 1000000000;
	if (!range.empty() && _wcsicmp(range.c_str(), L"All") != 0)
	{
		const size_t dash = range.find(L'-');
		const int from = parser.ParseInt(range.substr(0, dash), 0);
		const int to = (dash == std::wstring::npos) ? from : parser.ParseInt(range.substr(dash + 1), 0);
		if (from > 0 && to > 0)
		{
			first = from;
			last = to;
		}
	}

	for (int i = first; i <= last; ++i)
	{
		std::wstring name = L"Command";
		name += std::to_wstring(i);
		const auto& line = parser.ReadString(section, name, L"", { .sectionVariables = false });
		if (line.empty()) break;

		Step& step = m_Steps.emplace_back();
		step.options = m_Options;
		step.command = ScanOverrides(parser, line, step.options);
		step.prompts = StringUtil::CaseInsensitiveFind(step.command, c_UserInputToken) != std::wstring::npos;
	}

	return !m_Steps.empty();
}

void MeasureInputText::RunSteps()
{
	// A bang can refresh or unload the skin, and that destroys this measure in the middle of the
	// run. The shared state outlives it and says so, which is what makes coming back here safe.
	std::shared_ptr<MeasureInputText*> measure = m_MeasureRef;

	while (m_StepIndex < m_Steps.size())
	{
		const Step& step = m_Steps[m_StepIndex];

		if (step.prompts)
		{
			m_Box = std::make_shared<InputBox>(m_MeasureRef);
			const bool opened = m_Box->Open(step.options, m_Skin->GetWindow(), m_Skin->GetScale());
			if (!*measure) return;

			if (opened) return;

			m_Box.reset();

			LogErrorF(this, L"InputText: Unable to open the input box");
			EndRun(true);
			return;
		}

		// Taken out and stepped past before it runs, since neither the step nor the measure
		// holding it is certain to be there afterwards.
		const std::wstring command = step.command;
		++m_StepIndex;

		GetRainmeter().ExecuteCommand(command.c_str(), m_Skin);
		if (!*measure) return;
	}

	EndRun(false);
}

void MeasureInputText::HandleInput(const std::optional<std::wstring>& input)
{
	if (m_StepIndex >= m_Steps.size()) return;

	if (!input)
	{
		EndRun(true);
		return;
	}

	m_Input = *input;

	const Step& step = m_Steps[m_StepIndex];
	++m_StepIndex;

	std::shared_ptr<MeasureInputText*> measure = m_MeasureRef;

	if (!step.variable.empty())
	{
		m_Skin->SetVariable(step.variable, m_Input);
	}
	else
	{
		std::wstring command = step.command;
		auto tokenPos = StringUtil::CaseInsensitiveFind(command, c_UserInputToken);
		if (tokenPos != std::wstring::npos) command.replace(tokenPos, wcslen(c_UserInputToken), m_Input);
		GetRainmeter().ExecuteCommand(command.c_str(), m_Skin);
	}

	if (!*measure) return;

	RunSteps();
}

void MeasureInputText::EndRun(bool dismissed)
{
	m_Steps.clear();
	m_StepIndex = 0;

	// Last of all: the action may refresh the skin, which takes this measure with it.
	if (dismissed && !m_DismissAction.empty())
	{
		GetRainmeter().ExecuteCommand(m_DismissAction.c_str(), m_Skin);
	}
}
