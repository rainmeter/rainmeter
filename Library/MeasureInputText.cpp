// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureInputText.h"
#include "AsyncTask.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "../Common/CriticalSection.h"
#include <Commctrl.h>

// What the box shares with the measure that opened it. The box is on a worker thread and the
// measure is not, and a bang between two prompts can refresh the skin and take the measure with
// it, so what one needs of the other is kept here instead - where it outlives them both.
struct MeasureInputText::SharedData
{
	explicit SharedData(MeasureInputText* measure) : measure(measure) {}

	std::atomic<bool> active = true;
	CriticalSection criticalSection;

	// The open box, for as long as one is open, so that it can be closed from the main thread.
	HWND window = nullptr;

	MeasureInputText* measure = nullptr;
};

namespace {

const WCHAR* c_ClassName = L"RainmeterInputText";
const WCHAR* c_UserInputToken = L"$UserInput$";

// Whitespace as a skin file can write it. Nothing here parses prose, so the Unicode spaces are not
// worth the call it would take to recognise them.
bool IsSpace(WCHAR ch)
{
	return ch == L' ' || ch == L'\t';
}

std::wstring Trim(const std::wstring& text)
{
	size_t start = 0U;
	while (start < text.size() && IsSpace(text[start])) ++start;

	size_t end = text.size();
	while (end > start && IsSpace(text[end - 1U])) --end;

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
			BYTE ignored = 255U;
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
	return (loc == std::wstring::npos) ? std::wstring::npos : loc + 1U;
}

// The value written at |loc|, which runs to the next space, or to the closing quote where it
// opened with one. The quotes are left on: the caller has to know how much of the line the setting
// took up in order to cut it back out.
std::wstring TagData(const std::wstring& line, const WCHAR* name, size_t loc)
{
	size_t i = loc + wcslen(name) + 1U;

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
		line.erase(loc - 1U, 1U + wcslen(entry.name) + 1U + tag.size());

		std::wstring value = tag;
		if (value.size() >= 2U && value.front() == L'"') value = value.substr(1U, value.size() - 2U);

		// The line itself is read with its section variables left alone, so that the bang keeps
		// its own until it runs. A setting is read here and now, and wants them resolved.
		parser.ReplaceMeasures(value);

		if (!value.empty()) ApplyOption(options, parser, entry.option, value);
	}

	return line;
}

}  // namespace

// The box itself: one window, open for as long as one prompt lasts. It belongs to the worker
// thread that shows it, and everything it is drawn with was settled before it was created - the
// styles of an edit control cannot be changed once the control exists.
class MeasureInputText::InputBox
{
public:
	explicit InputBox(const std::shared_ptr<SharedData>& data) : m_Data(data) {}

	// Opens the box over |skinWindow| and pumps messages until it is answered. Returns the text on
	// submit, and nothing when it was dismissed - by Escape, by losing focus, or by the measure
	// closing it from the main thread.
	std::optional<std::wstring> Show(const InputTextOptions& options, HWND skinWindow, float scale);

private:
	static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK EditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR id, DWORD_PTR data);

	static std::wstring GetControlText(HWND control);

	// |false| for a character the InputNumber filter refuses.
	bool AcceptsChar(WCHAR ch) const;

	void Close(bool submitted);

	std::shared_ptr<SharedData> m_Data;
	const InputTextOptions* m_Options = nullptr;
	HWND m_Window = nullptr;
	HWND m_Edit = nullptr;
	HFONT m_Font = nullptr;
	HBRUSH m_BackBrush = nullptr;
	bool m_Submitted = false;

	// Set for as long as one Close() is on its way through DestroyWindow, since tearing the window
	// down deactivates it, and being deactivated is itself one of the ways out of the box.
	bool m_Closing = false;

	// What Close() lifted out of the edit control, since the control is gone by the time Show()
	// has anything to return.
	std::wstring m_Text;
};

std::optional<std::wstring> MeasureInputText::InputBox::Show(const InputTextOptions& options, HWND skinWindow, float scale)
{
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
		return std::nullopt;
	}

	m_Options = &options;

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
	if (options.opacity < 255U) exStyle |= WS_EX_LAYERED;

	// Created and published to the measure under the one lock, and the measure having gone is read
	// back under it too: a box the measure asked to close in the middle of this is either found by
	// it there, or caught here.
	bool active = false;
	{
		CriticalSectionLock lock(m_Data->criticalSection);

		m_Window = CreateWindowEx(exStyle, c_ClassName, L"", WS_POPUP,
			skinRect.left + ScaleCoordinate(options.x, scale),
			skinRect.top + ScaleCoordinate(options.y, scale),
			width, height, skinWindow, nullptr, GetRainmeter().GetModuleInstance(), this);

		m_Data->window = m_Window;
		active = m_Data->active;
	}

	if (m_Window != nullptr && active)
	{
		if (options.opacity < 255U)
		{
			SetLayeredWindowAttributes(m_Window, 0, options.opacity, LWA_ALPHA);
		}

		// FocusDismiss=0 is the modal case, and a modal dialog disables the window it belongs to.
		// The box cannot be dismissed by clicking away from it, so leaving the skin clickable
		// would let a click land on whatever is under a box that is still waiting to be answered.
		const bool disableSkin = !options.focusDismiss && IsWindowEnabled(skinWindow);
		if (disableSkin) EnableWindow(skinWindow, FALSE);

		ShowWindow(m_Window, SW_SHOW);
		SetForegroundWindow(m_Window);
		SetFocus(m_Edit);

		// Select all.
		SendMessage(m_Edit, EM_SETSEL, 0U, (LPARAM)-1);

		MSG msg;
		while (GetMessage(&msg, nullptr, 0U, 0U) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Before the window goes, or the skin cannot take back the focus the box is dropping.
		if (disableSkin) EnableWindow(skinWindow, TRUE);
	}

	Close(false);

	DeleteObject(m_Font);
	m_Font = nullptr;
	DeleteObject(m_BackBrush);
	m_BackBrush = nullptr;
	m_Options = nullptr;

	if (!m_Submitted) return std::nullopt;
	return m_Text;
}

void MeasureInputText::InputBox::Close(bool submitted)
{
	if (m_Window == nullptr || m_Closing) return;

	if (submitted)
	{
		// Trimmed, as the box has always trimmed it, so that a skin reading the text back does not
		// have to strip what a stray space at either end would leave in a path or a URL.
		m_Text = Trim(GetControlText(m_Edit));
		m_Submitted = true;
	}

	m_Closing = true;
	DestroyWindow(m_Window);
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
		// Whatever arrives before the box has been attached above is nothing this handles.
		if (box == nullptr) return DefWindowProc(wnd, msg, wParam, lParam);
		break;
	}

	switch (msg)
	{
	case WM_CREATE:
		{
			const InputTextOptions& options = *box->m_Options;

			RECT client = { 0 };
			GetClientRect(wnd, &client);

			// A password field is the one that cannot wrap: a multiline edit control ignores the
			// mask and draws the text as it was typed, which is how Password=1 came to do nothing
			// at all in the WinForms box this grew out of.
			DWORD style = WS_CHILD | WS_VISIBLE | options.align;
			style |= options.password ? (ES_PASSWORD | ES_AUTOHSCROLL) : ES_MULTILINE;

			box->m_Edit = CreateWindowEx(0UL, WC_EDIT, options.text.c_str(), style,
				0, 0, client.right, client.bottom, wnd, nullptr,
				GetRainmeter().GetModuleInstance(), nullptr);
			if (box->m_Edit == nullptr) return -1;

			SendMessage(box->m_Edit, WM_SETFONT, (WPARAM)box->m_Font, FALSE);
			if (options.maxLength > 0)
			{
				SendMessage(box->m_Edit, EM_SETLIMITTEXT, (WPARAM)options.maxLength, 0);
			}
			if (options.password)
			{
				SendMessage(box->m_Edit, EM_SETPASSWORDCHAR, (WPARAM)L'*', 0);
			}

			SetWindowSubclass(box->m_Edit, EditProc, 0U, (DWORD_PTR)box);
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
		SetTextColor((HDC)wParam, box->m_Options->fontColor);
		SetBkColor((HDC)wParam, box->m_Options->backColor);
		return (LRESULT)box->m_BackBrush;

	case WM_ACTIVATE:
		// Clicking away from the box is the usual way out of it, and the only one a skin that
		// draws no buttons of its own has.
		if (LOWORD(wParam) == WA_INACTIVE && box->m_Options->focusDismiss)
		{
			box->Close(false);
		}
		return 0;

	case WM_CLOSE:
		box->Close(false);
		return 0;

	case WM_DESTROY:
		{
			CriticalSectionLock lock(box->m_Data->criticalSection);
			box->m_Data->window = nullptr;
		}

		box->m_Window = nullptr;
		box->m_Edit = nullptr;
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(wnd, msg, wParam, lParam);
}

LRESULT CALLBACK MeasureInputText::InputBox::EditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	InputBox* box = (InputBox*)data;

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

	case WM_NCDESTROY:
		RemoveWindowSubclass(wnd, EditProc, id);
		break;
	}

	return DefSubclassProc(wnd, msg, wParam, lParam);
}

bool MeasureInputText::InputBox::AcceptsChar(WCHAR ch) const
{
	if (!m_Options->numeric) return true;

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
		DWORD start = 0UL;
		SendMessage(m_Edit, EM_GETSEL, (WPARAM)&start, 0);
		if (start != 0UL) return false;
	}

	return true;
}

// One prompt, and the worker thread it is opened on. The box runs a message loop of its own, and
// running that on the main thread would stop every skin drawing until it was answered.
class MeasureInputText::PromptTask : public AsyncTask
{
public:
	static PromptTask* Create(MeasureInputText* measure, const std::shared_ptr<SharedData>& data,
		const InputTextOptions& options, HWND skinWindow, float scale)
	{
		auto* task = new PromptTask(measure);
		task->m_Data = data;
		task->m_Options = options;
		task->m_SkinWindow = skinWindow;
		task->m_Scale = scale;

		if (!task->Start())
		{
			delete task;
			return nullptr;
		}

		return task;
	}

private:
	PromptTask(MeasureInputText* measure) : AsyncTask(measure) {}

	void StartWorkOnWorkerThread() override
	{
		if (m_AbortRequested || !m_Data->active) return;

		InputBox box(m_Data);
		m_Input = box.Show(m_Options, m_SkinWindow, m_Scale);
	}

	void FinishWorkOnMainThread() override
	{
		if (m_AbortRequested || !m_Data->active) return;

		auto* measure = m_Data->measure;
		if (measure && measure->m_Task == this)
		{
			measure->m_Task = nullptr;
			measure->HandleInput(m_Input);
		}
	}

	std::shared_ptr<SharedData> m_Data;
	InputTextOptions m_Options;
	HWND m_SkinWindow = nullptr;
	float m_Scale = 1.0f;
	std::optional<std::wstring> m_Input;
};

MeasureInputText::MeasureInputText(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Data(std::make_shared<SharedData>(this))
{
}

MeasureInputText::~MeasureInputText()
{
	// The box is on a worker thread and has no way of noticing the measure go, so it is closed
	// from here. The answer its task posts back afterwards is dropped by the flag.
	m_Data->active = false;
	m_Data->measure = nullptr;

	if (m_Task)
	{
		CloseBox();
		m_Task->AbortWhenPossible();
		m_Task = nullptr;
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
	// Posted rather than sent: the window belongs to the worker thread, and a sent message would
	// wait on a message loop that may be ending on its own.
	CriticalSectionLock lock(m_Data->criticalSection);
	if (m_Data->window != nullptr) PostMessage(m_Data->window, WM_CLOSE, 0U, 0U);
}

void MeasureInputText::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	// Back to the defaults first: an option the skin has stopped writing has to stop applying too.
	m_Options = InputTextOptions();

	for (const OptionName& entry : c_Options)
	{
		// An option that is not there leaves the default standing, which is what tells a box with
		// no FontFace of its own from one asking for the empty string.
		const std::wstring value = parser.ReadString(section, entry.name, L"");
		if (!value.empty()) ApplyOption(m_Options, parser, entry.option, value);
	}

	// Read without section variables resolved: the action is written to be run later, and later is
	// when what it names should be looked up.
	parser.ReadString(m_DismissAction, section, L"OnDismissAction", L"", { .sectionVariables = false });
}

void MeasureInputText::UpdateValue()
{
	// Nothing to update: what this measure holds is the text a box was last answered with, and
	// that arrives when the box is answered rather than when the measure ticks.
}

const WCHAR* MeasureInputText::GetStringValue()
{
	return CheckSubstitute(m_Input.c_str());
}

void MeasureInputText::Command(const std::wstring& command)
{
	// One box at a time: a second one over the same skin would be waiting for the same keyboard as
	// the first, so a bang arriving while one is open is dropped rather than queued.
	if (m_Task != nullptr) return;

	m_Steps.clear();
	m_StepIndex = 0U;

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

	const std::wstring verb = args.substr(0U, space);
	if (_wcsicmp(verb.c_str(), L"ExecuteBatch") != 0)
	{
		LogWarningF(this, L"!CommandMeasure: Unknown command: %s", verb.c_str());
		return false;
	}

	// ExecuteBatch [All|#|#-#]. A range that cannot be read is taken as All, which is what it has
	// always fallen back to.
	std::wstring range = args.substr(space + 1U);
	const size_t rangeEnd = range.find(L' ');
	if (rangeEnd != std::wstring::npos) range.erase(rangeEnd);
	range = Trim(range);

	int first = 1;
	int last = 1000000000;
	if (!range.empty() && _wcsicmp(range.c_str(), L"All") != 0)
	{
		const size_t dash = range.find(L'-');
		const int from = parser.ParseInt(range.substr(0U, dash), 0);
		const int to = (dash == std::wstring::npos) ? from : parser.ParseInt(range.substr(dash + 1U), 0);
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

		// Read with its section variables left alone: the line is a bang to be run later, and
		// later is when what it names should be looked up.
		const std::wstring line = parser.ReadString(section, name, L"", { .sectionVariables = false });

		// The first line that is not there ends the batch, whatever the range said: All is a range
		// with no end of its own.
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
	// run. The state the box shares with its task outlives it and says so, which is what makes
	// coming back here afterwards safe.
	std::shared_ptr<SharedData> data = m_Data;

	while (m_StepIndex < m_Steps.size())
	{
		const Step& step = m_Steps[m_StepIndex];

		if (step.prompts)
		{
			m_Task = PromptTask::Create(this, m_Data, step.options, m_Skin->GetWindow(),
				m_Skin->GetScale());

			// The rest of the run waits for HandleInput().
			if (m_Task != nullptr) return;

			LogErrorF(this, L"InputText: Unable to open the input box");
			EndRun(true);
			return;
		}

		// Taken out and stepped past before it runs, since neither the step nor the measure
		// holding it is certain to be there afterwards.
		const std::wstring command = step.command;
		++m_StepIndex;

		GetRainmeter().ExecuteCommand(command.c_str(), m_Skin);
		if (!data->active) return;
	}

	EndRun(false);
}

void MeasureInputText::HandleInput(const std::optional<std::wstring>& input)
{
	if (m_StepIndex >= m_Steps.size()) return;

	if (!input)
	{
		// A run stops where it was dismissed: every line after this one was written expecting this
		// one to have been answered.
		EndRun(true);
		return;
	}

	m_Input = *input;

	const Step& step = m_Steps[m_StepIndex];
	++m_StepIndex;

	std::shared_ptr<SharedData> data = m_Data;

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

	if (!data->active) return;

	RunSteps();
}

void MeasureInputText::EndRun(bool dismissed)
{
	m_Steps.clear();
	m_StepIndex = 0U;

	// Last of all: the action may refresh the skin, which takes this measure with it.
	if (dismissed && !m_DismissAction.empty())
	{
		GetRainmeter().ExecuteCommand(m_DismissAction.c_str(), m_Skin);
	}
}
