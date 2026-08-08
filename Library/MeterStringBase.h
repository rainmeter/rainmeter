// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "Measure.h"
#include <memory>

// Shared base for String, which formats a measure's value, and StringEdit, which lets the user
// edit the text. Holds the font options common to both and the drawing built on them. Each meter
// reads the Text option for itself, since they disagree on who owns its value.
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

	void SetText(const WCHAR* text) { m_Text = text; }

	// The text before any of String's formatting. For StringEdit this is what the user typed.
	const std::wstring& GetText() const { return m_Text; }

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

	MeterStringBase(Skin* skin, const WCHAR* name);

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return overwrite; }

	// Returns |defaultStyle| when the option is unset, and logs an unrecognised value.
	TEXTSTYLE ReadStringStyle(ConfigParser& parser, const WCHAR* section, const WCHAR* option,
		TEXTSTYLE defaultStyle);

	// Draws m_String, or measures it into |rect| when one is given. |str| and |format| override
	// what is drawn, so a subclass can render a second piece of text (StringEdit's placeholder)
	// through the same layout and clipping rules.
	bool DrawString(Gfx::Canvas& canvas, D2D1_RECT_F* rect,
		const std::wstring* str = nullptr, Gfx::TextFormat* format = nullptr);

	// Anything that hit-tests the layout has to go through this too, or what it measures is not
	// what is on screen.
	void ApplyTextState(Gfx::Canvas& canvas, Gfx::TextFormat* format = nullptr);

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
	bool m_NeedsClipping;
	int m_ClipStringW;
	int m_ClipStringH;
	std::unique_ptr<Gfx::TextFormat> m_TextFormat;
	FLOAT m_Angle;
	int m_FontWeight;

	// The text as drawn. String rewrites it from m_Text and its formatting options; StringEdit
	// keeps it identical to m_Text, so a caret offset is also an offset into the edited text.
	std::wstring m_String;
};
