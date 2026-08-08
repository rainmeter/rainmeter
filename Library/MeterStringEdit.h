// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "MeterStringBase.h"

// A text meter the user can type into. Clicking it places a caret, after which the skin routes
// keyboard input here. The text is rendered verbatim so that a caret offset is also an offset into
// the text being edited, which is why none of String's formatting options exist here.
class MeterStringEdit : public MeterStringBase
{
public:
	MeterStringEdit(Skin* skin, const WCHAR* name);
	virtual ~MeterStringEdit();

	MeterStringEdit(const MeterStringEdit& other) = delete;
	MeterStringEdit& operator=(MeterStringEdit other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterStringEdit>(); }

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

	// |false| when configured in a way editing cannot support: still draws, takes no input.
	bool AcceptsInput() const { return m_AcceptsInput; }

	// Applies ClearOnEnter and fills |command| with OnEnterAction ([$Input] already expanded,
	// empty if there is none). Returns false when Enter is not a commit here, leaving it to be
	// typed as a newline.
	bool HandleEnter(std::wstring& command);

	// OnFocusAction, with [$Input] expanded, or empty if there is none. Read after SetFocus(true),
	// so [$Input] carries what the field holds once ClearOnFocus has had its say.
	std::wstring GetFocusCommand();

	// The counterpart for moving away from the field without committing: applies ClearOnDismiss
	// and fills |command| with OnDismissAction.
	void HandleDismiss(std::wstring& command);

	// Focus (see also Skin, which owns which meter currently holds the caret).
	bool IsFocused() const { return m_Focused; }
	void SetFocus(bool focus);

	// |true| when taking focus decides the selection itself, so the click that focused the meter
	// must not then place the caret and collapse it.
	bool FocusOverridesCaret() const { return m_ClearOnFocus || m_SelectAllOnFocus; }

	// Lets the caret timer skip its whole-skin redraw on ticks where the phase did not flip.
	bool NeedsCaretRedraw() const
	{
		// A hidden meter draws no caret at all, so its recorded phase would never catch up.
		return m_Focused && !m_Hidden && IsCaretVisible() != m_CaretDrawnVisible;
	}

	// Moves the caret to the character nearest |x|,|y|. |extend| keeps the selection anchor, as a
	// shift-click or a drag does.
	bool SetCaretFromPoint(int x, int y, bool extend = false);

	// Routed here by the skin while this meter holds the caret. Return true if the input was
	// consumed and a redraw is needed.
	bool HandleChar(WCHAR ch);
	bool HandleKeyDown(WPARAM key, bool ctrl, bool shift);

	// Selects the word (or, failing that, the run of whitespace) around the caret.
	bool SelectWordAtCaret();
	bool SelectLineAtCaret();
	void SelectAll();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	// What the last edit was, so that a run of the same kind collapses into one undo step instead
	// of making the user undo a burst of typing one character at a time.
	enum class EditKind : BYTE
	{
		None,
		Typing,
		Deleting
	};

	struct EditSnapshot
	{
		std::wstring text;
		UINT32 caret;
		UINT32 anchor;
		bool trailing;
	};

	// |true| when Enter commits rather than inserting a newline, which is what makes the meter
	// single-line. Either "on enter" option implies it; Shift+Enter always inserts.
	bool CommitsOnEnter() const { return !m_OnEnterAction.empty() || m_ClearOnEnter; }

	void Clear();

	// Fills |command| with |action|, [$Input] expanded. Leaves it empty if there is no action.
	void ExpandAction(const std::wstring& action, std::wstring& command);

	void DrawCaret(Gfx::Canvas& canvas);
	void DrawSelection(Gfx::Canvas& canvas);

	// Frames the meter while it holds the caret, around the same box SolidColor fills.
	void DrawFocusBorder(Gfx::Canvas& canvas);

	// Scrolls the text inside a meter too small to show it all, so the caret stays visible. Does
	// nothing while everything fits, which is always the case for an auto-sized meter.
	void EnsureCaretVisible();

	bool ShowingPlaceholder() const { return m_PlaceholderFormat && m_String.empty(); }

	// A Gfx::TextFormat caches a single layout, so the placeholder needs its own or every frame
	// that alternates between the two would rebuild one. Allocated only while there is one.
	void UpdatePlaceholderFormat();

	UINT32 GetSelectionStart() const { return min(m_CaretPos, m_SelectionAnchor); }
	UINT32 GetSelectionEnd() const { return max(m_CaretPos, m_SelectionAnchor); }
	bool HasSelection() const { return m_CaretPos != m_SelectionAnchor; }

	// Moves the caret, collapsing the selection onto it unless |extend| is set. |trailing| says
	// which text the caret belongs to where opposing directions meet: set it when the caret
	// arrived from the left of |pos| in logical order, clear it when it arrived from the right.
	void MoveCaretTo(UINT32 pos, bool extend, bool trailing);

	// Replaces the selection, or inserts at the caret, leaving the caret after |text|. Truncates
	// |text| to whatever MaxLength leaves room for.
	void ReplaceSelection(const std::wstring& text);

	// |true| when MaxLength leaves no room to insert without first replacing a selection.
	bool IsFull() const;

	// Deletes the selection, or one cluster to either side of the caret when there is none.
	void DeleteSelectionOr(bool forward);

	// Deletes the selection, or from the caret to the neighbouring word boundary.
	void DeleteWord(bool forward);

	// Start of the next word (or of the word to the left), as Ctrl+Right/Ctrl+Left move.
	UINT32 FindWordBoundary(UINT32 pos, bool forward) const;

	// Clipboard. Copying alone changes nothing on screen, so only cutting reports a redraw.
	bool CopySelection(bool cut);
	bool Paste();

	// Snapshots are taken before an edit; |kind| lets consecutive edits of a kind share a step.
	void PushUndo(EditKind kind);
	void ApplySnapshot(const EditSnapshot& snapshot);
	bool Undo();
	bool Redo();
	void ClearUndoHistory();

	// Always |true| when caret blinking is turned off system-wide.
	bool IsCaretVisible() const;

	bool m_AcceptsInput;
	bool m_Focused;
	bool m_ClearOnFocus;
	bool m_SelectAllOnFocus;
	bool m_ClearOnEnter;
	bool m_ClearOnDismiss;

	// Longest the text may become, in UTF-16 units, as Win32 edit controls also count it. Zero or
	// less is unlimited. Only bounds what the user enters, not what Text starts as.
	int m_MaxLength;

	bool m_CaretDrawnVisible;
	D2D1_COLOR_F m_CaretColor;
	D2D1_COLOR_F m_SelectionColor;

	// Transparent turns the focus border off, which is the default.
	D2D1_COLOR_F m_FocusBorderColor;
	FLOAT m_FocusBorderWidth;

	// Tells a genuine Text change apart from a re-read that would discard what the user typed.
	std::wstring m_TextOption;

	// Having one makes the meter single-line; see CommitsOnEnter().
	std::wstring m_OnEnterAction;

	// Run when the field takes the caret.
	std::wstring m_OnFocusAction;

	// Run when the field is left without committing: Escape, a click elsewhere, or another window
	// taking focus.
	std::wstring m_OnDismissAction;

	// Drawn in place of the text while nothing has been typed. Never enters m_Text, so it cannot
	// reach [$Input] and the caret can never index into it.
	std::wstring m_Placeholder;
	D2D1_COLOR_F m_PlaceholderColor;
	std::wstring m_PlaceholderFontFace;
	FLOAT m_PlaceholderFontSize;
	TEXTSTYLE m_PlaceholderStyle;

	// Allocated only while m_Placeholder is non-empty.
	std::unique_ptr<Gfx::TextFormat> m_PlaceholderFormat;

	// Offset into m_String, which here is identical to m_Text.
	UINT32 m_CaretPos;

	// The end the caret is moving away from. Equal to m_CaretPos when nothing is selected.
	UINT32 m_SelectionAnchor;

	// Which side of m_CaretPos to draw on. Only visible where opposing directions meet, where one
	// index has two visual positions.
	bool m_CaretTrailing;

	// Reset whenever the caret moves, so it is solid right after a click.
	ULONGLONG m_CaretBlinkStart;

	std::vector<EditSnapshot> m_UndoStack;
	std::vector<EditSnapshot> m_RedoStack;
	EditKind m_LastEditKind;
};
