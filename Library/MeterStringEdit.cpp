// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterStringEdit.h"
#include "Pcre.h"
#include "Rainmeter.h"
#include "System.h"
#include "../Common/Gfx/Canvas.h"

namespace {

// Undo steps are bounded so that a long editing session cannot grow without limit.
const size_t c_MaxUndoSteps = 100U;

// What Password draws in place of the text, as a Win32 edit control has masked with since Vista.
const WCHAR c_PasswordChar = L'\x25CF';  // BLACK CIRCLE

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

}  // namespace

MeterStringEdit::MeterStringEdit(Skin* skin, const WCHAR* name) : MeterStringBase(skin, name),
	m_AcceptsInput(true),
	m_Focused(false),
	m_ClearOnFocus(false),
	m_SelectAllOnFocus(false),
	m_ClearOnEnter(false),
	m_ClearOnDismiss(false),
	m_Committed(false),
	m_MaxLength(0),
	m_Password(false),
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
}

MeterStringEdit::~MeterStringEdit()
{
}

void MeterStringEdit::Initialize()
{
	MeterStringBase::Initialize();
	UpdatePlaceholderFormat();
}

void MeterStringEdit::InvalidateDeviceResources()
{
	MeterStringBase::InvalidateDeviceResources();
	if (m_PlaceholderFormat)
	{
		m_PlaceholderFormat->InvalidateDeviceResources();
	}
}

void MeterStringEdit::UpdatePlaceholderFormat()
{
	if (m_Placeholder.empty())
	{
		m_PlaceholderFormat.reset();
		return;
	}

	if (!m_PlaceholderFormat)
	{
		m_PlaceholderFormat.reset(m_Skin->GetCanvas().CreateTextFormat(m_Skin->GetMathParser()));
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

void MeterStringEdit::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	MeterStringBase::ReadOptions(parser, section);

	// DynamicVariables re-reads options every update, which would discard what the user typed, so
	// Text is adopted only when the option itself changed - that still lets !SetOption or a
	// changed variable replace it.
	const std::wstring& text = parser.ReadString(section, L"Text", L"");
	if (!m_Initialized || text != m_TextOption)
	{
		m_Text = text;

		// A value arriving from the config is not something the user should be able to undo past.
		ClearUndoHistory();
	}
	m_TextOption = text;

	// Unconditional so that a StringCase arriving later - through !SetOption or a dynamic variable -
	// also converts text that is already there, whether it came from the option or from the user.
	ApplyCase(m_Text);

	m_ClearOnFocus = parser.ReadBool(section, L"ClearOnFocus", false);
	m_SelectAllOnFocus = parser.ReadBool(section, L"SelectAllOnFocus", false);
	m_ClearOnEnter = parser.ReadBool(section, L"ClearOnEnter", false);
	m_ClearOnDismiss = parser.ReadBool(section, L"ClearOnDismiss", false);
	m_MaxLength = parser.ReadInt(section, L"MaxLength", 0);

	const bool password = parser.ReadBool(section, L"Password", false);
	if (password != m_Password)
	{
		m_Password = password;

		// Not left to the next Update(), which UpdateDivider can defer: a field that has just been
		// masked must not go on drawing its text until then.
		if (m_Initialized) SyncDrawnString();
	}

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

	// Read without measure replacement so that it resolves when the action runs rather than when
	// the option is read, which is what lets it reference [$Input].
	m_OnEnterAction = parser.ReadString(section, L"OnEnterAction", L"", false);
	m_OnFocusAction = parser.ReadString(section, L"OnFocusAction", L"", false);
	m_OnDismissAction = parser.ReadString(section, L"OnDismissAction", L"", false);

	m_FocusBorderColor = parser.ReadColor(section, L"FocusBorderColor",
		D2D1::ColorF(D2D1::ColorF::Black, 0.0f));
	if (m_FocusBorderColor.a > 0.0f)
	{
		m_FocusBorderWidth = (FLOAT)parser.ReadFloat(section, L"FocusBorderWidth", 1.0);
		if (m_FocusBorderWidth < 0.0f) m_FocusBorderWidth = 0.0f;
	}

	m_Placeholder = parser.ReadString(section, L"Placeholder", L"");
	if (!m_Placeholder.empty())
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
	if (m_Initialized) UpdatePlaceholderFormat();
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
		LogWarningF(this, L"Meter=StringEdit requires AccurateText=1");
		m_AcceptsInput = false;
	}

	if (!m_AcceptsInput)
	{
		m_Skin->ClearInputFocus(this);
	}
}

void MeterStringEdit::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	// The text is the user's, so there is no measure to bind.
}

void MeterStringEdit::SyncDrawnString()
{
	// Rendered verbatim: any rewrite would shift the string relative to m_Text and put the caret
	// offsets DirectWrite reports in a different index space than the edited text. A mask is the
	// one rewrite that cannot, since it replaces each UTF-16 unit with exactly one of its own.
	if (m_Password)
	{
		m_String.assign(m_Text.length(), c_PasswordChar);
	}
	else
	{
		m_String = m_Text;
	}
}

bool MeterStringEdit::Update()
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

bool MeterStringEdit::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	// Trimming clips the text itself, but not the caret or the selection behind it.
	const bool clip = ShouldTrim() || m_TextOffset.x != 0.0f || m_TextOffset.y != 0.0f;
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
		drawn = DrawString(canvas, nullptr, &m_Placeholder, m_PlaceholderFormat.get(),
			&m_PlaceholderColor);
	}
	else
	{
		drawn = DrawString(canvas, nullptr);
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

void MeterStringEdit::UpdateAutoSizeForText()
{
	// An empty auto-sized field would collapse to nothing and become unclickable, so it takes its
	// size from the placeholder instead.
	if (ShowingPlaceholder())
	{
		UpdateAutoSize(&m_Placeholder, m_PlaceholderFormat.get());
	}
	else
	{
		UpdateAutoSize();
	}
}

void MeterStringEdit::EnsureCaretVisible()
{
	if (!m_TextFormat->IsInitialized()) return;

	// ClipString trims the text to the meter instead, which is the other answer to the same
	// overflow, so a clipped field never scrolls.
	if (ShouldTrim())
	{
		m_TextOffset = D2D1::Point2F();
		return;
	}

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

	// Never scroll past the start; there is nothing to reveal before it.
	m_TextOffset.x = max(m_TextOffset.x, 0.0f);
	m_TextOffset.y = max(m_TextOffset.y, 0.0f);

	// Nor past the end: deleting text, or a meter that grew, can leave more of the box scrolled
	// past than there is text to fill it. The caret position after the last character sits at the
	// end of the text, so it gives that edge back without measuring the string again.
	if (m_TextOffset.x > 0.0f || m_TextOffset.y > 0.0f)
	{
		D2D1_RECT_F endCaret = { 0 };
		if (canvas.GetCaretRect(m_String, *m_TextFormat, GetTextRect(),
			(UINT32)m_String.length(), true, 1.0f, endCaret))
		{
			if (endCaret.right < box.right)
			{
				m_TextOffset.x = max(m_TextOffset.x - (box.right - endCaret.right), 0.0f);
			}

			if (endCaret.bottom < box.bottom)
			{
				m_TextOffset.y = max(m_TextOffset.y - (box.bottom - endCaret.bottom), 0.0f);
			}
		}
	}
}

void MeterStringEdit::DrawFocusBorder(Gfx::Canvas& canvas)
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

bool MeterStringEdit::IsCaretVisible() const
{
	const UINT blinkTime = GetCaretBlinkTime();

	// GetCaretBlinkTime() returns INFINITE when the user has turned caret blinking off.
	if (blinkTime == 0U || blinkTime == INFINITE) return true;

	const ULONGLONG elapsed = GetTickCount64() - m_CaretBlinkStart;
	return ((elapsed / blinkTime) % 2ULL) == 0ULL;
}

void MeterStringEdit::SetFocus(bool focus)
{
	if (m_Focused == focus) return;

	m_Focused = focus;
	m_CaretBlinkStart = GetTickCount64();
	m_Committed = false;

	if (!focus) return;

	// Nothing can be typed until the field has the caret, so this is the last moment the pattern
	// is needed and the first at which a skin full of unfocused fields has not paid for one.
	CompileInputRegExp();

	// ClearOnFocus wins over SelectAllOnFocus: there is nothing left to select once it has run.
	if (m_ClearOnFocus)
	{
		Clear();
	}
	else if (m_SelectAllOnFocus)
	{
		SelectAll();
	}
}

bool MeterStringEdit::HandleEnter(std::wstring& command)
{
	if (!CommitsOnEnter()) return false;

	// Expanded before anything is cleared, so [$Input] still carries what was submitted.
	ExpandAction(m_OnEnterAction, command);

	if (m_ClearOnEnter) Clear();

	// After Clear(), which edits the text and would otherwise reset this again.
	m_Committed = true;

	return true;
}

std::wstring MeterStringEdit::GetFocusCommand()
{
	std::wstring command;
	ExpandAction(m_OnFocusAction, command);
	return command;
}

void MeterStringEdit::HandleDismiss(std::wstring& command)
{
	// A commit is not something to then abandon: Enter has already run its action and had its say
	// over the text, so leaving afterwards is only a dismissal once the text has moved on from it.
	if (m_Committed) return;

	ExpandAction(m_OnDismissAction, command);

	if (m_ClearOnDismiss) Clear();
}

void MeterStringEdit::Clear()
{
	if (m_String.empty()) return;

	PushUndo(EditKind::None);
	SelectAll();
	ReplaceSelection(std::wstring());
}

void MeterStringEdit::ExpandAction(const std::wstring& action, std::wstring& command)
{
	if (action.empty()) return;

	command = action;

	// Expanded here rather than by the command handler because [$Input] is scoped to this meter,
	// exactly as mouse actions expand [$MOUSEX] before they are handed over.
	m_Skin->GetParser().ExpandSectionVariables(command, VariableExpandMode::DollarInputOnly, this);
}

void MeterStringEdit::MoveCaretTo(UINT32 pos, bool extend, bool trailing)
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

void MeterStringEdit::PushUndo(EditKind kind)
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

void MeterStringEdit::ApplySnapshot(const EditSnapshot& snapshot)
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
	m_Committed = false;

	UpdateAutoSizeForText();
}

bool MeterStringEdit::Undo()
{
	if (m_UndoStack.empty()) return false;

	m_RedoStack.push_back({ m_Text, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
	ApplySnapshot(m_UndoStack.back());
	m_UndoStack.pop_back();

	m_LastEditKind = EditKind::None;
	return true;
}

bool MeterStringEdit::Redo()
{
	if (m_RedoStack.empty()) return false;

	m_UndoStack.push_back({ m_Text, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
	ApplySnapshot(m_RedoStack.back());
	m_RedoStack.pop_back();

	m_LastEditKind = EditKind::None;
	return true;
}

void MeterStringEdit::ClearUndoHistory()
{
	m_UndoStack.clear();
	m_RedoStack.clear();
	m_LastEditKind = EditKind::None;
}

bool MeterStringEdit::SetCaretFromPoint(int x, int y, bool extend)
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

void MeterStringEdit::SelectAll()
{
	m_SelectionAnchor = 0U;
	m_CaretPos = (UINT32)m_String.length();
	m_CaretTrailing = true;
	m_CaretBlinkStart = GetTickCount64();
	m_LastEditKind = EditKind::None;
}

bool MeterStringEdit::SelectLineAtCaret()
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

bool MeterStringEdit::SelectWordAtCaret()
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

bool MeterStringEdit::GetAdjacentCaretIndex(UINT32 pos, bool forward, UINT32& adjacent)
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

UINT32 MeterStringEdit::FindWordBoundary(UINT32 pos, bool forward) const
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

bool MeterStringEdit::IsFull() const
{
	return m_MaxLength > 0 && m_String.length() >= (size_t)m_MaxLength;
}

void MeterStringEdit::SetInputRegExp(const std::wstring& pattern)
{
	// A dynamic InputRegExp is re-read on every update, so an unchanged pattern must keep whatever
	// compiling it already produced: recompiling would cost the same work every update, and would
	// report a pattern that does not compile over and over.
	if (pattern == m_InputRegExpPattern) return;

	m_InputRegExpPattern = pattern;
	m_RegExp.reset();
	m_RegExpError = false;
}

void MeterStringEdit::CompileInputRegExp()
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

std::wstring MeterStringEdit::PreviewReplacement(const std::wstring& text, std::wstring& insert) const
{
	const UINT32 start = GetSelectionStart();
	const UINT32 end = GetSelectionEnd();

	insert = text;

	// UPPER and LOWER convert each character on its own, so the insert can be converted before it
	// is spliced in, leaving text the user did not type alone.
	ApplyCase(insert, m_InputCase);

	if (m_MaxLength > 0 && !insert.empty())
	{
		// What the selection leaves behind is what the limit has to accommodate. An existing text
		// already over the limit (from the Text option, which is not truncated) leaves no room.
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

bool MeterStringEdit::AcceptsReplacement(const std::wstring& text) const
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

void MeterStringEdit::ReplaceSelection(const std::wstring& text)
{
	const UINT32 start = GetSelectionStart();

	std::wstring insert;
	m_Text = PreviewReplacement(text, insert);
	SyncDrawnString();

	m_SelectionAnchor = m_CaretPos = start + (UINT32)insert.length();
	m_CaretTrailing = true;
	m_Committed = false;

	// Before EnsureCaretVisible(), which measures the caret against the meter box: on an auto-sized
	// meter that box is what the text just made it, so scrolling first would compare the new caret
	// against the width of the previous text and scroll a field that in fact fits.
	UpdateAutoSizeForText();

	EnsureCaretVisible();
	m_CaretBlinkStart = GetTickCount64();
}

bool MeterStringEdit::CopySelection(bool cut)
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

bool MeterStringEdit::Paste()
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

	// An empty paste is not an edit, and must not swallow the selection. Neither must one the
	// filter refuses, which it does whole rather than by dropping the characters it dislikes.
	if (text.empty() || !AcceptsReplacement(text)) return false;

	PushUndo(EditKind::None);
	ReplaceSelection(text);
	return true;
}

void MeterStringEdit::DeleteWord(bool forward)
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

void MeterStringEdit::DeleteSelectionOr(bool forward)
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

bool MeterStringEdit::HandleChar(WCHAR ch)
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

bool MeterStringEdit::HandleKeyDown(WPARAM key, bool ctrl, bool shift)
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
		// A committing meter is single-line and the skin handles the commit. Shift+Enter still
		// adds a line.
		if (!shift && CommitsOnEnter()) return false;

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

void MeterStringEdit::DrawCaret(Gfx::Canvas& canvas)
{
	m_CaretDrawnVisible = IsCaretVisible();
	if (!m_CaretDrawnVisible) return;

	DWORD caretWidth = 1UL;
	if (!SystemParametersInfo(SPI_GETCARETWIDTH, 0U, &caretWidth, 0U) || caretWidth == 0UL)
	{
		caretWidth = 1UL;
	}

	D2D1_RECT_F caretRect = { 0 };
	if (canvas.GetCaretRect(m_String, *m_TextFormat, GetTextRect(),
		m_CaretPos, m_CaretTrailing, (FLOAT)caretWidth, caretRect))
	{
		canvas.FillRectangle(caretRect, m_CaretColor);
	}
}

void MeterStringEdit::DrawSelection(Gfx::Canvas& canvas)
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

