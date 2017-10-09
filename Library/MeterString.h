/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERSTRING_H__
#define __METERSTRING_H__

#include "Meter.h"
#include "Measure.h"

class MeterString : public Meter
{
public:
	MeterString(Skin* skin, const WCHAR* name);
	virtual ~MeterString();

	MeterString(const MeterString& other) = delete;
	MeterString& operator=(MeterString other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterString>(); }

	virtual int GetX(bool abs = false);
	virtual int GetY(bool abs = false);

	virtual void Initialize();
	virtual bool Update();
	void SetText(const WCHAR* text) { m_Text = text; }
	virtual bool Draw(Gfx::Canvas& canvas);
	Gdiplus::RectF GetRect() { return m_Rect; }

	static void EnumerateInstalledFontFamilies();

	static void InitializeStatic();
	static void FinalizeStatic();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return overwrite; }

private:
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

	enum TEXTCASE
	{
		TEXTCASE_NONE,
		TEXTCASE_UPPER,
		TEXTCASE_LOWER,
		TEXTCASE_PROPER
	};

	enum CLIPTYPE
	{
		CLIP_OFF,
		CLIP_ON,
		CLIP_AUTO
	};

	bool DrawString(Gfx::Canvas& canvas, Gdiplus::RectF* rect);

	Gdiplus::Color m_Color;
	Gdiplus::Color m_EffectColor;
	std::wstring m_Postfix;
	std::wstring m_Prefix;
	std::wstring m_Text;
	std::wstring m_FontFace;
	AUTOSCALE m_AutoScale;
	TEXTSTYLE m_Style;
	TEXTEFFECT m_Effect;
	TEXTCASE m_Case;
	int m_FontSize;
	double m_Scale;
	bool m_NoDecimals;
	bool m_Percentual;
	CLIPTYPE m_ClipType;
	bool m_NeedsClipping;
	int m_ClipStringW;
	int m_ClipStringH;
	Gfx::TextFormat* m_TextFormat;
	int m_NumOfDecimals;
	Gdiplus::REAL m_Angle;
	Gdiplus::RectF m_Rect;

	int m_FontWeight;

	std::wstring m_String;
};

#endif
