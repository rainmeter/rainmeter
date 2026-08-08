// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterStringEdit.h"
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

}  // namespace

MeterStringEdit::MeterStringEdit(Skin* skin, const WCHAR* name) : MeterStringBase(skin, name),
	m_AcceptsInput(true),
	m_Focused(false),
	m_ClearOnFocus(false),
	m_SelectAllOnFocus(false),
	m_ClearOnEnter(false),
	m_ClearOnDismiss(false),
	m_MaxLength(0),
	m_CaretDrawnVisible(false),
	m_CaretColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_SelectionColor(D2D1::ColorF(D2D1::ColorF::SteelBlue, 0.5f)),
	m_FocusBorderColor(D2D1::ColorF(D2D1::ColorF::Black, 0.0f)),
	m_FocusBorderWidth(1.0f),
	m_CaretPos(0U),
	m_SelectionAnchor(0U),
	m_CaretTrailing(false),
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

	m_ClearOnFocus = parser.ReadBool(section, L"ClearOnFocus", false);
	m_SelectAllOnFocus = parser.ReadBool(section, L"SelectAllOnFocus", false);
	m_ClearOnEnter = parser.ReadBool(section, L"ClearOnEnter", false);
	m_ClearOnDismiss = parser.ReadBool(section, L"ClearOnDismiss", false);
	m_MaxLength = parser.ReadInt(section, L"MaxLength", 0);

	// Read without measure replacement so that it resolves when the action runs rather than when
	// the option is read, which is what lets it reference [$Input].
	m_OnEnterAction = parser.ReadString(section, L"OnEnterAction", L"", false);
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

bool MeterStringEdit::Update()
{
	if (Meter::Update())
	{
		// Rendered verbatim: any rewrite would shift the string relative to m_Text and put the
		// caret offsets DirectWrite reports in a different index space than the edited text.
		m_String = m_Text;

		const UINT32 len = (UINT32)m_String.length();
		m_CaretPos = min(m_CaretPos, len);
		m_SelectionAnchor = min(m_SelectionAnchor, len);

		UpdateTextFormat();

		// An empty auto-sized field would collapse to nothing and become unclickable, so it takes
		// its size from the placeholder instead.
		if (ShowingPlaceholder())
		{
			UpdateAutoSize(&m_Placeholder, m_PlaceholderFormat.get());
		}
		else
		{
			UpdateAutoSize();
		}

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
		drawn = DrawString(canvas, nullptr, &m_Placeholder, m_PlaceholderFormat.get());
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

	if (!focus) return;

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

	return true;
}

void MeterStringEdit::HandleDismiss(std::wstring& command)
{
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

	m_UndoStack.push_back({ m_String, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
	m_LastEditKind = kind;
}

void MeterStringEdit::ApplySnapshot(const EditSnapshot& snapshot)
{
	m_Text = snapshot.text;
	m_String = m_Text;

	const UINT32 len = (UINT32)m_String.length();
	m_CaretPos = min(snapshot.caret, len);
	m_CaretTrailing = snapshot.trailing;
	m_SelectionAnchor = min(snapshot.anchor, len);
	m_CaretBlinkStart = GetTickCount64();

	UpdateAutoSize();
}

bool MeterStringEdit::Undo()
{
	if (m_UndoStack.empty()) return false;

	m_RedoStack.push_back({ m_String, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
	ApplySnapshot(m_UndoStack.back());
	m_UndoStack.pop_back();

	m_LastEditKind = EditKind::None;
	return true;
}

bool MeterStringEdit::Redo()
{
	if (m_RedoStack.empty()) return false;

	m_UndoStack.push_back({ m_String, m_CaretPos, m_SelectionAnchor, m_CaretTrailing });
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

void MeterStringEdit::ReplaceSelection(const std::wstring& text)
{
	const UINT32 start = GetSelectionStart();
	const UINT32 end = GetSelectionEnd();

	std::wstring insert = text;
	if (m_MaxLength > 0 && !insert.empty())
	{
		// What the selection leaves behind is what the limit has to accommodate. An existing text
		// already over the limit (from the Text option, which is not truncated) leaves no room.
		const size_t kept = m_String.length() - (end - start);
		const size_t room = kept < (size_t)m_MaxLength ? (size_t)m_MaxLength - kept : 0U;

		if (insert.length() > room)
		{
			insert.resize(room);

			// Truncating must not leave a high surrogate without its pair.
			if (!insert.empty() && IS_HIGH_SURROGATE(insert.back())) insert.pop_back();
		}
	}

	m_Text = m_String;
	m_Text.replace(start, end - start, insert);
	m_String = m_Text;

	m_SelectionAnchor = m_CaretPos = start + (UINT32)insert.length();
	m_CaretTrailing = true;
	EnsureCaretVisible();
	m_CaretBlinkStart = GetTickCount64();

	UpdateAutoSize();
}

bool MeterStringEdit::CopySelection(bool cut)
{
	if (!HasSelection()) return false;

	const UINT32 start = GetSelectionStart();
	System::SetClipboardText(m_String.substr(start, GetSelectionEnd() - start));

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
		Gfx::Canvas& canvas = m_Skin->GetCanvas();
		ApplyTextState(canvas);

		UINT32 adjacent = m_CaretPos;
		if (!canvas.GetAdjacentCaretIndex(
			m_String, *m_TextFormat, GetTextRect(), m_CaretPos, forward, adjacent))
		{
			return;
		}

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

	// Replacing a selection is a distinct step from the typing that follows it, so it does not
	// fold into a preceding run.
	PushUndo(HasSelection() ? EditKind::None : EditKind::Typing);
	ReplaceSelection(std::wstring(1U, ch));
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

			ApplyTextState(canvas);

			UINT32 adjacent = m_CaretPos;
			if (canvas.GetAdjacentCaretIndex(
				m_String, *m_TextFormat, GetTextRect(), m_CaretPos, forward, adjacent))
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

