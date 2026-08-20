// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterTextEdit.h"
#include "Pcre.h"
#include "Rainmeter.h"
#include "System.h"
#include "../Common/Gfx/Canvas.h"

namespace {

// Undo steps are bounded so that a long editing session cannot grow without limit.
const size_t c_MaxUndoSteps = 100U;

enum class CharClass
{
	Space,
	Word,
	Punctuation
};

CharClass ClassifyChar(WCHAR ch)
{
	// iswspace()/iswalnum() only recognise ASCII in the "C" locale, which the process never leaves
	// (only LC_NUMERIC is ever set), so Cyrillic, Greek, Arabic, Hebrew and CJK would all classify
	// as punctuation. GetStringTypeW is locale independent.
	WORD type = 0;
	if (!GetStringTypeW(CT_CTYPE1, &ch, 1, &type)) return CharClass::Punctuation;

	if ((type & C1_SPACE) != 0) return CharClass::Space;

	// Underscore counts as part of a word, as it does in most editors. Lone surrogates are alpha
	// in neither half, so a pair classifies as punctuation in both halves and stays in one run,
	// which keeps a word boundary from landing inside it.
	if ((type & (C1_ALPHA | C1_DIGIT)) != 0 || ch == L'_') return CharClass::Word;
	return CharClass::Punctuation;
}

// Takes the caret for the length of a bang, as a click into the field would, and runs what that
// leaves behind on the way out: the field being left has an action for being left, and the field
// taking the caret has one of its own. They are held until the scope ends because either may
// refresh or close the skin - destroying both meters and the window - so the bang has to have
// finished what it came to do before they go off.
class FocusMeterScope
{
public:
	FocusMeterScope(MeterTextEdit* meter, Skin* skin)
	{
		if (meter->IsFocused() || !meter->AcceptsInput()) return;

		m_Outgoing = skin->GetInputFocusMeter();
		if (!skin->SetInputFocus(meter, &m_DismissCommand)) return;

		// The window has to hold keyboard focus for typing to reach the field at all.
		SetFocus(skin->GetWindow());

		m_Incoming = meter;
		m_FocusCommand = meter->GetOnFocusAction();
	}

	// In the order the two things happened. Nothing may be touched afterwards.
	~FocusMeterScope()
	{
		if (!m_DismissCommand.empty())
		{
			GetRainmeter().ExecuteActionCommand(m_DismissCommand.c_str(), m_Outgoing);
		}

		if (!m_FocusCommand.empty())
		{
			GetRainmeter().ExecuteActionCommand(m_FocusCommand.c_str(), m_Incoming);
		}
	}

	FocusMeterScope(const FocusMeterScope& other) = delete;
	FocusMeterScope& operator=(FocusMeterScope other) = delete;

private:
	MeterTextEdit* m_Outgoing = nullptr;
	std::wstring m_DismissCommand;

	MeterTextEdit* m_Incoming = nullptr;
	std::wstring m_FocusCommand;
};

void DoFocusBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	auto* editMeter = (MeterTextEdit*)meter;
	if (!editMeter->AcceptsInput())
	{
		LogWarningF(skin, L"!TextEdit:Focus: [%s] does not accept input", meter->GetName());
		return;
	}

	FocusMeterScope focus(editMeter, skin);
}

void DoDismissBang(std::vector<std::wstring>& args, Skin* skin)
{
	skin->DismissInputFocus();
}

void DoSubmitBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	((MeterTextEdit*)meter)->Submit();
}

void DoSelectBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	ConfigParser& parser = skin->GetParser();
	const int index = parser.ParseInt(args[0].c_str(), 0);
	const int length = parser.ParseInt(args[1].c_str(), -1);

	auto* editMeter = (MeterTextEdit*)meter;
	FocusMeterScope focus(editMeter, skin);
	editMeter->SelectRange(index, length);
}

void DoSelectAllBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	auto* editMeter = (MeterTextEdit*)meter;
	FocusMeterScope focus(editMeter, skin);
	editMeter->SelectAll();
}

void DoSetTextBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	((MeterTextEdit*)meter)->SetText(args[0]);
	skin->RequestWindowSizeCheck();
}

void DoScrollByLineBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	const int lines = skin->GetParser().ParseInt(args[0].c_str(), 0);
	((MeterTextEdit*)meter)->ScrollByLine(lines);
}

void DoResetScrollBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	((MeterTextEdit*)meter)->ResetScroll();
}

void DoClearBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	((MeterTextEdit*)meter)->Clear();
	skin->RequestWindowSizeCheck();
}

void DoResetBang(Meter* meter, std::vector<std::wstring>& args, Skin* skin)
{
	((MeterTextEdit*)meter)->Reset();
	skin->RequestWindowSizeCheck();
}

}  // namespace

MeterTextEdit::MeterTextEdit(Skin* skin, const WCHAR* name) : MeterStringBase(skin, name),
	m_AcceptsInput(true),
	m_Focused(false),
	m_TrackInitialText(true),
	m_Submitted(false),
	m_MaxLength(0),
	m_Multiline(false),
	m_SubmitOnEnter(true),
	m_Password(false),
	m_PasswordChar(),
	m_RegExpError(false),
	m_InputCase(TEXTCASE_NONE),
	m_CaretDrawnVisible(false),
	m_CaretColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_SelectionColor(D2D1::ColorF(D2D1::ColorF::SteelBlue, 0.5f)),
	m_FocusBorderColor(D2D1::ColorF(D2D1::ColorF::Black, 0.0f)),
	m_FocusBorderWidth(1.0f),
	m_CaretPos(0U),
	m_SelectionAnchor(0U),
	m_CaretTrailing(false),
	m_PlaceholderColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_PlaceholderFontSize(10.0f),
	m_PlaceholderStyle(NORMAL),
	m_PlaceholderFormat(),
	m_CaretBlinkStart(0ULL),
	m_LastEditKind(EditKind::None)
{
	static const bool s_BangsRegistered = []()
	{
		const UINT typeId = TypeID<MeterTextEdit>();
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:Focus", 0, DoFocusBang);
		CommandHandler::RegisterSkinBang(L"TextEdit:Dismiss", 0, DoDismissBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:Submit", 0, DoSubmitBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:Select", 2, DoSelectBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:SelectAll", 0, DoSelectAllBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:SetText", 1, DoSetTextBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:ScrollByLine", 1, DoScrollByLineBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:ResetScroll", 0, DoResetScrollBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:Clear", 0, DoClearBang);
		CommandHandler::RegisterMeterBang(typeId, L"TextEdit:Reset", 0, DoResetBang);
		return true;
	} ();
}

MeterTextEdit::~MeterTextEdit()
{
}

void MeterTextEdit::Initialize()
{
	MeterStringBase::Initialize();
	UpdatePlaceholderFormat();
	UpdatePasswordChar();
}

void MeterTextEdit::InvalidateDeviceResources()
{
	MeterStringBase::InvalidateDeviceResources();
	if (m_PlaceholderFormat)
	{
		m_PlaceholderFormat->InvalidateDeviceResources();
	}
}

void MeterTextEdit::UpdatePlaceholderFormat()
{
	if (m_PlaceholderText.empty())
	{
		m_PlaceholderFormat.reset();
		return;
	}

	if (!m_PlaceholderFormat)
	{
		m_PlaceholderFormat.reset(m_Skin->GetCanvas().CreateTextFormat());
	}

	m_PlaceholderFormat->SetProperties(
		m_PlaceholderFontFace.c_str(),
		m_PlaceholderFontSize,
		(m_PlaceholderStyle & BOLD) != 0,
		(m_PlaceholderStyle & ITALIC) != 0,
		m_Skin->GetFontCollection());

	// The placeholder sits in the same box as the text would, so it follows the meter's alignment.
	m_PlaceholderFormat->SetHorizontalAlignment(m_TextFormat->GetHorizontalAlignment());
	m_PlaceholderFormat->SetVerticalAlignment(m_TextFormat->GetVerticalAlignment());
}

void MeterTextEdit::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	MeterStringBase::ReadOptions(parser, section);

	m_MaxLength = parser.ReadInt(section, L"MaxLength", 0);
	m_Multiline = parser.ReadBool(section, L"Multiline", false);

	if (m_TrackInitialText)
	{
		parser.ReadString(m_Text, section, L"InitialText", L"");

		// The caret, the scroll and the undo history all point into the text this replaced. Put
		// back to where a new field has them, rather than clamped: nothing here is the user's.
		m_SelectionAnchor = m_CaretPos = 0U;
		m_CaretTrailing = false;
		m_TextOffset = D2D1::Point2F();
		ClearUndoHistory();
	}

	// The limits are re-read above and can have moved, so they are applied to whatever the field
	// holds - what the user typed included - rather than only to a freshly read option.
	ApplyTextTransformations(m_Text);

	const bool password = parser.ReadBool(section, L"Password", false);
	const bool passwordChanged = password != m_Password;
	m_Password = password;

	// Every filter is a pattern, so that a rule about the text around a character - where a sign or
	// a decimal point may go - is expressed the same way as one about the character alone.
	//
	// The patterns below end in [0-9]* rather than [0-9]+ so that they also match a half-typed
	// number: "-" and "1." as much as "-1.5". That is not what partial matching is for, because
	// PCRE reports a partial match only when it finds no full one - given "1.", [0-9]+ would match
	// just the "1", which AcceptsReplacement() reads as a refusal, and the digit after the point
	// could never be typed.
	//
	// [0-9] and not \d, which would also take the digits of other scripts, and those are not what
	// a skin reading the field back as a number can parse. No pattern needs ^ or $, since
	// AcceptsReplacement() anchors both ends itself.
	const std::wstring& filterOption = parser.ReadString(section, L"InputFilter", L"NONE");
	const WCHAR* filter = filterOption.c_str();
	if (_wcsicmp(filter, L"NONE") == 0)
	{
		SetInputRegExp(L"");
	}
	else if (_wcsicmp(filter, L"NUMERIC") == 0)
	{
		SetInputRegExp(L"[0-9]*");
	}
	else if (_wcsicmp(filter, L"INTEGER") == 0)
	{
		SetInputRegExp(L"-?[0-9]*");
	}
	else if (_wcsicmp(filter, L"DECIMAL") == 0)
	{
		SetInputRegExp(L"-?[0-9]*\\.?[0-9]*");
	}
	else if (_wcsicmp(filter, L"REGEXP") == 0)
	{
		SetInputRegExp(parser.ReadString(section, L"InputRegExp", L".*"));
	}
	else
	{
		LogErrorF(this, L"InputFilter=%s is not valid", filter);
		SetInputRegExp(L"");
	}

	// PROPER is not offered here: it is a property of whole words, and the text is converted a
	// keystroke at a time, with no way to know whether more of the word is still coming. StringCase
	// converts the whole text and does support it.
	m_InputCase = ReadStringCase(parser, section, L"InputCase", TEXTCASE_NONE);
	if (m_InputCase == TEXTCASE_PROPER)
	{
		LogErrorF(this, L"InputCase=PROPER is not valid");
		m_InputCase = TEXTCASE_NONE;
	}

	m_SubmitOnEnter = parser.ReadBool(section, L"SubmitOnEnter", !m_Multiline);

	// Read without measure replacement so that it resolves when the action runs rather than when
	// the option is read, which is what lets it reference [$Input].
	parser.ReadString(m_OnSubmitAction, section, L"OnSubmitAction", L"", { .sectionVariables = false });
	parser.ReadString(m_OnFocusAction, section, L"OnFocusAction", L"", { .sectionVariables = false });
	parser.ReadString(m_OnDismissAction, section, L"OnDismissAction", L"", { .sectionVariables = false });

	m_FocusBorderColor = parser.ReadColor(section, L"FocusBorderColor",
		D2D1::ColorF(D2D1::ColorF::Black, 0.0f));
	if (m_FocusBorderColor.a > 0.0f)
	{
		m_FocusBorderWidth = (FLOAT)parser.ReadFloat(section, L"FocusBorderWidth", 1.0);
		if (m_FocusBorderWidth < 0.0f) m_FocusBorderWidth = 0.0f;
	}

	parser.ReadString(m_PlaceholderText, section, L"PlaceholderText", L"");
	if (!m_PlaceholderText.empty())
	{
		// Only read once there is a placeholder to draw, so a meter without one pays nothing here.
		D2D1_COLOR_F dimmed = m_Color;
		dimmed.a *= 0.4f;
		m_PlaceholderColor = parser.ReadColor(section, L"PlaceholderFontColor", dimmed);

		m_PlaceholderFontFace = parser.ReadString(section, L"PlaceholderFontFace", m_FontFace.c_str());
		if (m_PlaceholderFontFace.empty()) m_PlaceholderFontFace = m_FontFace;

		m_PlaceholderFontSize = (FLOAT)parser.ReadFloat(section, L"PlaceholderFontSize", m_FontSize);
		if (m_PlaceholderFontSize < 0.0f) m_PlaceholderFontSize = m_FontSize;

		// Unset inherits the meter's own style.
		m_PlaceholderStyle = ReadStringStyle(parser, section, L"PlaceholderStringStyle", m_Style);
	}

	// Done after the font options above so a changed placeholder font takes effect, and after the
	// base has already recreated the meter's own font if that changed.
	if (m_Initialized)
	{
		UpdatePlaceholderFormat();
		UpdatePasswordChar();

		// A field that has just been masked - or that just took a new InitialText - must not go on
		// drawing the old string until the next Update(), which UpdateDivider can defer.
		if (passwordChanged || m_TrackInitialText)
		{
			SyncDrawnString();
			UpdateAutoSizeForText();
		}
	}
	m_CaretColor = parser.ReadColor(section, L"CaretColor", m_Color);

	// The default is the system highlight at half alpha, since the highlight is drawn behind the
	// text rather than under a recolored run of it.
	const COLORREF highlight = GetSysColor(COLOR_HIGHLIGHT);
	m_SelectionColor = parser.ReadColor(section, L"SelectionColor", D2D1::ColorF(
		GetRValue(highlight) / 255.0f,
		GetGValue(highlight) / 255.0f,
		GetBValue(highlight) / 255.0f,
		0.5f));

	// Without AccurateText the renderer emulates GDI+ by nudging the draw origin and by adding
	// per-character spacing to the layout. Editing only has to be correct in one of the two text
	// models, so it opts out of the emulated one rather than compensating for it.
	m_AcceptsInput = true;
	if (!m_Skin->GetCanvas().IsAccurateText())
	{
		LogWarningF(this, L"Meter=TextEdit requires AccurateText=1");
		m_AcceptsInput = false;
	}

	if (!m_AcceptsInput)
	{
		m_Skin->ClearInputFocus(this);
	}
}

void MeterTextEdit::BindMeasures(ConfigParser& parser, std::wstring_view section)
{
	// The text is the user's, so there is no measure to bind.
}

void MeterTextEdit::ApplyTextTransformations(std::wstring& text) const
{
	if (!m_Multiline)
	{
		text.erase(std::remove(text.begin(), text.end(), L'\n'), text.end());
	}

	if (m_MaxLength > 0 && text.length() > (size_t)m_MaxLength)
	{
		text.resize(m_MaxLength);

		// Truncating must not leave a high surrogate without its pair.
		if (IS_HIGH_SURROGATE(text.back())) text.pop_back();
	}

	ApplyCase(text);
}

void MeterTextEdit::UpdatePasswordChar()
{
	if (!m_Password) return;

	const WCHAR blackCircle = L'\u25cf';
	const WCHAR bullet = L'\u2022';
	const WCHAR asterisk = L'*';

	// A Win32 edit control masks with the black circle and nothing else, but the masked string is
	// nothing but the mask character, so a font without it is not a run that falls back but a whole
	// line whose metrics come from the substitute - a baseline that moves the moment the field is
	// masked. Take the best character the font can draw itself instead, which leaves the line
	// exactly where the text left it. The asterisk is the last resort, and is one every font has.
	m_PasswordChar =
		m_TextFormat->HasCharacter(blackCircle) ? blackCircle :
		m_TextFormat->HasCharacter(bullet) ? bullet :
		asterisk;
}

void MeterTextEdit::SyncDrawnString()
{
	// Rendered verbatim: any rewrite would shift the string relative to m_Text and put the caret
	// offsets DirectWrite reports in a different index space than the edited text. A mask is the
	// one rewrite that cannot, since it replaces each UTF-16 unit with exactly one of its own.
	if (m_Password)
	{
		m_String.assign(m_Text.length(), m_PasswordChar);
	}
	else
	{
		m_String = m_Text;
	}
}

bool MeterTextEdit::Update()
{
	if (Meter::Update())
	{
		SyncDrawnString();

		const UINT32 len = (UINT32)m_String.length();
		m_CaretPos = min(m_CaretPos, len);
		m_SelectionAnchor = min(m_SelectionAnchor, len);

		UpdateTextFormat();
		UpdateAutoSizeForText();

		return true;
	}
	return false;
}

bool MeterTextEdit::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	// A meter given a size can hold more text than fits, scrolled or not, so what does not fit is
	// cut at the box. An auto-sized one grows to its text and has nothing to cut.
	const bool clip = m_WDefined || m_HDefined || ShouldClip() || m_TextOffset.x != 0.0f || m_TextOffset.y != 0.0f;
	if (clip) canvas.PushClip(GetMeterRectPadding());

	// The highlight goes behind the glyphs, so it has to be drawn before the text. It builds the
	// same layout DrawString() is about to use, so the extra call is not an extra layout.
	if (m_Focused && HasSelection())
	{
		DrawSelection(canvas);
	}

	bool drawn = true;
	if (ShowingPlaceholder())
	{
		drawn = DrawString(canvas, &m_PlaceholderText, m_PlaceholderFormat.get(), &m_PlaceholderColor);
	}
	else
	{
		drawn = DrawString(canvas);
	}

	// Drawn over the placeholder, and using the meter's own format rather than the placeholder's,
	// so a smaller placeholder does not shrink the caret.
	if (drawn && m_Focused)
	{
		DrawCaret(canvas);
	}

	if (clip) canvas.PopClip();

	// Outside the clip, so the frame is not scrolled away with the text.
	if (m_Focused) DrawFocusBorder(canvas);

	return drawn;
}

void MeterTextEdit::UpdateAutoSizeForText()
{
	if (ShowingPlaceholder())
	{
		UpdateAutoSize(&m_PlaceholderText, m_PlaceholderFormat.get());
	}
	else if (m_String.empty() || m_String.back() == L'\n')
	{
		// An empty string measures zero in both directions, so a field the user just cleared would
		// collapse to nothing and leave nothing to click on or to put the caret in. A trailing
		// newline is also ignored by measurement, which makes sense for drawing, but not for editing.
		// Handle both cases with an additional period.
		const std::wstring measured = m_String + L".";
		UpdateAutoSize(&measured);
	}
	else
	{
		UpdateAutoSize();
	}
}

void MeterTextEdit::EnsureCaretVisible()
{
	if (!m_TextFormat->IsInitialized()) return;

	// Nothing to scroll to, and the caret rect an empty string gives back is no basis for deciding
	// otherwise. Handled here rather than being left to the clamp below so that the placeholder,
	// which is drawn through the same offset, is not left scrolled off by the text that was there.
	if (m_String.empty())
	{
		m_TextOffset = D2D1::Point2F();
		return;
	}

	Gfx::Canvas& canvas = m_Skin->GetCanvas();
	ApplyTextState(canvas);

	D2D1_RECT_F caret = { 0 };
	if (!canvas.GetCaretRect(
		m_String, *m_TextFormat, GetTextRect(), m_CaretPos, m_CaretTrailing, 1.0f, caret)) return;

	// The caret comes back in screen space, so comparing it against the unscrolled box gives the
	// distance the text has to move for it to come into view.
	const D2D1_RECT_F box = GetMeterRectPadding();

	if (caret.right > box.right) m_TextOffset.x += caret.right - box.right;
	else if (caret.left < box.left) m_TextOffset.x -= box.left - caret.left;

	if (caret.bottom > box.bottom) m_TextOffset.y += caret.bottom - box.bottom;
	else if (caret.top < box.top) m_TextOffset.y -= box.top - caret.top;

	ClampTextOffset();
}

void MeterTextEdit::ClampTextOffset()
{
	// Never scroll past the start; there is nothing to reveal before it.
	m_TextOffset.x = max(m_TextOffset.x, 0.0f);
	m_TextOffset.y = max(m_TextOffset.y, 0.0f);

	if (m_TextOffset.x == 0.0f && m_TextOffset.y == 0.0f) return;

	Gfx::Canvas& canvas = m_Skin->GetCanvas();
	ApplyTextState(canvas);

	// The caret position after the last character sits at the end of the text, so it gives that
	// edge back without measuring the string again.
	D2D1_RECT_F endCaret = { 0 };
	if (!canvas.GetCaretRect(m_String, *m_TextFormat, GetTextRect(), (UINT32)m_String.length(), true, 1.0f, endCaret)) return;

	const D2D1_RECT_F box = GetMeterRectPadding();

	if (endCaret.right < box.right)
	{
		m_TextOffset.x = max(m_TextOffset.x - (box.right - endCaret.right), 0.0f);
	}

	if (endCaret.bottom < box.bottom)
	{
		m_TextOffset.y = max(m_TextOffset.y - (box.bottom - endCaret.bottom), 0.0f);
	}
}

FLOAT MeterTextEdit::GetLineHeight()
{
	if (m_String.empty()) return 0.0f;

	Gfx::Canvas& canvas = m_Skin->GetCanvas();
	ApplyTextState(canvas);

	// Taken from the caret at the start of the text rather than from the font, so that whatever the
	// layout made of the line - a fallback font among the metrics, or a format that asked for more
	// leading - is what a line steps by.
	D2D1_RECT_F caret = { 0 };
	if (!canvas.GetCaretRect(m_String, *m_TextFormat, GetTextRect(), 0U, false, 1.0f, caret))
	{
		return 0.0f;
	}

	return max(caret.bottom - caret.top, 0.0f);
}

void MeterTextEdit::ScrollByLine(int lines)
{
	// Focus has no say here: the scroll belongs to the field, so a skin can drive an idle one and
	// have it stay where it was put.
	if (lines == 0 || !m_TextFormat->IsInitialized()) return;

	const FLOAT lineHeight = GetLineHeight();
	if (lineHeight <= 0.0f) return;

	m_TextOffset.y += lineHeight * (FLOAT)lines;
	ClampTextOffset();
}

void MeterTextEdit::ResetScroll()
{
	m_TextOffset = D2D1::Point2F();
}

void MeterTextEdit::DrawFocusBorder(Gfx::Canvas& canvas)
{
	if (m_FocusBorderColor.a <= 0.0f || m_FocusBorderWidth <= 0.0f) return;

	const RECT meterRect = GetMeterRect();
	const FLOAT left = (FLOAT)meterRect.left;
	const FLOAT top = (FLOAT)meterRect.top;
	const FLOAT right = (FLOAT)meterRect.right;
	const FLOAT bottom = (FLOAT)meterRect.bottom;

	// Inset, so the border frames the meter rather than bleeding onto its neighbours. A border
	// thicker than the meter collapses to a filled rect rather than drawing the edges twice.
	const FLOAT w = min(m_FocusBorderWidth, min((right - left) / 2.0f, (bottom - top) / 2.0f));
	if (w <= 0.0f) return;

	// Drawn as four strips that meet rather than overlap at the corners, so a semi-transparent
	// colour does not blend against itself there.
	canvas.FillRectangle(D2D1::RectF(left, top, right, top + w), m_FocusBorderColor);
	canvas.FillRectangle(D2D1::RectF(left, bottom - w, right, bottom), m_FocusBorderColor);
	canvas.FillRectangle(D2D1::RectF(left, top + w, left + w, bottom - w), m_FocusBorderColor);
	canvas.FillRectangle(D2D1::RectF(right - w, top + w, right, bottom - w), m_FocusBorderColor);
}

bool MeterTextEdit::IsCaretVisible() const
{
	const UINT blinkTime = GetCaretBlinkTime();

	// GetCaretBlinkTime() returns INFINITE when the user has turned caret blinking off.
	if (blinkTime == 0U || blinkTime == INFINITE) return true;

	const ULONGLONG elapsed = GetTickCount64() - m_CaretBlinkStart;
	return ((elapsed / blinkTime) % 2ULL) == 0ULL;
}

void MeterTextEdit::SetFocus(bool focus)
{
	if (m_Focused == focus) return;

	m_Focused = focus;
	m_CaretBlinkStart = GetTickCount64();
	m_Submitted = false;

	// The scroll is the field's own and outlives being left, so that a skin can put a field where it
	// wants it and have it stay there, and so that coming back to one finds it as it was.
	if (!focus) return;

	m_TrackInitialText = false;

	// Nothing can be typed until the field has the caret, so this is the last moment the pattern
	// is needed and the first at which a skin full of unfocused fields has not paid for one.
	CompileInputRegExp();

	EnsureCaretVisible();
}

bool MeterTextEdit::IsSubmitKey(WPARAM key) const
{
	return key == VK_RETURN && m_SubmitOnEnter;
}

void MeterTextEdit::Submit()
{
	m_Submitted = true;

	if (!m_OnSubmitAction.empty())
	{
		GetRainmeter().ExecuteActionCommand(m_OnSubmitAction.c_str(), this);
	}
}

void MeterTextEdit::HandleDismiss(std::wstring& command)
{
	// A submit is not something to then abandon: it has already run its action and had its say over
	// the text, so leaving afterwards is only a dismissal once the text has moved on from it.
	if (m_Submitted) return;

	command = m_OnDismissAction;
}

void MeterTextEdit::Clear()
{
	if (m_String.empty()) return;

	// Emptying the field is the skin's doing rather than the user's next edit, so it leaves a
	// submit standing: an OnSubmitAction that clears the field must not thereby turn leaving it
	// into a dismissal.
	const bool submitted = m_Submitted;

	PushUndo(EditKind::None);
	SelectAll();
	ReplaceSelection(std::wstring());

	m_Submitted = submitted;
}

void MeterTextEdit::SetText(std::wstring_view text)
{
	m_TrackInitialText = false;

	std::wstring newText(text);
	ApplyTextTransformations(newText);

	if (newText == m_Text) return;

	// Like Clear(), the skin's doing rather than the user's next edit: undoable, but leaving a
	// submit standing so that replacing what was sent is not itself abandoning it.
	PushUndo(EditKind::None);

	m_Text = std::move(newText);
	SyncDrawnString();

	// The caret goes to the end, where typing on from the new text is what the user is most likely
	// to want, and where nothing has to be clamped to a text it no longer indexes into.
	m_SelectionAnchor = m_CaretPos = (UINT32)m_String.length();
	m_CaretTrailing = true;

	UpdateAutoSizeForText();

	// Text a skin puts into an idle field is read from its start, wherever the field happened to be
	// scrolled to. A field being typed into follows the caret this left at the end instead, which is
	// where typing on from the new text carries on.
	m_Focused ? EnsureCaretVisible() : ResetScroll();
}

void MeterTextEdit::Reset()
{
	ConfigParser& parser = m_Skin->GetParser();
	ConfigParser::InheritChainScope inheritChain(parser, GetName(), true);
	std::wstring text = parser.ReadString(GetName(), L"InitialText", L"");

	ApplyTextTransformations(text);

	m_Text = std::move(text);
	m_TrackInitialText = true;
	SyncDrawnString();

	m_SelectionAnchor = m_CaretPos = 0U;
	m_CaretTrailing = false;
	m_CaretBlinkStart = GetTickCount64();
	m_LastEditKind = EditKind::None;
	m_TextOffset = D2D1::Point2F();
	ClearUndoHistory();

	// m_Submitted is left as it was, as Clear() leaves it: OnSubmitAction resetting the field is a
	// common enough thing to write, and it must not turn leaving the field into a dismissal.

	UpdateAutoSizeForText();
}

void MeterTextEdit::MoveCaretTo(UINT32 pos, bool extend, bool trailing)
{
	m_CaretPos = min(pos, (UINT32)m_String.length());
	m_CaretTrailing = trailing;
	if (!extend)
	{
		m_SelectionAnchor = m_CaretPos;
	}

	EnsureCaretVisible();

	// Restart the blink cycle so the caret is solid wherever it just landed.
	m_CaretBlinkStart = GetTickCount64();

	// Moving the caret ends the current run of edits, so typing either side of a click undoes as
	// two steps rather than one.
	m_LastEditKind = EditKind::None;
}

void MeterTextEdit::PushUndo(EditKind kind)
{
	m_RedoStack.clear();

	if (kind != EditKind::None && kind == m_LastEditKind && !m_UndoStack.empty())
	{
		// Same kind of edit as the last one and the caret has not moved since, so fold this into
		// the step already on the stack.
		return;
	}

	if (m_UndoStack.size() >= c_MaxUndoSteps)
	{
		m_UndoStack.erase(m_UndoStack.begin());
	}

	m_UndoStack.push_back({ m_Text, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
	m_LastEditKind = kind;
}

void MeterTextEdit::ApplySnapshot(const EditSnapshot& snapshot)
{
	m_Text = snapshot.text;

	// A snapshot taken under a different StringCase would otherwise bring the old case back.
	ApplyCase(m_Text);
	SyncDrawnString();

	const UINT32 len = (UINT32)m_String.length();
	m_CaretPos = min(snapshot.caret, len);
	m_CaretTrailing = snapshot.trailing;
	m_SelectionAnchor = min(snapshot.anchor, len);
	m_CaretBlinkStart = GetTickCount64();
	m_Submitted = false;

	UpdateAutoSizeForText();
}

bool MeterTextEdit::Undo()
{
	if (m_UndoStack.empty()) return false;

	m_RedoStack.push_back({ m_Text, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
	ApplySnapshot(m_UndoStack.back());
	m_UndoStack.pop_back();

	m_LastEditKind = EditKind::None;
	return true;
}

bool MeterTextEdit::Redo()
{
	if (m_RedoStack.empty()) return false;

	m_UndoStack.push_back({ m_Text, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
	ApplySnapshot(m_RedoStack.back());
	m_RedoStack.pop_back();

	m_LastEditKind = EditKind::None;
	return true;
}

void MeterTextEdit::ClearUndoHistory()
{
	m_UndoStack.clear();
	m_RedoStack.clear();
	m_LastEditKind = EditKind::None;
}

bool MeterTextEdit::SetCaretFromPoint(int x, int y, bool extend)
{
	if (!m_AcceptsInput || !m_TextFormat->IsInitialized()) return false;

	Gfx::Canvas& canvas = m_Skin->GetCanvas();
	ApplyTextState(canvas);

	UINT32 caretPos = 0U;
	bool trailing = false;
	if (!canvas.HitTestTextPoint(m_String, *m_TextFormat, GetTextRect(),
		D2D1::Point2F((FLOAT)x, (FLOAT)y), caretPos, trailing))
	{
		return false;
	}

	MoveCaretTo(caretPos, extend, trailing);
	return true;
}

void MeterTextEdit::SelectAll()
{
	m_SelectionAnchor = 0U;
	m_CaretPos = (UINT32)m_String.length();
	m_CaretTrailing = true;
	m_CaretBlinkStart = GetTickCount64();
	m_LastEditKind = EditKind::None;
}

void MeterTextEdit::SelectRange(int start, int length)
{
	const UINT32 textLength = (UINT32)m_String.length();
	const UINT32 from = min((UINT32)max(start, 0), textLength);

	m_SelectionAnchor = from;
	m_CaretPos = length < 0 ? textLength : min(from + (UINT32)length, textLength);
	m_CaretTrailing = true;
	m_CaretBlinkStart = GetTickCount64();
	m_LastEditKind = EditKind::None;

	EnsureCaretVisible();
}

bool MeterTextEdit::SelectLineAtCaret()
{
	if (!m_AcceptsInput || !m_TextFormat->IsInitialized()) return false;

	Gfx::Canvas& canvas = m_Skin->GetCanvas();
	ApplyTextState(canvas);

	UINT32 lineStart = 0U;
	UINT32 lineEnd = (UINT32)m_String.length();
	if (!canvas.GetLineRange(
		m_String, *m_TextFormat, GetTextRect(), m_CaretPos, lineStart, lineEnd))
	{
		return false;
	}

	m_SelectionAnchor = lineStart;
	m_CaretPos = lineEnd;
	m_CaretTrailing = true;
	m_CaretBlinkStart = GetTickCount64();
	m_LastEditKind = EditKind::None;
	return true;
}

bool MeterTextEdit::SelectWordAtCaret()
{
	if (!m_AcceptsInput || m_String.empty()) return false;

	const UINT32 len = (UINT32)m_String.length();
	const UINT32 pos = min(m_CaretPos, len);

	// Double-clicking past the end of a word should still select that word, so start from the
	// character to the left when the caret sits on a boundary.
	const bool onBoundary = pos == len || ClassifyChar(m_String[pos]) == CharClass::Space;
	const UINT32 probe = onBoundary && pos > 0U ? pos - 1U : pos;
	const CharClass wanted = ClassifyChar(m_String[probe]);

	UINT32 start = probe;
	while (start > 0U && ClassifyChar(m_String[start - 1U]) == wanted) --start;

	UINT32 end = probe;
	while (end < len && ClassifyChar(m_String[end]) == wanted) ++end;

	m_SelectionAnchor = start;
	m_CaretPos = end;
	m_CaretTrailing = true;
	m_CaretBlinkStart = GetTickCount64();
	m_LastEditKind = EditKind::None;
	return true;
}

bool MeterTextEdit::GetAdjacentCaretIndex(UINT32 pos, bool forward, UINT32& adjacent)
{
	if (!m_Password)
	{
		Gfx::Canvas& canvas = m_Skin->GetCanvas();
		ApplyTextState(canvas);
		return canvas.GetAdjacentCaretIndex(
			m_String, *m_TextFormat, GetTextRect(), pos, forward, adjacent);
	}

	// Every unit of masked text draws as its own mask character, so the clusters the layout would
	// report are the mask's and not the text's. Stepping over the text instead keeps the caret out
	// of the middle of a surrogate pair, which is the only thing here that is more than one unit.
	const UINT32 len = (UINT32)m_Text.length();
	if (forward ? pos >= len : pos == 0U) return false;

	UINT32 next = forward ? pos + 1U : pos - 1U;
	if (next > 0U && next < len && IS_LOW_SURROGATE(m_Text[next]))
	{
		next = forward ? next + 1U : next - 1U;
	}

	adjacent = next;
	return true;
}

UINT32 MeterTextEdit::FindWordBoundary(UINT32 pos, bool forward) const
{
	const UINT32 len = (UINT32)m_String.length();
	pos = min(pos, len);

	if (forward)
	{
		// Stop at the start of the next word: step over the run under the caret, then over any
		// whitespace following it.
		if (pos >= len) return len;

		const CharClass cls = ClassifyChar(m_String[pos]);
		while (pos < len && ClassifyChar(m_String[pos]) == cls) ++pos;
		while (pos < len && ClassifyChar(m_String[pos]) == CharClass::Space) ++pos;
		return pos;
	}

	// Stop at the start of the word to the left, stepping back over any whitespace first.
	while (pos > 0U && ClassifyChar(m_String[pos - 1U]) == CharClass::Space) --pos;
	if (pos == 0U) return 0U;

	const CharClass cls = ClassifyChar(m_String[pos - 1U]);
	while (pos > 0U && ClassifyChar(m_String[pos - 1U]) == cls) --pos;
	return pos;
}

bool MeterTextEdit::IsFull() const
{
	return m_MaxLength > 0 && m_String.length() >= (size_t)m_MaxLength;
}

void MeterTextEdit::SetInputRegExp(const std::wstring& pattern)
{
	// A dynamic InputRegExp is re-read on every update, so an unchanged pattern must keep whatever
	// compiling it already produced: recompiling would cost the same work every update, and would
	// report a pattern that does not compile over and over.
	if (pattern == m_InputRegExpPattern) return;

	m_InputRegExpPattern = pattern;
	m_RegExp.reset();
	m_RegExpError = false;
}

void MeterTextEdit::CompileInputRegExp()
{
	if (m_InputRegExpPattern.empty() || m_RegExp || m_RegExpError) return;

	const char* error = nullptr;
	m_RegExp = std::make_unique<Pcre>(m_InputRegExpPattern.c_str(), &error);
	if (*m_RegExp) return;

	LogErrorF(this, L"Error: \"%S\" in InputRegExp=%s", error, m_InputRegExpPattern.c_str());

	// Fail open: a pattern that does not compile cannot say what is allowed, and refusing
	// everything would leave the field impossible to type into. No pattern is no filter, so
	// dropping it is all that takes.
	m_RegExp.reset();
	m_RegExpError = true;
}

std::wstring MeterTextEdit::PreviewReplacement(const std::wstring& text, std::wstring& insert) const
{
	const UINT32 start = GetSelectionStart();
	const UINT32 end = GetSelectionEnd();

	insert = text;

	// UPPER and LOWER convert each character on its own, so the insert can be converted before it
	// is spliced in, leaving text the user did not type alone.
	ApplyCase(insert, m_InputCase);

	if (m_MaxLength > 0 && !insert.empty())
	{
		// What the selection leaves behind is what the limit has to accommodate.
		const size_t kept = m_Text.length() - (end - start);
		const size_t room = kept < (size_t)m_MaxLength ? (size_t)m_MaxLength - kept : 0U;

		if (insert.length() > room)
		{
			insert.resize(room);

			// Truncating must not leave a high surrogate without its pair.
			if (!insert.empty() && IS_HIGH_SURROGATE(insert.back())) insert.pop_back();
		}
	}

	std::wstring result = m_Text;
	result.replace(start, end - start, insert);

	// Converted whole rather than per insert, since PROPER has to see the surrounding words.
	ApplyCase(result);
	return result;
}

bool MeterTextEdit::AcceptsReplacement(const std::wstring& text) const
{
	if (!m_RegExp) return true;

	// All or nothing, and judged on the text as it would be stored: a pattern covers the whole
	// field, so there is no meaningful part of a refused edit to keep. Salvaging the longest
	// prefix that still matched would turn one rejected paste into a field holding half of it.
	std::wstring insert;
	const std::wstring result = PreviewReplacement(text, insert);

	// An empty field is always reachable, whatever the pattern says, so that a filter can never
	// leave text the user is unable to delete.
	if (result.empty()) return true;

	// PCRE_ANCHORED fixes the match at the start; the end is checked below. Between them the
	// pattern is judged against the whole text, so a pattern that would otherwise match a fragment
	// of it - and validate nothing - cannot.
	int ovector[3] = { 0 };
	const int rc = m_RegExp->Execute(result, PCRE_ANCHORED | PCRE_PARTIAL_SOFT, ovector, (int)_countof(ovector));

	// A partial match is text on its way to matching: the pattern ran out of subject rather than
	// failing against it. Accepting it is what lets a field be typed into one character at a time
	// instead of only ever holding a complete match.
	if (rc == PCRE_ERROR_PARTIAL) return true;

	// A match that stops short leaves a tail the pattern does not cover, which is a refusal even
	// though PCRE reports a match. Partial matching is preferred over a short match only when the
	// pattern reaches the end of the subject, so this is the one that has to be caught here.
	return rc >= 0 && ovector[1] == (int)result.length();
}

void MeterTextEdit::ReplaceSelection(const std::wstring& text)
{
	const UINT32 start = GetSelectionStart();

	std::wstring insert;
	m_Text = PreviewReplacement(text, insert);
	m_TrackInitialText = false;
	SyncDrawnString();

	m_SelectionAnchor = m_CaretPos = start + (UINT32)insert.length();
	m_CaretTrailing = true;
	m_Submitted = false;

	// Before EnsureCaretVisible(), which measures the caret against the meter box: on an auto-sized
	// meter that box is what the text just made it, so scrolling first would compare the new caret
	// against the width of the previous text and scroll a field that in fact fits.
	UpdateAutoSizeForText();

	EnsureCaretVisible();
	m_CaretBlinkStart = GetTickCount64();
}

bool MeterTextEdit::CopySelection(bool cut)
{
	// A masked field hands nothing to the clipboard, as a Win32 password edit does not - cutting
	// included, since it would put the text there on its way out. Deleting still empties it.
	if (m_Password) return false;

	if (!HasSelection()) return false;

	const UINT32 start = GetSelectionStart();
	System::SetClipboardText(m_Text.substr(start, GetSelectionEnd() - start));

	if (!cut) return false;  // Copying changes nothing on screen.

	PushUndo(EditKind::None);
	ReplaceSelection(std::wstring());
	return true;
}

bool MeterTextEdit::Paste()
{
	auto clipboard = System::GetClipboardText();
	if (!clipboard || clipboard->empty()) return false;

	std::wstring text = std::move(*clipboard);

	// Normalise line endings so that a pasted CRLF does not become a two unit break the caret has
	// to step over, and so that a lone CR cannot end up in the drawn string.
	size_t pos = 0U;
	while ((pos = text.find(L'\r', pos)) != std::wstring::npos)
	{
		if (pos + 1U < text.length() && text[pos + 1U] == L'\n')
		{
			text.erase(pos, 1U);
		}
		else
		{
			text[pos] = L'\n';
			++pos;
		}
	}

	// A single-line field takes the text with its breaks dropped, as a Win32 edit control without
	// ES_MULTILINE does, rather than refusing the paste or keeping a line it cannot show.
	if (!m_Multiline)
	{
		text.erase(std::remove(text.begin(), text.end(), L'\n'), text.end());
	}

	// An empty paste is not an edit, and must not swallow the selection. Neither must one the
	// filter refuses, which it does whole rather than by dropping the characters it dislikes.
	if (text.empty() || !AcceptsReplacement(text)) return false;

	PushUndo(EditKind::None);
	ReplaceSelection(text);
	return true;
}

void MeterTextEdit::DeleteWord(bool forward)
{
	if (!HasSelection())
	{
		const UINT32 boundary = FindWordBoundary(m_CaretPos, forward);
		if (boundary == m_CaretPos) return;

		PushUndo(EditKind::Deleting);
		m_SelectionAnchor = boundary;
	}
	else
	{
		PushUndo(EditKind::Deleting);
	}

	ReplaceSelection(std::wstring());
}

void MeterTextEdit::DeleteSelectionOr(bool forward)
{
	if (!HasSelection())
	{
		const UINT32 len = (UINT32)m_String.length();
		if (forward ? m_CaretPos >= len : m_CaretPos == 0U) return;

		// Deleting has to take the whole cluster, otherwise backspacing over an emoji would leave
		// half a surrogate pair behind.
		UINT32 adjacent = m_CaretPos;
		if (!GetAdjacentCaretIndex(m_CaretPos, forward, adjacent)) return;

		// Taken after the early returns above so that a no-op keypress leaves no undo step, and
		// before the anchor moves so that the snapshot holds the selection the user had.
		PushUndo(EditKind::Deleting);
		m_SelectionAnchor = adjacent;
	}
	else
	{
		PushUndo(EditKind::Deleting);
	}

	ReplaceSelection(std::wstring());
}

bool MeterTextEdit::HandleChar(WCHAR ch)
{
	if (!m_AcceptsInput) return false;

	// Control characters arrive here too (Ctrl+A as 0x01, Escape as 0x1B, and so on). They are
	// either handled as key presses or not at all, so none of them belong in the text.
	if (ch < L' ' || ch == 0x7F) return false;

	// A full field still accepts typing that replaces a selection.
	if (IsFull() && !HasSelection()) return false;

	// Asked before the undo stack is touched: a refused keystroke is not an edit and must leave
	// nothing behind, not even a step that undoes to itself, and it must not reach
	// ReplaceSelection() to delete the selection in place of the character that was refused.
	const std::wstring insert(1U, ch);
	if (!AcceptsReplacement(insert)) return false;

	// Replacing a selection is a distinct step from the typing that follows it, so it does not
	// fold into a preceding run.
	PushUndo(HasSelection() ? EditKind::None : EditKind::Typing);
	ReplaceSelection(insert);
	m_LastEditKind = EditKind::Typing;
	return true;
}

bool MeterTextEdit::HandleKeyDown(WPARAM key, bool ctrl, bool shift)
{
	if (!m_AcceptsInput) return false;

	Gfx::Canvas& canvas = m_Skin->GetCanvas();
	const UINT32 len = (UINT32)m_String.length();

	switch (key)
	{
	case 'A':
		if (!ctrl) return false;
		SelectAll();
		return true;

	case 'C':
		if (!ctrl) return false;
		return CopySelection(false);

	case 'X':
		if (!ctrl) return false;
		return CopySelection(true);

	case 'V':
		if (!ctrl) return false;
		return Paste();

	case 'Z':
		if (!ctrl) return false;
		return shift ? Redo() : Undo();

	case 'Y':
		if (!ctrl) return false;
		return Redo();

	case VK_INSERT:
		// The older bindings for the same three operations.
		if (ctrl) return CopySelection(false);
		if (shift) return Paste();
		return false;

	case VK_LEFT:
	case VK_RIGHT:
		{
			const bool forward = key == VK_RIGHT;

			// An unshifted arrow against a selection collapses it to that side rather than moving.
			if (!shift && HasSelection())
			{
				MoveCaretTo(forward ? GetSelectionEnd() : GetSelectionStart(), false, forward);
				return true;
			}

			if (ctrl)
			{
				MoveCaretTo(FindWordBoundary(m_CaretPos, forward), shift, forward);
				return true;
			}

			UINT32 adjacent = m_CaretPos;
			if (GetAdjacentCaretIndex(m_CaretPos, forward, adjacent))
			{
				MoveCaretTo(adjacent, shift, forward);
			}
		}
		return true;

	case VK_UP:
	case VK_DOWN:
		{
			ApplyTextState(canvas);

			const D2D1_RECT_F meterRect = GetTextRect();
			D2D1_RECT_F caretRect = { 0 };
			if (!canvas.GetCaretRect(
				m_String, *m_TextFormat, meterRect, m_CaretPos, m_CaretTrailing, 1.0f, caretRect))
			{
				return true;
			}

			// Step a whole line height above/below the caret and hit-test there, which keeps the
			// caret in roughly the same column the way an edit control does.
			const FLOAT lineHeight = caretRect.bottom - caretRect.top;
			const FLOAT y = key == VK_UP ?
				caretRect.top - lineHeight / 2.0f : caretRect.bottom + lineHeight / 2.0f;

			UINT32 caretPos = 0U;
			bool trailing = false;
			if (canvas.HitTestTextPoint(m_String, *m_TextFormat, meterRect,
				D2D1::Point2F(caretRect.left, y), caretPos, trailing))
			{
				MoveCaretTo(caretPos, shift, trailing);
			}
		}
		return true;

	case VK_HOME:
	case VK_END:
		{
			const bool end = key == VK_END;
			if (ctrl)
			{
				MoveCaretTo(end ? len : 0U, shift, end);
				return true;
			}

			ApplyTextState(canvas);

			UINT32 lineStart = 0U;
			UINT32 lineEnd = len;
			if (canvas.GetLineRange(
				m_String, *m_TextFormat, GetTextRect(), m_CaretPos, lineStart, lineEnd))
			{
				MoveCaretTo(end ? lineEnd : lineStart, shift, end);
			}
		}
		return true;

	case VK_RETURN:
		// The skin handles the submit. Shift+Enter still adds a line, where there may be one, as it
		// does in a chat box that sends on Enter.
		if (!shift && m_SubmitOnEnter) return false;

		if (!m_Multiline) return false;

		// No filter is meant for text with lines in it, and a newline the filter went on to refuse
		// would delete the selection in place of the line it was meant to add.
		if (m_RegExp) return false;

		// WM_CHAR delivers Enter as a carriage return, which the control character filter in
		// HandleChar drops, so the newline is inserted from here instead. A line feed rather than
		// a CRLF keeps it one unit for the caret to step over.
		PushUndo(EditKind::None);
		ReplaceSelection(std::wstring(1U, L'\n'));
		return true;

	case VK_BACK:
		if (ctrl)
		{
			DeleteWord(false);
			return true;
		}

		DeleteSelectionOr(false);
		return true;

	case VK_DELETE:
		// Shift+Delete is cut, which has to be checked before the deletion bindings.
		if (shift) return CopySelection(true);

		if (ctrl)
		{
			DeleteWord(true);
			return true;
		}

		DeleteSelectionOr(true);
		return true;

	case VK_ESCAPE:
		// Handled by the skin, which drops the focus.
		return false;
	}

	return false;
}

void MeterTextEdit::DrawCaret(Gfx::Canvas& canvas)
{
	m_CaretDrawnVisible = IsCaretVisible();
	if (!m_CaretDrawnVisible) return;

	DWORD caretWidth = 1UL;
	if (!SystemParametersInfo(SPI_GETCARETWIDTH, 0U, &caretWidth, 0U) || caretWidth == 0UL)
	{
		caretWidth = 1UL;
	}

	D2D1_RECT_F caretRect = { 0 };
	if (canvas.GetCaretRect(m_String, *m_TextFormat, GetTextRect(), m_CaretPos, m_CaretTrailing, (FLOAT)caretWidth, caretRect))
	{
		canvas.FillRectangle(caretRect, m_CaretColor);
	}
}

void MeterTextEdit::DrawSelection(Gfx::Canvas& canvas)
{
	ApplyTextState(canvas);

	const UINT32 start = GetSelectionStart();

	std::vector<D2D1_RECT_F> rects;
	if (!canvas.GetTextRangeRects(m_String, *m_TextFormat, GetTextRect(),
		start, GetSelectionEnd() - start, rects))
	{
		return;
	}

	for (const auto& rect : rects)
	{
		canvas.FillRectangle(rect, m_SelectionColor);
	}
}

