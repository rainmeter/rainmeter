/*
  Copyright (C) 2001 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __METERSTRING_H__
#define __METERSTRING_H__

#include "Meter.h"
#include "Measure.h"
#include <unordered_map>

class CMeterString : public CMeter
{
public:
	CMeterString(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeterString();

	virtual UINT GetTypeID() { return TypeID<CMeterString>(); }

	virtual int GetX(bool abs = false);
	virtual int GetY(bool abs = false);

	virtual void Initialize();
	virtual bool Update();
	void SetText(const WCHAR* text) { m_Text = text; }
	virtual bool Draw(Gdiplus::Graphics& graphics);
	Gdiplus::RectF GetRect() { return m_Rect; }

	static void FreeFontCache(Gdiplus::PrivateFontCollection* collection = NULL);
	static void EnumerateInstalledFontFamilies();

protected:
	virtual void ReadOptions(CConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(CConfigParser& parser, const WCHAR* section);

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

	bool DrawString(Gdiplus::Graphics& graphics, Gdiplus::RectF* rect);

	Gdiplus::Color m_Color;
	Gdiplus::Color m_EffectColor;
	std::wstring m_Postfix;
	std::wstring m_Prefix;
	std::wstring m_Text;
	std::wstring m_FontFace;
	AUTOSCALE m_AutoScale;
	METER_ALIGNMENT m_Align;
	TEXTSTYLE m_Style;
	TEXTEFFECT m_Effect;
	TEXTCASE m_Case;
	int m_FontSize;
	double m_Scale;
	bool m_NoDecimals;
	bool m_Percentual;
	bool m_ClipString;
	Gdiplus::Font* m_Font;
	Gdiplus::FontFamily* m_FontFamily;
	int m_NumOfDecimals;
	Gdiplus::REAL m_Angle;
	Gdiplus::RectF m_Rect;

	std::wstring m_String;

	static std::wstring FontFaceToString(const std::wstring& fontFace, Gdiplus::PrivateFontCollection* collection);
	static std::wstring FontPropertiesToString(Gdiplus::REAL size, Gdiplus::FontStyle style);

	static std::unordered_map<std::wstring, Gdiplus::FontFamily*> c_FontFamilies;		// Cache for the font families
	static std::unordered_map<std::wstring, Gdiplus::Font*> c_Fonts;					// Cache for the fonts
};

#endif
