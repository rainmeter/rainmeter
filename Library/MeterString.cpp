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

using namespace Gdiplus;

std::unordered_map<std::wstring, Gdiplus::FontFamily*> CMeterString::c_FontFamilies;
std::unordered_map<std::wstring, Gdiplus::Font*> CMeterString::c_Fonts;

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
			LCMapString(LOCALE_USER_DEFAULT, (str[i - 1] == L' ') ? LCMAP_UPPERCASE : LCMAP_LOWERCASE, srcAndDest, 1, srcAndDest, 1);
		}
	}
}

/*
** The constructor
**
*/
CMeterString::CMeterString(CMeterWindow* meterWindow, const WCHAR* name) : CMeter(meterWindow, name),
	m_Color(Color::White),
	m_EffectColor(Color::Black),
	m_AutoScale(AUTOSCALE_OFF),
	m_Align(ALIGN_LEFT),
	m_Style(NORMAL),
	m_Effect(EFFECT_NONE),
	m_Case(TEXTCASE_NONE),
	m_FontSize(10),
	m_Scale(1.0),
	m_NoDecimals(true),
	m_Percentual(true),
	m_ClipString(false),
	m_Font(),
	m_FontFamily(),
	m_NumOfDecimals(-1),
	m_Angle()
{
}

/*
** The destructor
**
*/
CMeterString::~CMeterString()
{
}

/*
** Returns the X-coordinate of the meter
**
*/
int CMeterString::GetX(bool abs)
{
	int x = CMeter::GetX();

	if (!abs)
	{
		switch (m_Align)
		{
		case ALIGN_CENTER:			// Same as ALIGN_CENTERTOP
		case ALIGN_CENTERBOTTOM:
		case ALIGN_CENTERCENTER:
			x -= m_W / 2;
			break;

		case ALIGN_RIGHT:			// Same as ALIGN_RIGHTTOP
		case ALIGN_RIGHTBOTTOM:
		case ALIGN_RIGHTCENTER:
			x -= m_W;
			break;

		case ALIGN_LEFT:			// Same as ALIGN_LEFTTOP
		case ALIGN_LEFTBOTTOM:
		case ALIGN_LEFTCENTER:
			// This is already correct
			break;
		}
	}

	return x;
}

/*
** Returns the Y-coordinate of the meter
**
*/
int CMeterString::GetY(bool abs)
{
	int y = CMeter::GetY();

	if (!abs)
	{
		switch (m_Align)
		{
		case ALIGN_LEFTCENTER:
		case ALIGN_RIGHTCENTER:
		case ALIGN_CENTERCENTER:
			y -= m_H / 2;
			break;

		case ALIGN_LEFTBOTTOM:
		case ALIGN_RIGHTBOTTOM:
		case ALIGN_CENTERBOTTOM:
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
void CMeterString::Initialize()
{
	CMeter::Initialize();

	// Check if the font family is in the cache and use it
	std::wstring cacheKey;
	std::wstring systemFontFaceKey = FontFaceToString(m_FontFace, NULL);
	std::unordered_map<std::wstring, Gdiplus::FontFamily*>::const_iterator iter = c_FontFamilies.find(systemFontFaceKey);
	if (iter != c_FontFamilies.end())
	{
		m_FontFamily = (*iter).second;
		cacheKey = systemFontFaceKey;
	}
	else
	{
		m_FontFamily = NULL;

		PrivateFontCollection* collection = m_MeterWindow->GetPrivateFontCollection();
		std::wstring privateFontFaceKey;

		if (collection)
		{
			// Check if the private font family is in the cache and use it
			privateFontFaceKey = FontFaceToString(m_FontFace, collection);
			iter = c_FontFamilies.find(privateFontFaceKey);
			if (iter != c_FontFamilies.end())
			{
				m_FontFamily = (*iter).second;
				cacheKey = privateFontFaceKey;
			}
		}

		if (m_FontFamily == NULL)  // Not found in the cache
		{
			m_FontFamily = new FontFamily(m_FontFace.c_str());
			Status status = m_FontFamily->GetLastStatus();

			if (Ok == status)
			{
				cacheKey = systemFontFaceKey;
			}
			else
			{
				delete m_FontFamily;

				// It couldn't find the font family
				// Therefore we look in the privatefontcollection of this meters MeterWindow
				if (collection)
				{
					m_FontFamily = new FontFamily(m_FontFace.c_str(), collection);
					status = m_FontFamily->GetLastStatus();

					if (Ok == status)
					{
						cacheKey = privateFontFaceKey;
					}
				}
				else
				{
					m_FontFamily = NULL;
				}

				// It couldn't find the font family: Log it.
				if (Ok != status)
				{
					LogWithArgs(LOG_ERROR, L"String: Unable to load font: %s", m_FontFace.c_str());

					delete m_FontFamily;
					m_FontFamily = NULL;

					cacheKey = L"<>";  // set dummy key
				}
			}

			if (m_FontFamily)
			{
				// Cache
				//LogWithArgs(LOG_DEBUG, L"FontFamilyCache-Add: %s", cacheKey.c_str());
				c_FontFamilies[cacheKey] = m_FontFamily;
			}
		}
	}

	FontStyle style = FontStyleRegular;

	switch (m_Style)
	{
	case ITALIC:
		style = FontStyleItalic;
		break;

	case BOLD:
		style = FontStyleBold;
		break;

	case BOLDITALIC:
		style = FontStyleBoldItalic;
		break;
	}

	// Adjust the font size with screen DPI
	HDC dc = GetDC(0);
	int dpi = GetDeviceCaps(dc, LOGPIXELSX);
	ReleaseDC(0, dc);

	REAL size = (REAL)m_FontSize * (96.0f / (REAL)dpi);

	// Check if the font is in the cache and use it
	cacheKey += L'-';
	cacheKey += FontPropertiesToString(size, style);
	std::unordered_map<std::wstring, Gdiplus::Font*>::const_iterator iter2 = c_Fonts.find(cacheKey);
	if (iter2 != c_Fonts.end())
	{
		m_Font = (*iter2).second;
	}
	else
	{
		if (m_FontFamily)
		{
			m_Font = new Gdiplus::Font(m_FontFamily, size, style);
		}
		else
		{
			m_Font = new Gdiplus::Font(FontFamily::GenericSansSerif (), size, style);
		}

		Status status = m_Font->GetLastStatus();
		if (Ok == status)
		{
			// Cache
			//LogWithArgs(LOG_DEBUG, L"FontCache-Add: %s", cacheKey.c_str());
			c_Fonts[cacheKey] = m_Font;
		}
		else
		{
			delete m_Font;
			m_Font = NULL;

			if (m_FontSize != 0)
			{
				std::wstring error = L"String: Unable to create font: " + m_FontFace;
				throw CError(error);
			}
		}
	}
}

/*
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterString::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	// Store the current font values so we know if the font needs to be updated
	std::wstring oldFontFace = m_FontFace;
	int oldFontSize = m_FontSize;
	TEXTSTYLE oldStyle = m_Style;

	// Read common configs
	CMeter::ReadOptions(parser, section);

	// Check for extra measures
	if (!m_Initialized && !m_MeasureName.empty())
	{
		ReadMeasureNames(parser, section, m_MeasureNames);
	}

	m_Color = parser.ReadColor(section, L"FontColor", Color::Black);
	m_EffectColor = parser.ReadColor(section, L"FontEffectColor", Color::Black);

	m_Prefix = parser.ReadString(section, L"Prefix", L"");
	m_Postfix = parser.ReadString(section, L"Postfix", L"");
	m_Text = parser.ReadString(section, L"Text", L"");

	m_Percentual = 0!=parser.ReadInt(section, L"Percentual", 0);
	m_ClipString = 0!=parser.ReadInt(section, L"ClipString", 0);

	m_FontFace = parser.ReadString(section, L"FontFace", L"Arial");
	if (m_FontFace.empty())
	{
		m_FontFace = L"Arial";
	}

	m_FontSize = (int)parser.ReadFloat(section, L"FontSize", 10);
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

	const WCHAR* align = parser.ReadString(section, L"StringAlign", L"LEFT").c_str();
	if (_wcsicmp(align, L"LEFT") == 0 || _wcsicmp(align, L"LEFTTOP") == 0)
	{
		m_Align = ALIGN_LEFT;
	}
	else if (_wcsicmp(align, L"RIGHT") == 0 || _wcsicmp(align, L"RIGHTTOP") == 0)
	{
		m_Align = ALIGN_RIGHT;
	}
	else if (_wcsicmp(align, L"CENTER") == 0 || _wcsicmp(align, L"CENTERTOP") == 0)
	{
		m_Align = ALIGN_CENTER;
	}
	else if (_wcsicmp(align, L"LEFTBOTTOM") == 0)
	{
		m_Align = ALIGN_LEFTBOTTOM;
	}
	else if (_wcsicmp(align, L"RIGHTBOTTOM") == 0)
	{
		m_Align = ALIGN_RIGHTBOTTOM;
	}
	else if (_wcsicmp(align, L"CENTERBOTTOM") == 0)
	{
		m_Align = ALIGN_CENTERBOTTOM;
	}
	else if (_wcsicmp(align, L"LEFTCENTER") == 0)
	{
		m_Align = ALIGN_LEFTCENTER;
	}
	else if (_wcsicmp(align, L"RIGHTCENTER") == 0)
	{
		m_Align = ALIGN_RIGHTCENTER;
	}
	else if (_wcsicmp(align, L"CENTERCENTER") == 0)
	{
		m_Align = ALIGN_CENTERCENTER;
	}
	else
	{
		LogWithArgs(LOG_ERROR, L"StringAlign=%s is not valid in [%s]", align, m_Name.c_str());
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
		LogWithArgs(LOG_ERROR, L"StringCase=%s is not valid in [%s]", stringCase, m_Name.c_str());
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
		LogWithArgs(LOG_ERROR, L"StringStyle=%s is not valid in [%s]", style, m_Name.c_str());
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
		LogWithArgs(LOG_ERROR, L"StringEffect=%s is not valid in [%s]", effect, m_Name.c_str());
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
bool CMeterString::Update()
{
	if (CMeter::Update())
	{
		std::vector<std::wstring> stringValues;

		int decimals = (m_NumOfDecimals != -1) ? m_NumOfDecimals : (m_NoDecimals && (m_Percentual || m_AutoScale == AUTOSCALE_OFF)) ? 0 : 1;

		if (m_Measure) stringValues.push_back(m_Measure->GetStringValue(m_AutoScale, m_Scale, decimals, m_Percentual));

		// Get the values for the other measures
		std::vector<CMeasure*>::const_iterator iter = m_Measures.begin();
		for ( ; iter != m_Measures.end(); ++iter)
		{
			stringValues.push_back((*iter)->GetStringValue(m_AutoScale, m_Scale, decimals, m_Percentual));
		}

		// Create the text
		m_String = m_Prefix;
		if (m_Text.empty())
		{
			if (!stringValues.empty())
			{
				m_String += stringValues[0];
			}
		}
		else if (!stringValues.empty())
		{
			std::wstring tmpText = m_Text;
			ReplaceMeasures(stringValues, tmpText);
			m_String += tmpText;
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

		if (!m_WDefined || !m_HDefined)
		{
			// Calculate the text size
			RectF rect;
			Graphics graphics(m_MeterWindow->GetDoubleBuffer());
			if (DrawString(graphics, &rect))
			{
				m_W = (int)rect.Width;
				m_H = (int)rect.Height;
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
bool CMeterString::Draw(Graphics& graphics)
{
	if (!CMeter::Draw(graphics)) return false;

	return DrawString(graphics, NULL);
}

/*
** Draws the string or calculates it's size
**
*/
bool CMeterString::DrawString(Graphics& graphics, RectF* rect)
{
	if (m_Font == NULL) return false;

	LPCWSTR string = m_String.c_str();
	int stringLen = (int)m_String.length();

	StringFormat stringFormat;

	if (m_AntiAlias)
	{
		graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
	}
	else
	{
		graphics.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);
	}

	switch (m_Align)
	{
	case ALIGN_CENTERCENTER:
		stringFormat.SetAlignment(StringAlignmentCenter);
		stringFormat.SetLineAlignment(StringAlignmentCenter);
		break;

	case ALIGN_RIGHTCENTER:
		stringFormat.SetAlignment(StringAlignmentFar);
		stringFormat.SetLineAlignment(StringAlignmentCenter);
		break;

	case ALIGN_LEFTCENTER:
		stringFormat.SetAlignment(StringAlignmentNear);
		stringFormat.SetLineAlignment(StringAlignmentCenter);
		break;

	case ALIGN_CENTERBOTTOM:
		stringFormat.SetAlignment(StringAlignmentCenter);
		stringFormat.SetLineAlignment(StringAlignmentFar);
		break;

	case ALIGN_RIGHTBOTTOM:
		stringFormat.SetAlignment(StringAlignmentFar);
		stringFormat.SetLineAlignment(StringAlignmentFar);
		break;

	case ALIGN_LEFTBOTTOM:
		stringFormat.SetAlignment(StringAlignmentNear);
		stringFormat.SetLineAlignment(StringAlignmentFar);
		break;

	case ALIGN_CENTER:	// Same as CenterTop
		stringFormat.SetAlignment(StringAlignmentCenter);
		stringFormat.SetLineAlignment(StringAlignmentNear);
		break;

	case ALIGN_RIGHT:	// Same as RightTop
		stringFormat.SetAlignment(StringAlignmentFar);
		stringFormat.SetLineAlignment(StringAlignmentNear);
		break;

	case ALIGN_LEFT:	// Same as LeftTop
		stringFormat.SetAlignment(StringAlignmentNear);
		stringFormat.SetLineAlignment(StringAlignmentNear);
		break;
	}

	if (m_ClipString)
	{
		stringFormat.SetTrimming(StringTrimmingEllipsisCharacter);
	}
	else
	{
		stringFormat.SetTrimming(StringTrimmingNone);
		stringFormat.SetFormatFlags(StringFormatFlagsNoClip | StringFormatFlagsNoWrap);
	}

	CharacterRange range(0, stringLen);
	stringFormat.SetMeasurableCharacterRanges(1, &range);

	REAL x = (REAL)GetX();
	REAL y = (REAL)GetY();

	if (rect)
	{
		PointF pos(x, y);
		graphics.MeasureString(string, stringLen, m_Font, pos, &stringFormat, rect);
	}
	else
	{
		RectF rcDest(x, y, (REAL)m_W, (REAL)m_H);
		m_Rect = rcDest;

		if (m_Angle != 0.0f)
		{
			graphics.TranslateTransform((Gdiplus::REAL)CMeter::GetX(), y);
			graphics.RotateTransform(CONVERT_TO_DEGREES(m_Angle));
			graphics.TranslateTransform(-(Gdiplus::REAL)CMeter::GetX(), -y);
		}

		if (m_Effect != EFFECT_NONE)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(rcDest);

			if (m_Effect == EFFECT_SHADOW)
			{
				rcEffect.Offset(1, 1);
				graphics.DrawString(string, stringLen, m_Font, rcEffect, &stringFormat, &solidBrush);
			}
			else  //if (m_Effect == EFFECT_BORDER)
			{
				rcEffect.Offset(0, 1);
				graphics.DrawString(string, stringLen, m_Font, rcEffect, &stringFormat, &solidBrush);
				rcEffect.Offset(1, -1);
				graphics.DrawString(string, stringLen, m_Font, rcEffect, &stringFormat, &solidBrush);
				rcEffect.Offset(-1, -1);
				graphics.DrawString(string, stringLen, m_Font, rcEffect, &stringFormat, &solidBrush);
				rcEffect.Offset(-1, 1);
				graphics.DrawString(string, stringLen, m_Font, rcEffect, &stringFormat, &solidBrush);
			}
		}

		SolidBrush solidBrush(m_Color);
		graphics.DrawString(string, stringLen, m_Font, rcDest, &stringFormat, &solidBrush);

		if (m_Angle != 0.0f)
		{
			graphics.ResetTransform();
		}
	}

	return true;
}

/*
** Overridden method. The string meters need not to be bound on anything
**
*/
void CMeterString::BindMeasure(const std::list<CMeasure*>& measures)
{
	if (m_MeasureName.empty()) return;	// Allow NULL measure binding

	CMeter::BindMeasure(measures);

	std::vector<std::wstring>::const_iterator j = m_MeasureNames.begin();
	for (; j != m_MeasureNames.end(); ++j)
	{
		// Go through the list and check it there is a secondary measures for us
		const WCHAR* name = (*j).c_str();
		std::list<CMeasure*>::const_iterator i = measures.begin();
		for ( ; i != measures.end(); ++i)
		{
			if (_wcsicmp((*i)->GetName(), name) == 0)
			{
				m_Measures.push_back(*i);
				break;
			}
		}

		if (i == measures.end())
		{
			std::wstring error = L"The meter [" + m_Name;
			error += L"] cannot be bound with [";
			error += (*j);
			error += L"]";
			throw CError(error);
		}
	}
	CMeter::SetAllMeasures(m_Measures);
}

/*
** Static function which frees the font cache.
** If collection is not NULL, frees the private font cache.
**
*/
void CMeterString::FreeFontCache(PrivateFontCollection* collection)
{
	std::wstring prefix;

	if (collection)
	{
		WCHAR buffer[32];
		size_t len = _snwprintf_s(buffer, _TRUNCATE, L"<%p>", collection);

		prefix.assign(buffer, len);
		_wcsupr(&prefix[0]);
	}

	std::unordered_map<std::wstring, Gdiplus::Font*>::iterator iter2 = c_Fonts.begin();
	while (iter2 != c_Fonts.end())
	{
		if (collection == NULL || wcsncmp((*iter2).first.c_str(), prefix.c_str(), prefix.length()) == 0)
		{
			//LogWithArgs(LOG_DEBUG, L"FontCache-Remove: %s", (*iter2).first.c_str());
			delete (*iter2).second;

			if (collection)
			{
				c_Fonts.erase(iter2++);
				continue;
			}
		}
		++iter2;
	}
	if (collection == NULL) c_Fonts.clear();

	std::unordered_map<std::wstring, Gdiplus::FontFamily*>::iterator iter = c_FontFamilies.begin();
	while (iter != c_FontFamilies.end())
	{
		if (collection == NULL || wcsncmp((*iter).first.c_str(), prefix.c_str(), prefix.length()) == 0)
		{
			//LogWithArgs(LOG_DEBUG, L"FontFamilyCache-Remove: %s", (*iter).first.c_str());
			delete (*iter).second;

			if (collection)
			{
				c_FontFamilies.erase(iter++);
				continue;
			}
		}
		++iter;
	}
	if (collection == NULL) c_FontFamilies.clear();
}

/*
** Static helper to convert font name to a string so it can be used as a key for the cache map.
**
*/
std::wstring CMeterString::FontFaceToString(const std::wstring& fontFace, PrivateFontCollection* collection)
{
	WCHAR buffer[32];
	size_t len = _snwprintf_s(buffer, _TRUNCATE, L"<%p>", collection);

	std::wstring strTmp;
	strTmp.reserve(len + fontFace.size());
	strTmp.assign(buffer, len);
	strTmp += fontFace;
	_wcsupr(&strTmp[0]);
	return strTmp;
}

/*
** Static helper to convert font properties to a string so it can be used as a key for the cache map.
**
*/
std::wstring CMeterString::FontPropertiesToString(REAL size, FontStyle style)
{
	WCHAR buffer[64];
	size_t len = _snwprintf_s(buffer, _TRUNCATE, L"%.1f-%i", size, (int)style);

	return std::wstring(buffer, len);
}

/*
** Static helper to log all installed font families.
**
*/
void CMeterString::EnumerateInstalledFontFamilies()
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
				Log(LOG_NOTICE, fonts.c_str());
			}
			else
			{
				Log(LOG_ERROR, L"Font enumeration: GetFamilies failed");
			}

			delete [] fontFamilies;
		}
		else
		{
			Log(LOG_WARNING, L"No installed fonts");
		}
	}
	else
	{
		Log(LOG_ERROR, L"Font enumeration: InstalledFontCollection failed");
	}
}