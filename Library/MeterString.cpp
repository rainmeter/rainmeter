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

#include "StdAfx.h"
#include "MeterString.h"
#include "Rainmeter.h"
#include "Measure.h"
#include "Error.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

#define PI	(3.14159265f)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0f / PI))

void StringToUpper(std::wstring& str)
{
	WCHAR* srcAndDest = &str[0];
	int strAndDestLen = (int)str.length();
	LCMapString(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, srcAndDest, strAndDestLen, srcAndDest, strAndDestLen);
}

void StringToLower(std::wstring& str)
{
	WCHAR* srcAndDest = &str[0];
	int strAndDestLen = (int)str.length();
	LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, srcAndDest, strAndDestLen, srcAndDest, strAndDestLen);
}

void StringToProper(std::wstring& str)
{
	if (!str.empty())
	{
		WCHAR* srcAndDest = &str[0];
		LCMapString(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, srcAndDest, 1, srcAndDest, 1);

		for (size_t i = 1; i < str.length(); ++i)
		{
			srcAndDest = &str[i];
			LCMapString(LOCALE_USER_DEFAULT, (iswspace(str[i - 1]) > 0) ? LCMAP_UPPERCASE : LCMAP_LOWERCASE, srcAndDest, 1, srcAndDest, 1);
		}
	}
}

/*
** The constructor
**
*/
MeterString::MeterString(MeterWindow* meterWindow, const WCHAR* name) : Meter(meterWindow, name),
	m_Color(Color::White),
	m_EffectColor(Color::Black),
	m_AutoScale(AUTOSCALE_OFF),
	m_Style(NORMAL),
	m_Effect(EFFECT_NONE),
	m_Case(TEXTCASE_NONE),
	m_FontSize(10),
	m_Scale(1.0),
	m_NoDecimals(true),
	m_Percentual(true),
	m_ClipType(CLIP_OFF),
	m_NeedsClipping(false),
	m_ClipStringW(-1),
	m_ClipStringH(-1),
	m_TextFormat(meterWindow->GetCanvas().CreateTextFormat()),
	m_NumOfDecimals(-1),
	m_Angle()
{
}

/*
** The destructor
**
*/
MeterString::~MeterString()
{
	delete m_TextFormat;
	m_TextFormat = nullptr;
}

/*
** Returns the X-coordinate of the meter
**
*/
int MeterString::GetX(bool abs)
{
	int x = Meter::GetX();

	if (!abs)
	{
		switch (m_TextFormat->GetHorizontalAlignment())
		{
		case Gfx::HorizontalAlignment::Center:
			x -= m_W / 2;
			break;

		case Gfx::HorizontalAlignment::Right:
			x -= m_W;
			break;
		}
	}

	return x;
}

/*
** Returns the Y-coordinate of the meter
**
*/
int MeterString::GetY(bool abs)
{
	int y = Meter::GetY();

	if (!abs)
	{
		switch (m_TextFormat->GetVerticalAlignment())
		{
		case Gfx::VerticalAlignment::Center:
			y -= m_H / 2;
			break;

		case Gfx::VerticalAlignment::Bottom:
			y -= m_H;
			break;
		}
	}

	return y;
}

/*
** Create the font that is used to draw the text.
**
*/
void MeterString::Initialize()
{
	Meter::Initialize();

	m_TextFormat->SetProperties(
		m_FontFace.c_str(),
		m_FontSize,
		m_Style & BOLD,
		m_Style & ITALIC,
		m_MeterWindow->GetFontCollection());
}

/*
** Read the options specified in the ini file.
**
*/
void MeterString::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	// Store the current font values so we know if the font needs to be updated
	std::wstring oldFontFace = m_FontFace;
	int oldFontSize = m_FontSize;
	TEXTSTYLE oldStyle = m_Style;

	Meter::ReadOptions(parser, section);

	m_Color = parser.ReadColor(section, L"FontColor", Color::Black);
	m_EffectColor = parser.ReadColor(section, L"FontEffectColor", Color::Black);

	m_Prefix = parser.ReadString(section, L"Prefix", L"");
	m_Postfix = parser.ReadString(section, L"Postfix", L"");
	m_Text = parser.ReadString(section, L"Text", L"");

	m_Percentual = parser.ReadBool(section, L"Percentual", false);

	int clipping = parser.ReadInt(section, L"ClipString", 0);
	switch (clipping)
	{
	case 2:
		m_ClipType = CLIP_AUTO;

		m_ClipStringW = parser.ReadInt(section, L"ClipStringW", -1);
		m_ClipStringH = parser.ReadInt(section, L"ClipStringH", -1);
		break;

	case 1:
		m_ClipType = CLIP_ON;
		break;

	case 0:
		m_ClipType = CLIP_OFF;
		break;

	default:
		LogErrorF(this, L"ClipString=%s is not valid", clipping);
	}

	m_FontFace = parser.ReadString(section, L"FontFace", L"Arial");
	if (m_FontFace.empty())
	{
		m_FontFace = L"Arial";
	}

	m_FontSize = parser.ReadInt(section, L"FontSize", 10);
	if (m_FontSize < 0)
	{
		m_FontSize = 10;
	}

	m_NumOfDecimals = parser.ReadInt(section, L"NumOfDecimals", -1);

	m_Angle = (Gdiplus::REAL)parser.ReadFloat(section, L"Angle", 0.0);

	const std::wstring& autoscale = parser.ReadString(section, L"AutoScale", L"0");
	int autoscaleValue = _wtoi(autoscale.c_str());
	if (autoscaleValue == 0)
	{
		m_AutoScale = AUTOSCALE_OFF;
	}
	else
	{
		if (autoscale.find_last_of(L"kK") == std::wstring::npos)
		{
			m_AutoScale = (autoscaleValue == 2) ? AUTOSCALE_1000 : AUTOSCALE_1024;
		}
		else
		{
			m_AutoScale = (autoscaleValue == 2) ? AUTOSCALE_1000K : AUTOSCALE_1024K;
		}
	}

	const std::wstring& scale = parser.ReadString(section, L"Scale", L"1");
	m_NoDecimals = (scale.find(L'.') == std::wstring::npos);
	m_Scale = parser.ParseDouble(scale.c_str(), 1);

	const WCHAR* hAlign = parser.ReadString(section, L"StringAlign", L"LEFT").c_str();
	const WCHAR* vAlign = nullptr;
	if (_wcsnicmp(hAlign, L"LEFT", 4) == 0)
	{
		m_TextFormat->SetHorizontalAlignment(Gfx::HorizontalAlignment::Left);
		vAlign = hAlign + 4;
	}
	else if (_wcsnicmp(hAlign, L"RIGHT", 5) == 0)
	{
		m_TextFormat->SetHorizontalAlignment(Gfx::HorizontalAlignment::Right);
		vAlign = hAlign + 5;
	}
	else if (_wcsnicmp(hAlign, L"CENTER", 6) == 0)
	{
		m_TextFormat->SetHorizontalAlignment(Gfx::HorizontalAlignment::Center);
		vAlign = hAlign + 6;
	}

	if (!vAlign || _wcsicmp(vAlign, L"TOP") == 0)
	{
		m_TextFormat->SetVerticalAlignment(Gfx::VerticalAlignment::Top);
	}
	else if (_wcsicmp(vAlign, L"BOTTOM") == 0)
	{
		m_TextFormat->SetVerticalAlignment(Gfx::VerticalAlignment::Bottom);
	}
	else if (_wcsicmp(vAlign, L"CENTER") == 0)
	{
		m_TextFormat->SetVerticalAlignment(Gfx::VerticalAlignment::Center);
	}

	const WCHAR* stringCase = parser.ReadString(section, L"StringCase", L"NONE").c_str();
	if (_wcsicmp(stringCase, L"NONE") == 0)
	{
		m_Case = TEXTCASE_NONE;
	}
	else if (_wcsicmp(stringCase, L"UPPER") == 0)
	{
		m_Case = TEXTCASE_UPPER;
	}
	else if (_wcsicmp(stringCase, L"LOWER") == 0)
	{
		m_Case = TEXTCASE_LOWER;
	}
	else if (_wcsicmp(stringCase, L"PROPER") == 0)
	{
		m_Case = TEXTCASE_PROPER;
	}
	else
	{
		LogErrorF(this, L"StringCase=%s is not valid", stringCase);
	}

	const WCHAR* style = parser.ReadString(section, L"StringStyle", L"NORMAL").c_str();
	if (_wcsicmp(style, L"NORMAL") == 0)
	{
		m_Style = NORMAL;
	}
	else if (_wcsicmp(style, L"BOLD") == 0)
	{
		m_Style = BOLD;
	}
	else if (_wcsicmp(style, L"ITALIC") == 0)
	{
		m_Style = ITALIC;
	}
	else if (_wcsicmp(style, L"BOLDITALIC") == 0)
	{
		m_Style = BOLDITALIC;
	}
	else
	{
		LogErrorF(this, L"StringStyle=%s is not valid", style);
	}

	const WCHAR* effect = parser.ReadString(section, L"StringEffect", L"NONE").c_str();
	if (_wcsicmp(effect, L"NONE") == 0)
	{
		m_Effect = EFFECT_NONE;
	}
	else if (_wcsicmp(effect, L"SHADOW") == 0)
	{
		m_Effect = EFFECT_SHADOW;
	}
	else if (_wcsicmp(effect, L"BORDER") == 0)
	{
		m_Effect = EFFECT_BORDER;
	}
	else
	{
		LogErrorF(this, L"StringEffect=%s is not valid", effect);
	}

	if (m_Initialized &&
		(wcscmp(oldFontFace.c_str(), m_FontFace.c_str()) != 0 ||
		oldFontSize != m_FontSize ||
		oldStyle != m_Style))
	{
		Initialize();	// Recreate the font
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterString::Update()
{
	if (Meter::Update())
	{
		int decimals = (m_NumOfDecimals != -1) ? m_NumOfDecimals : (m_NoDecimals && (m_Percentual || m_AutoScale == AUTOSCALE_OFF)) ? 0 : 1;

		// Create the text
		m_String = m_Prefix;
		if (!m_Measures.empty())
		{
			if (m_Text.empty())
			{
				m_String += m_Measures[0]->GetStringOrFormattedValue(
					m_AutoScale, m_Scale, decimals, m_Percentual);
			}
			else
			{
				std::wstring tmpText = m_Text;
				ReplaceMeasures(tmpText, m_AutoScale, m_Scale, decimals, m_Percentual);
				m_String += tmpText;
			}
		}
		else
		{
			m_String += m_Text;
		}
		if (!m_Postfix.empty()) m_String += m_Postfix;

		switch (m_Case)
		{
		case TEXTCASE_UPPER:
			StringToUpper(m_String);
			break;
		case TEXTCASE_LOWER:
			StringToLower(m_String);
			break;
		case TEXTCASE_PROPER:
			StringToProper(m_String);
			break;
		}

		// Ugly hack to make D2D render trailing spaces followed by a non-breaking space correctly.
		// By default, D2D ignores all trailing whitespace. Both GDI+ and D2D, however, acknowledge the
		// presense of the zero-width space (and give it a width of 0px), so we append the zero-width
		// space after each non-breaking space.
		size_t pos = 0;
		while (true)
		{
			const WCHAR spaceChars[] =
			{
				L'\u00A0',  // No-Break Space
				L'\u205F'   // Medium Mathematical Space
			};

			pos = m_String.find_first_of(spaceChars, pos, _countof(spaceChars));
			if (pos == std::wstring::npos) break;

			m_String.insert(pos + 1, 1, L'\u200B');
			pos += 2;
		}

		if (!m_WDefined || !m_HDefined)
		{
			// Calculate the text size
			RectF rect;
			if (DrawString(m_MeterWindow->GetCanvas(), &rect))
			{
				m_W = (int)rect.Width + GetWidthPadding();
				m_H = (int)rect.Height + GetHeightPadding();
			}
			else
			{
				m_W = 1;
				m_H = 1;
			}
		}

		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterString::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	return DrawString(canvas, nullptr);
}

/*
** Draws the string or calculates it's size
**
*/
bool MeterString::DrawString(Gfx::Canvas& canvas, RectF* rect)
{
	if (!m_TextFormat->IsInitialized()) return false;

	LPCWSTR string = m_String.c_str();
	UINT stringLen = (UINT)m_String.length();

	canvas.SetTextAntiAliasing(m_AntiAlias);

	m_TextFormat->SetTrimming(
		m_ClipType == CLIP_ON ||
		(m_ClipType == CLIP_AUTO && (m_NeedsClipping || (m_WDefined && m_HDefined))));

	Gdiplus::Rect meterRect = GetMeterRectPadding();

	if (rect)
	{
		rect->X = (REAL)meterRect.X;
		rect->Y = (REAL)meterRect.Y;
		if (canvas.MeasureTextW(string, stringLen, *m_TextFormat, *rect) &&
			m_ClipType == CLIP_AUTO)
		{
			// Set initial clipping
			m_NeedsClipping = false;

			REAL w, h;
			bool updateSize = true;

			if (m_WDefined)
			{
				w = (REAL)meterRect.Width;
				h = rect->Height;
				m_NeedsClipping = true;
			}
			else if (m_HDefined)
			{
				if (m_ClipStringW == -1)
				{
					// Text does not fit in defined height, clip it
					if (rect->Height > (REAL)meterRect.Height)
					{
						m_NeedsClipping = true;
					}

					rect->Height = (REAL)meterRect.Height;
					updateSize = false;

				}
				else
				{
					if (rect->Width > (REAL)m_ClipStringW)
					{
						w = (REAL)m_ClipStringW;
						m_NeedsClipping = true;
					}
					else
					{
						w = rect->Width;
					}

					h = (REAL)meterRect.Height;
				}
			}
			else
			{
				if (m_ClipStringW == -1)
				{
					// Clip text if already larger than ClipStringH
					if (m_ClipStringH != -1 && rect->Height > (REAL)m_ClipStringH)
					{
						m_NeedsClipping = true;
						rect->Height = (REAL)m_ClipStringH;
					}

					updateSize = false;
				}
				else
				{
					if (rect->Width > (REAL)m_ClipStringW)
					{
						w = (REAL)m_ClipStringW;
						m_NeedsClipping = true;
					}
					else
					{
						w = rect->Width;
					}

					h = rect->Height;
				}
			}

			if (updateSize)
			{
				UINT lines = 0;
				RectF layout((REAL)meterRect.X, (REAL)meterRect.Y, w, h);
				if (canvas.MeasureTextLinesW(string, stringLen, *m_TextFormat, layout, lines) &&
					lines != 0)
				{
					rect->Width = w;
					rect->Height = layout.Height;

					if (m_HDefined || (m_ClipStringH != -1 && rect->Height > (REAL)m_ClipStringH))
					{
						rect->Height = m_HDefined ? (REAL)meterRect.Height : (REAL)m_ClipStringH;
					}
				}
			}
		}
	}
	else
	{
		RectF rcDest((REAL)meterRect.X, (REAL)meterRect.Y, (REAL)meterRect.Width, (REAL)meterRect.Height);
		m_Rect = rcDest;

		if (m_Angle != 0.0f)
		{
			const float baseX = (float)Meter::GetX();
			canvas.RotateTransform(CONVERT_TO_DEGREES(m_Angle), baseX, (REAL)meterRect.Y, -baseX, -(REAL)meterRect.Y);
		}

		if (m_Effect != EFFECT_NONE)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(rcDest);

			if (m_Effect == EFFECT_SHADOW)
			{
				rcEffect.Offset(1, 1);
				canvas.DrawTextW(string, (UINT)stringLen, *m_TextFormat, rcEffect, solidBrush);
			}
			else  //if (m_Effect == EFFECT_BORDER)
			{
				rcEffect.Offset(0, 1);
				canvas.DrawTextW(string, (UINT)stringLen, *m_TextFormat, rcEffect, solidBrush);
				rcEffect.Offset(1, -1);
				canvas.DrawTextW(string, (UINT)stringLen, *m_TextFormat, rcEffect, solidBrush);
				rcEffect.Offset(-1, -1);
				canvas.DrawTextW(string, (UINT)stringLen, *m_TextFormat, rcEffect, solidBrush);
				rcEffect.Offset(-1, 1);
				canvas.DrawTextW(string, (UINT)stringLen, *m_TextFormat, rcEffect, solidBrush);
			}
		}

		SolidBrush solidBrush(m_Color);
		canvas.DrawTextW(string, (UINT)stringLen, *m_TextFormat, rcDest, solidBrush);

		if (m_Angle != 0.0f)
		{
			canvas.ResetTransform();
		}
	}

	return true;
}

/*
** Overridden method. The string meters need not to be bound on anything
**
*/
void MeterString::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

/*
** Static helper to log all installed font families.
**
*/
void MeterString::EnumerateInstalledFontFamilies()
{
	INT fontCount;
	InstalledFontCollection fontCollection;

	if (Ok == fontCollection.GetLastStatus())
	{
		fontCount = fontCollection.GetFamilyCount();
		if (fontCount > 0)
		{
			INT fontFound;

			FontFamily* fontFamilies = new FontFamily[fontCount];

			if (Ok == fontCollection.GetFamilies(fontCount, fontFamilies, &fontFound))
			{
				std::wstring fonts;
				for (INT i = 0; i < fontCount; ++i)
				{
					WCHAR familyName[LF_FACESIZE];
					if (Ok == fontFamilies[i].GetFamilyName(familyName))
					{
						fonts += familyName;
					}
					else
					{
						fonts += L"***";
					}
					fonts += L", ";
				}
				LogWarning(fonts.c_str());
			}
			else
			{
				LogError(L"Font enumeration: GetFamilies failed");
			}

			delete [] fontFamilies;
		}
		else
		{
			LogWarning(L"No installed fonts");
		}
	}
	else
	{
		LogError(L"Font enumeration: InstalledFontCollection failed");
	}
}

void MeterString::InitializeStatic()
{
	if (GetRainmeter().GetDebug())
	{
		LogDebug(L"------------------------------");
		LogDebug(L"* Font families:");
		EnumerateInstalledFontFamilies();
		LogDebug(L"------------------------------");
	}
}

void MeterString::FinalizeStatic()
{
}
