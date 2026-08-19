// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "Measure.h"
#include <memory>
#include <optional>
#include <string_view>

// Shared base for String, which formats a measure's value, and TextEdit, which lets the user
// edit the text. Holds the font and case options common to both and the drawing built on them, but
// leaves it to each meter to decide when the case conversion runs. Neither the text nor the option
// it comes from is read here: String rebuilds m_Text from Text on every read, while TextEdit
// follows InitialText only until the field is touched, and leaves the text to the user after.
class __declspec(novtable) MeterStringBase : public Meter
{
public:
	virtual ~MeterStringBase();

	MeterStringBase(const MeterStringBase& other) = delete;
	MeterStringBase& operator=(MeterStringBase other) = delete;

	virtual int GetX(bool abs = false);
	virtual int GetY(bool abs = false);

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;

	// The text before any of String's formatting. For TextEdit this is what the user typed.
	const std::wstring& GetText() const { return m_Text; }
	virtual void SetText(std::wstring_view text) { m_Text = text; }

	static void InitializeStatic();
	static void FinalizeStatic();

protected:
	enum TEXTSTYLE
	{
		NORMAL,
		BOLD,
		ITALIC,
		BOLDITALIC
	};

	enum TEXTEFFECT
	{
		EFFECT_NONE,
		EFFECT_SHADOW,
		EFFECT_BORDER
	};

	enum CLIPTYPE
	{
		CLIP_OFF,
		CLIP_ON,
		CLIP_AUTO
	};

	enum TEXTCASE
	{
		TEXTCASE_NONE,
		TEXTCASE_UPPER,
		TEXTCASE_LOWER,
		TEXTCASE_PROPER
	};

	MeterStringBase(Skin* skin, const WCHAR* name);

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return overwrite; }

	// Applies |textCase| to |text| in place. The conversions map each UTF-16 unit onto one unit, so
	// offsets into the text survive them, which is what lets TextEdit convert what it edits rather
	// than only what it draws. Left to the subclass to call: String converts the finished m_String,
	// TextEdit the text itself.
	void ApplyCase(std::wstring& text, TEXTCASE textCase) const;
	void ApplyCase(std::wstring& text) const { ApplyCase(text, m_Case); }

	// Returns |defaultCase| when the option is unset, and logs an unrecognised value.
	TEXTCASE ReadStringCase(ConfigParser& parser, const WCHAR* section, const WCHAR* option,
		TEXTCASE defaultCase);

	// Returns |defaultStyle| when the option is unset, and logs an unrecognised value.
	TEXTSTYLE ReadStringStyle(ConfigParser& parser, const WCHAR* section, const WCHAR* option,
		TEXTSTYLE defaultStyle);

	// The bounds m_String needs, or nothing when it cannot be measured. Also decides, for CLIP_AUTO,
	// whether the text needs clipping. |str| and |format| override what is measured, so a subclass
	// can size itself from a second piece of text through the same rules.
	std::optional<D2D1_RECT_F> MeasureStringBounds(Gfx::Canvas& canvas, const std::wstring* str = nullptr, Gfx::TextFormat* format = nullptr);

	// Draws m_String. |str|, |format| and |color| override what is drawn, so a subclass can render a
	// second piece of text through the same layout and clipping rules. Measure first: what is drawn
	// is laid out in the meter's rect, which auto-sizing takes from MeasureStringBounds().
	bool DrawString(Gfx::Canvas& canvas, const std::wstring* str = nullptr, Gfx::TextFormat* format = nullptr, const D2D1_COLOR_F* color = nullptr);

	// Anything that hit-tests the layout has to go through this too, or what it measures is not
	// what is on screen.
	void ApplyTextState(Gfx::Canvas& canvas, Gfx::TextFormat* format = nullptr);

	// |true| when the text is bound to the meter box at all, and so is measured, laid out and drawn
	// against it rather than being allowed to size the meter to itself.
	bool ShouldClip() const;

	// |true| when the lines may break to the meter's width instead of running past it. A meter that
	// cannot wrap answers a line too long for it along the x axis instead, by being cut there and by
	// scrolling where it can. Single-line TextEdit is the only one, and matches the Win32 edit
	// control it stands in for, which never turns one line into several.
	virtual bool CanWrap() const { return true; }

	// |true| when the text that still does not fit is cut short with an ellipsis. String says so
	// whenever it clips, having no other way to show that there is more text than fits. TextEdit
	// never does: its text can be scrolled to instead.
	virtual bool ShouldTrim() const { return ShouldClip(); }

	// The text box shifted by m_TextOffset. Everything that draws or hit-tests the text uses this
	// rather than GetMeterRectPadding(), so scrolled text and the positions reported for it stay
	// in the same space.
	D2D1_RECT_F GetTextRect();

	// Call after building m_String, before measuring or drawing it.
	void UpdateTextFormat();

	// Recomputes whichever of m_W/m_H is not fixed by an option.
	void UpdateAutoSize(const std::wstring* str = nullptr, Gfx::TextFormat* format = nullptr);

	D2D1_COLOR_F m_Color;
	D2D1_COLOR_F m_EffectColor;
	std::wstring m_Text;
	std::wstring m_FontFace;
	TEXTSTYLE m_Style;
	TEXTEFFECT m_Effect;
	FLOAT m_FontSize;
	CLIPTYPE m_ClipType;
	TEXTCASE m_Case;
	bool m_NeedsClipping;
	int m_ClipStringW;
	int m_ClipStringH;
	std::unique_ptr<Gfx::TextFormat> m_TextFormat;
	int m_FontWeight;

	// How far the text is scrolled inside the meter, for a field whose text outgrew it. Zero for
	// String, which has no way to scroll.
	D2D1_POINT_2F m_TextOffset;

	// The text as drawn. String rewrites it from m_Text and its formatting options; TextEdit
	// keeps it the same length as m_Text - identical to it, or a mask of it - so that a caret
	// offset is also an offset into the edited text.
	std::wstring m_String;
};
