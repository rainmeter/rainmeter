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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "MeterString.h"
#include "Rainmeter.h"
#include "Measure.h"
#include "Error.h"

using namespace Gdiplus;

std::map<std::wstring, Gdiplus::FontFamily*> CMeterString::c_FontFamilies;
std::map<std::wstring, Gdiplus::Font*> CMeterString::c_Fonts;

void StringToUpper(std::wstring& str)
{
	//change each element of the string to upper case
	std::transform(str.begin(), str.end(), str.begin(), ::towupper);
}

void StringToLower(std::wstring& str)
{
	//change each element of the string to lower case
	std::transform(str.begin(), str.end(), str.begin(), ::towlower);
}

void StringToProper(std::wstring& str)
{
	//change each element of the string to lower case
	if (!str.empty())
	{
		str[0] = towupper(str[0]);

		for (size_t i = 1; i < str.length(); ++i)
		{
			if (str[i-1] == L' ')
			{
				str[i] = towupper(str[i]);
			}
			else
			{
				str[i] = towlower(str[i]);
			}
		}
	}
}

/*
** CMeterString
**
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
	m_textCase(TEXTCASE_NONE),
	m_FontSize(10),
	m_Scale(1.0),
	m_NoDecimals(true),
	m_Percentual(true),
	m_ClipString(false),
	m_Font(),
	m_FontFamily(),
	m_NumOfDecimals(-1),
	m_DimensionsDefined(false),
	m_Angle()
{
}

/*
** ~CMeterString
**
** The destructor
**
*/
CMeterString::~CMeterString()
{
}

/*
** GetX
**
** Returns the X-coordinate of the meter
**
*/
int CMeterString::GetX(bool abs)
{
	int x = CMeter::GetX();

	if (!abs) 
	{
		switch(m_Align)
		{
		case ALIGN_CENTER:
			x = x - (m_W / 2);
			break;

		case ALIGN_RIGHT:
			x -= m_W;
			break;

		case ALIGN_LEFT:
			// This is already correct
			break;
		}
	}

	return x;
}


/*
** Initialize
**
** Create the font that is used to draw the text.
**
*/
void CMeterString::Initialize()
{
	CMeter::Initialize();

	// Check if the font family is in the cache and use it
	std::wstring cacheKey;
	std::wstring systemFontFaceKey = FontFaceToString(m_FontFace, NULL);
	std::map<std::wstring, Gdiplus::FontFamily*>::const_iterator iter = c_FontFamilies.find(systemFontFaceKey);
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
					std::wstring error = L"Unable to load font family: " + m_FontFace;
					Log(LOG_ERROR, error.c_str());

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

	switch(m_Style)
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
	cacheKey += L"-";
	cacheKey += FontPropertiesToString(size, style);
	std::map<std::wstring, Gdiplus::Font*>::const_iterator iter2 = c_Fonts.find(cacheKey);
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
			m_Font = new Gdiplus::Font(FontFamily::GenericSansSerif(), size, style);
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
				std::wstring error = L"Unable to create font: " + m_FontFace;
				throw CError(error, __LINE__, __FILE__);
			}
		}
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterString::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Store the current font values so we know if the font needs to be updated
	std::wstring oldFontFace = m_FontFace;
	int oldFontSize = m_FontSize;
	TEXTSTYLE oldStyle = m_Style;

	// Read common configs
	CMeter::ReadConfig(parser, section);

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

	m_FontSize = (int)parser.ReadFormula(section, L"FontSize", 10);
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
	if (scale.find(L'.') == std::wstring::npos)
	{
		m_NoDecimals = true;
	}
	else
	{
		m_NoDecimals = false;
	}
	m_Scale = wcstod(scale.c_str(), NULL);

	const std::wstring& align = parser.ReadString(section, L"StringAlign", L"LEFT");
	if(_wcsicmp(align.c_str(), L"LEFT") == 0)
	{
		m_Align = ALIGN_LEFT;
	}
	else if(_wcsicmp(align.c_str(), L"RIGHT") == 0)
	{
		m_Align = ALIGN_RIGHT;
	}
	else if(_wcsicmp(align.c_str(), L"CENTER") == 0)
	{
		m_Align = ALIGN_CENTER;
	}
	else
	{
		std::wstring error = L"StringAlign=" + align;
		error += L" is not valid in meter [";
		error += m_Name;
		error += L"].";
		throw CError(error, __LINE__, __FILE__);
	}

	const std::wstring& stringCase = parser.ReadString(section, L"StringCase", L"NONE");
	if(_wcsicmp(stringCase.c_str(), L"NONE") == 0)
	{
		m_textCase = TEXTCASE_NONE;
	}
	else if(_wcsicmp(stringCase.c_str(), L"UPPER") == 0)
	{
		m_textCase = TEXTCASE_UPPER;
	}
	else if(_wcsicmp(stringCase.c_str(), L"LOWER") == 0)
	{
		m_textCase = TEXTCASE_LOWER;
	}
	else if(_wcsicmp(stringCase.c_str(), L"PROPER") == 0)
	{
		m_textCase = TEXTCASE_PROPER;
	}
	else
	{
		std::wstring error = L"StringCase=" + stringCase;
		error += L" is not valid in meter [";
		error += m_Name;
		error += L"].";
		throw CError(error, __LINE__, __FILE__);
	}

	const std::wstring& style = parser.ReadString(section, L"StringStyle", L"NORMAL");
	if(_wcsicmp(style.c_str(), L"NORMAL") == 0)
	{
		m_Style = NORMAL;
	}
	else if(_wcsicmp(style.c_str(), L"BOLD") == 0)
	{
		m_Style = BOLD;
	}
	else if(_wcsicmp(style.c_str(), L"ITALIC") == 0)
	{
		m_Style = ITALIC;
	}
	else if(_wcsicmp(style.c_str(), L"BOLDITALIC") == 0)
	{
		m_Style = BOLDITALIC;
	}
	else
	{
		std::wstring error = L"StringStyle=" + style;
		error += L" is not valid in meter [";
		error += m_Name;
		error += L"].";
		throw CError(error, __LINE__, __FILE__);
	}

	const std::wstring& effect = parser.ReadString(section, L"StringEffect", L"NONE");
	if(_wcsicmp(effect.c_str(), L"NONE") == 0)
	{
		m_Effect = EFFECT_NONE;
	}
	else if(_wcsicmp(effect.c_str(), L"SHADOW") == 0)
	{
		m_Effect = EFFECT_SHADOW;
	}
	else if(_wcsicmp(effect.c_str(), L"BORDER") == 0)
	{
		m_Effect = EFFECT_BORDER;
	}
	else
	{
		std::wstring error = L"StringEffect=" + effect;
		error += L" is not valid in meter [";
		error += m_Name;
		error += L"].";
		throw CError(error, __LINE__, __FILE__);
	}

	if (parser.IsValueDefined(section, L"W") && parser.IsValueDefined(section, L"H"))
	{
		m_DimensionsDefined = true;
	}

	if (m_Initialized &&
		(oldFontFace != m_FontFace ||
		oldFontSize != m_FontSize ||
		oldStyle != m_Style))
	{
		Initialize();	// Recreate the font
	}
}

/*
** Update
**
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
		for (size_t i = 0, isize = m_Measures.size(); i < isize; ++i)
		{
			stringValues.push_back(m_Measures[i]->GetStringValue(m_AutoScale, m_Scale, decimals, m_Percentual));
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

		switch(m_textCase)
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

		if (!m_DimensionsDefined)
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
** Draw
**
** Draws the meter on the double buffer
**
*/
bool CMeterString::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	return DrawString(graphics, NULL);
}

/*
** DrawString
**
** Draws the string or calculates it's size
**
*/
bool CMeterString::DrawString(Graphics& graphics, RectF* rect)
{
	if (m_Font == NULL) return false;

	StringFormat stringFormat;
	
	if (m_AntiAlias)
	{
		graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
	}
	else
	{
		graphics.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);
	}

	switch(m_Align)
	{
	case ALIGN_CENTER:
		stringFormat.SetAlignment(StringAlignmentCenter);
		break;

	case ALIGN_RIGHT:
		stringFormat.SetAlignment(StringAlignmentFar);
		break;

	case ALIGN_LEFT:
		stringFormat.SetAlignment(StringAlignmentNear);
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

	CharacterRange range(0, (int)m_String.length());
	stringFormat.SetMeasurableCharacterRanges(1, &range);

	REAL x = (REAL)GetX();
	REAL y = (REAL)GetY();

	if (rect)
	{
		PointF pos(x, y);
		graphics.MeasureString(m_String.c_str(), (int)m_String.length(), m_Font, pos, &stringFormat, rect);
	}
	else
	{
		m_Rect = RectF((REAL)x, (REAL)y, (REAL)m_W, (REAL)m_H);

		if (m_Angle != 0.0f)
		{
			REAL angle = m_Angle * 180.0f / 3.14159265f;		// Convert to degrees
			graphics.TranslateTransform((Gdiplus::REAL)CMeter::GetX(), y);
			graphics.RotateTransform(angle);
			graphics.TranslateTransform(-(Gdiplus::REAL)CMeter::GetX(), -y);
		}

		if (m_Effect == EFFECT_SHADOW)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(m_Rect);
			rcEffect.Offset(1, 1);
			graphics.DrawString(m_String.c_str(), (int)m_String.length(), m_Font, rcEffect, &stringFormat, &solidBrush);
		}
		else if (m_Effect == EFFECT_BORDER)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(m_Rect);
			rcEffect.Offset(0, 1);
			graphics.DrawString(m_String.c_str(), (int)m_String.length(), m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(1, -1);
			graphics.DrawString(m_String.c_str(), (int)m_String.length(), m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(-1, -1);
			graphics.DrawString(m_String.c_str(), (int)m_String.length(), m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(-1, 1);
			graphics.DrawString(m_String.c_str(), (int)m_String.length(), m_Font, rcEffect, &stringFormat, &solidBrush);
		}
		
		SolidBrush solidBrush(m_Color);
		graphics.DrawString(m_String.c_str(), (int)m_String.length(), m_Font, m_Rect, &stringFormat, &solidBrush);

		if (m_Angle != 0.0f)
		{
			graphics.ResetTransform();
		}
	}

	return true;
}

/*
** BindMeasure
**
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
		std::list<CMeasure*>::const_iterator i = measures.begin();
		for( ; i != measures.end(); ++i)
		{
			if(_wcsicmp((*i)->GetName(), (*j).c_str()) == 0)
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
			error += L"]!";
			throw CError(error, __LINE__, __FILE__);
		}
	}
	CMeter::SetAllMeasures(m_Measures);
}

/*
** FreeFontCache
**
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
		_snwprintf_s(buffer, _TRUNCATE, L"<%p>", collection);
		prefix = buffer;
		StringToLower(prefix);
	}

	std::map<std::wstring, Gdiplus::Font*>::iterator iter2 = c_Fonts.begin();
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

	std::map<std::wstring, Gdiplus::FontFamily*>::iterator iter = c_FontFamilies.begin();
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
** FontFaceToString
**
** Static helper to convert font name to a string so it can be used as a key for the cache map.
**
*/
std::wstring CMeterString::FontFaceToString(const std::wstring& fontFace, PrivateFontCollection* collection)
{
	WCHAR buffer[32];
	_snwprintf_s(buffer, _TRUNCATE, L"<%p>", collection);
	std::wstring strTmp = buffer + fontFace;
	StringToLower(strTmp);
	return strTmp;
}

/*
** FontPropertiesToString
**
** Static helper to convert font properties to a string so it can be used as a key for the cache map.
**
*/
std::wstring CMeterString::FontPropertiesToString(REAL size, FontStyle style)
{
	WCHAR buffer[64];
	_snwprintf_s(buffer, _TRUNCATE, L"%.1f-%i", size, (int)style);
	return buffer;
}

/*
** EnumerateInstalledFontFamilies
**
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
				Log(LOG_ERROR, L"Failed to enumerate installed font families: GetFamilies() failed.");
			}

			delete [] fontFamilies;
		}
		else
		{
			Log(LOG_WARNING, L"There are no installed font families!");
		}
	}
	else
	{
		Log(LOG_ERROR, L"Failed to enumerate installed font families: InstalledFontCollection() failed.");
	}
}