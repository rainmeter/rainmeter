// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "MeterStringBase.h"

class Pcre;

// A text meter the user can type into. Clicking it places a caret, after which the skin routes
// keyboard input here. The text is rendered verbatim so that a caret offset is also an offset into
// the text being edited, which is why none of String's formatting options exist here. StringCase is
// the exception: it converts the text itself rather than only its rendering, so offsets still line
// up and what the skin reads back through [$Input] is what is on screen. InputCase is the same
// conversion narrowed to what the user types or pastes, and to the cases that work per character.
// Password is the other exception, and keeps the offsets in the same way: it draws one mask
// character per UTF-16 unit, so the two strings stay the same length even where they differ.
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

	bool IsSubmitKey(WPARAM key) const;

	// Runs OnSubmitAction, with [$Input] expanded. Nothing may be touched afterwards: the action may
	// refresh or close the skin, which destroys this meter, so the caller redraws before calling.
	void Submit();

	// OnFocusAction, with [$Input] expanded, or empty if there is none.
	std::wstring GetFocusCommand();

	// The counterpart for moving away from the field without submitting: fills |command| with
	// OnDismissAction. Leaves it empty when the text standing in the field is what a submit
	// already sent.
	void HandleDismiss(std::wstring& command);

	// Focus (see also Skin, which owns which meter currently holds the caret).
	bool IsFocused() const { return m_Focused; }
	void SetFocus(bool focus);

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
	void SelectRange(int start, int length = -1);

	void Clear();
	void Reset();

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
		// m_Text and not m_String, which under Password holds the mask rather than the text
		// ApplySnapshot() restores.
		std::wstring text;
		UINT32 caret;
		UINT32 anchor;
		bool trailing;
	};

	void ApplyTextTransformations(std::wstring& text) const;

	// Rebuilds the drawn string from the edited one. Identical to it, unless Password replaces it
	// with a mask - one mask character per UTF-16 unit, so that every offset into one is still an
	// offset into the other and nothing that measures or hit-tests the text has to know.
	void SyncDrawnString();

	// The caret index one step from |pos|, or |false| where there is nowhere to step. Wraps the
	// canvas, which reports clusters in the drawn string: those are the wrong thing to step by
	// while it is masked, where each unit draws as its own mask character.
	bool GetAdjacentCaretIndex(UINT32 pos, bool forward, UINT32& adjacent);

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

	// UpdateAutoSize() against whichever of the two texts is on screen. Every path that changes the
	// text goes through this rather than the base directly, or an edit that empties the field would
	// size it to the empty string while the placeholder is what gets drawn.
	void UpdateAutoSizeForText();

	// A Gfx::TextFormat caches a single layout, so the placeholder needs its own or every frame
	// that alternates between the two would rebuild one. Allocated only while there is one.
	void UpdatePlaceholderFormat();

	// Picks the mask character: the first the font can draw itself, so that masking does not send
	// the whole line to a fallback font and move the baseline with it. Only where there is a mask
	// to draw, and only when the font changes, since nothing else can change the answer.
	void UpdatePasswordChar();

	UINT32 GetSelectionStart() const { return min(m_CaretPos, m_SelectionAnchor); }
	UINT32 GetSelectionEnd() const { return max(m_CaretPos, m_SelectionAnchor); }
	bool HasSelection() const { return m_CaretPos != m_SelectionAnchor; }

	// Moves the caret, collapsing the selection onto it unless |extend| is set. |trailing| says
	// which text the caret belongs to where opposing directions meet: set it when the caret
	// arrived from the left of |pos| in logical order, clear it when it arrived from the right.
	void MoveCaretTo(UINT32 pos, bool extend, bool trailing);

	// Records the pattern InputFilter resolved to, discarding what was compiled from the previous
	// one. Does not compile it: CompileInputRegExp() does, once the field is focused.
	void SetInputRegExp(const std::wstring& pattern);

	// Compiles the recorded pattern, unless it is already compiled or already known not to. Leaves
	// the field unfiltered when it does not compile, rather than refusing everything typed into it.
	void CompileInputRegExp();

	// The text the field would hold if |text| replaced the selection, with |insert| left holding
	// the part of |text| that would actually land, after InputCase and MaxLength.
	std::wstring PreviewReplacement(const std::wstring& text, std::wstring& insert) const;

	// |true| when InputFilter would allow that replacement. Checked by the callers that insert,
	// before they touch the undo stack, so that a refused edit leaves no trace of itself.
	// Deletions bypass it: a filter that refused one could leave text impossible to erase.
	bool AcceptsReplacement(const std::wstring& text) const;

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

	// Set by a submit and cleared by the next edit, so that leaving a field whose contents have
	// already been submitted is not also treated as abandoning them.
	bool m_Submitted;

	int m_MaxLength;
	bool m_Multiline;
	bool m_SubmitOnEnter;

	bool m_Password;
	WCHAR m_PasswordChar;

	// What InputFilter resolved to, empty when there is none. Bounds what the user may enter and
	// only that, unlike MaxLength and Multiline: text that arrived through the Text option is left
	// as it was written, so a filter set after the fact does not rewrite it.
	std::wstring m_InputRegExpPattern;
	std::unique_ptr<Pcre> m_RegExp;
	bool m_RegExpError;

	// Converts what the user contributes - typing, paste, drops - and nothing else, so text that
	// arrived through the Text option keeps the case it was written in. StringCase, held by the
	// base, converts the whole text instead, and is applied after this one. Never TEXTCASE_PROPER,
	// which only makes sense over whole words.
	TEXTCASE m_InputCase;

	bool m_CaretDrawnVisible;
	D2D1_COLOR_F m_CaretColor;
	D2D1_COLOR_F m_SelectionColor;

	// Transparent turns the focus border off, which is the default.
	D2D1_COLOR_F m_FocusBorderColor;
	FLOAT m_FocusBorderWidth;

	// Tells a genuine Text change apart from a re-read that would discard what the user typed.
	std::wstring m_TextOption;

	// Run by Enter and by !TextEdit:Submit.
	std::wstring m_OnSubmitAction;

	// Run when the field takes the caret.
	std::wstring m_OnFocusAction;

	// Run when the field is left without submitting: Escape, a click elsewhere, or another window
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

	// Offset into m_String, and equally into m_Text: the two are the same length here, whether or
	// not Password has replaced one with a mask.
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
