/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterString.h"
#include "Rainmeter.h"
#include "Measure.h"
#include "../Common/Gfx/Canvas.h"

#define PI	(3.14159265f)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0f / PI))

MeterString::MeterString(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Color(D2D1::ColorF(D2D1::ColorF::White)),
	m_EffectColor(D2D1::ColorF(D2D1::ColorF::Black)),
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
	m_TextFormat(skin->GetCanvas().CreateTextFormat()),
	m_NumOfDecimals(-1),
	m_Angle(),
	m_FontWeight(-1)
{
}

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
		(m_Style & BOLD) != 0,
		(m_Style & ITALIC) != 0,
		m_Skin->GetFontCollection());
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

	m_Color = parser.ReadColor(section, L"FontColor", D2D1::ColorF(D2D1::ColorF::Black));
	m_EffectColor = parser.ReadColor(section, L"FontEffectColor", D2D1::ColorF(D2D1::ColorF::Black));

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
		LogErrorF(this, L"ClipString=%i is not valid", clipping);
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

	m_Angle = (FLOAT)parser.ReadFloat(section, L"Angle", 0.0);

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

	int weight = parser.ReadInt(section, L"FontWeight", -1);
	if (parser.GetLastValueDefined())
	{
		if (weight > 0 && weight < 1000)
		{
			m_FontWeight = weight;
		}
		else
		{
			LogErrorF(this, L"Invalid FontWeight: %i", weight);
		}
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

	m_TextFormat->ReadInlineOptions(parser, section);

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
			StringUtil::ToUpperCase(m_String);
			break;
		case TEXTCASE_LOWER:
			StringUtil::ToLowerCase(m_String);
			break;
		case TEXTCASE_PROPER:
			StringUtil::ToProperCase(m_String);
			break;
		}

		for (size_t i = 0; i < m_String.length(); ++i)
		{
			if (m_String[i] == L'\u00A0' ||  // No-Break Space
				m_String[i] == L'\u205F')    // Medium Mathematical Space
			{
				// Ugly hack to make D2D render trailing spaces followed by a non-breaking space
				// correctly. By default, D2D ignores all trailing whitespace. Both GDI+ and D2D,
				// however, acknowledge the presense of the zero-width space (and give it a width
				// of 0px), so we append the zero-width space after each non-breaking space.
				++i;
				m_String.insert(i, 1, L'\u200B');
			}
			else if (m_String[i] == L'\r')
			{
				// GDI+ seems to ignore carriage returns, so strip it entirely to make it behave
				// similarly with D2D as well.
				m_String.erase(i, 1);
				--i;
			}
		}

		m_TextFormat->SetFontWeight(m_FontWeight);
		m_TextFormat->FindInlineRanges(m_String);

		if (!m_WDefined || !m_HDefined)
		{
			// Calculate the text size
			D2D1_RECT_F rect;
			if (DrawString(m_Skin->GetCanvas(), &rect))
			{
				if (!m_WDefined)
					m_W = (int)(rect.right - rect.left) + GetWidthPadding();
				if (!m_HDefined)
					m_H = (int)(rect.bottom - rect.top) + GetHeightPadding();
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
bool MeterString::DrawString(Gfx::Canvas& canvas, D2D1_RECT_F* rect)
{
	if (!m_TextFormat->IsInitialized()) return false;

	canvas.SetTextAntiAliasing(m_AntiAlias);

	m_TextFormat->SetTrimming(
		m_ClipType == CLIP_ON ||
		(m_ClipType == CLIP_AUTO && (m_NeedsClipping || (m_WDefined && m_HDefined))));

	D2D1_RECT_F meterRect = GetMeterRectPadding();

	if (rect)
	{
		rect->left = meterRect.left;
		rect->top = meterRect.top;

		D2D1_SIZE_F size = D2D1::SizeF(rect->right - rect->left, rect->bottom - rect->top);
		if (canvas.MeasureTextW(m_String, *m_TextFormat, size))
		{
			rect->right = rect->left + size.width;
			rect->bottom = rect->top + size.height;

			if (m_ClipType == CLIP_AUTO)
			{
				// Set initial clipping
				m_NeedsClipping = false;

				FLOAT w = 0.0f;
				FLOAT h = 0.0f;
				bool updateSize = true;

				if (m_WDefined)
				{
					w = meterRect.right - meterRect.left;
					h = rect->bottom - rect->top;
					m_NeedsClipping = true;
				}
				else if (m_HDefined)
				{
					if (m_ClipStringW == -1)
					{
						// Text does not fit in defined height, clip it
						if (rect->bottom - rect->top > meterRect.bottom - meterRect.top)
						{
							m_NeedsClipping = true;
						}

						rect->bottom = meterRect.bottom;
						updateSize = false;

					}
					else
					{
						if (rect->right - rect->left > (FLOAT)m_ClipStringW)
						{
							w = (FLOAT)m_ClipStringW;
							m_NeedsClipping = true;
						}
						else
						{
							w = rect->right - rect->left;
						}

						h = meterRect.bottom - meterRect.top;
					}
				}
				else
				{
					if (m_ClipStringW == -1)
					{
						// Clip text if already larger than ClipStringH
						if (m_ClipStringH != -1 && rect->bottom - rect->top > (FLOAT)m_ClipStringH)
						{
							m_NeedsClipping = true;
							rect->bottom = rect->top + (FLOAT)m_ClipStringH;
						}

						updateSize = false;
					}
					else
					{
						if (rect->right - rect->left > (FLOAT)m_ClipStringW)
						{
							w = (FLOAT)m_ClipStringW;
							m_NeedsClipping = true;
						}
						else
						{
							w = rect->right - rect->left;
						}

						h = rect->bottom - rect->top;
					}
				}

				if (updateSize)
				{
					UINT32 lines = 0U;
					D2D1_SIZE_F size = D2D1::SizeF(w, h);
					if (canvas.MeasureTextLinesW(m_String, *m_TextFormat, size, lines) && lines != 0U)
					{
						rect->right = rect->left + w;
						rect->bottom = rect->top + size.height;

						if (m_HDefined || (m_ClipStringH != -1 && rect->bottom - rect->top > (FLOAT)m_ClipStringH))
						{
							rect->bottom = rect->top + (m_HDefined ? (FLOAT)meterRect.bottom - meterRect.top : (FLOAT)m_ClipStringH);
						}
					}
				}
			}
		}
	}
	else
	{
		D2D1_RECT_F rcDest = meterRect;

		if (m_Angle != 0.0f)
		{
			// Get current transform
			D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();
			canvas.GetTransform(&matrix);

			// GDI+ combatibiity.
			FLOAT cx = rcDest.left;
			switch (m_TextFormat->GetHorizontalAlignment())
			{
			case Gfx::HorizontalAlignment::Center: cx = (rcDest.left + rcDest.right) / 2.0f; break;
			case Gfx::HorizontalAlignment::Right:  cx = rcDest.right; break;
			}

			canvas.SetTransform(
				D2D1::Matrix3x2F::Rotation(CONVERT_TO_DEGREES(m_Angle), D2D1::Point2F(cx, rcDest.top)) *
				matrix);
		}

		if (m_Effect != EFFECT_NONE)
		{
			const D2D1_COLOR_F solidBrush = m_EffectColor;
			D2D1_RECT_F rcEffect = rcDest;

			auto offsetEffect = [&](FLOAT x, FLOAT y)
			{
				rcEffect.left += x;
				rcEffect.right += x;
				rcEffect.top += y;
				rcEffect.bottom += y;
			};

			if (m_Effect == EFFECT_SHADOW)
			{
				offsetEffect(1.0f, 1.0f);
				canvas.DrawTextW(m_String, *m_TextFormat, rcEffect, solidBrush);
			}
			else  //if (m_Effect == EFFECT_BORDER)
			{
				offsetEffect(0.0f, 1.0f);
				canvas.DrawTextW(m_String, *m_TextFormat, rcEffect, solidBrush);
				offsetEffect(1.0f, -1.0f);
				canvas.DrawTextW(m_String, *m_TextFormat, rcEffect, solidBrush);
				offsetEffect(-1.0f, -1.0f);
				canvas.DrawTextW(m_String, *m_TextFormat, rcEffect, solidBrush);
				offsetEffect(-1.0f, 1.0f);
				canvas.DrawTextW(m_String, *m_TextFormat, rcEffect, solidBrush);
			}
		}

		canvas.DrawTextW(m_String, *m_TextFormat, rcDest, m_Color, true);

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
** TODO: use Direct2d to enumrate the installed font families.
** See: https://msdn.microsoft.com/en-us/library/windows/desktop/dd756583(v=vs.85).aspx
*/
void MeterString::EnumerateInstalledFontFamilies()
{
	INT fontCount;
	Gdiplus::InstalledFontCollection fontCollection;

	if (Gdiplus::Ok == fontCollection.GetLastStatus())
	{
		fontCount = fontCollection.GetFamilyCount();
		if (fontCount > 0)
		{
			INT fontFound;

			Gdiplus::FontFamily* fontFamilies = new Gdiplus::FontFamily[fontCount];

			if (Gdiplus::Ok == fontCollection.GetFamilies(fontCount, fontFamilies, &fontFound))
			{
				std::wstring fonts;
				for (INT i = 0; i < fontCount; ++i)
				{
					WCHAR familyName[LF_FACESIZE];
					if (Gdiplus::Ok == fontFamilies[i].GetFamilyName(familyName))
					{
						if (*familyName)
						{
							fonts += familyName;
						}
					}
					else
					{
						fonts += L"***";
					}

					if (*familyName && i != (fontCount - 1))
					{
						fonts += L", ";
					}
				}
				LogDebug(fonts.c_str());
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
