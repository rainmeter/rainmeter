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

std::wstring StringToUpper(std::wstring str);
std::wstring StringToLower(std::wstring str);
std::wstring StringToProper(std::wstring str);

/*
** CMeterString
**
** The constructor
**
*/
CMeterString::CMeterString(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Color = RGB(255, 255, 255);
	m_EffectColor = RGB(0, 0, 0);
	m_Effect = EFFECT_NONE;
	m_AutoScale = true;
	m_Align = ALIGN_LEFT;
	m_Font = NULL;
	m_FontFamily = NULL;
	m_Style = NORMAL;
	m_FontSize = 10;
	m_Scale = 1.0;
	m_NoDecimals = true;
	m_Percentual = true;
	m_ClipString = false;
	m_NumOfDecimals = -1;
	m_DimensionsDefined = false;
	m_Angle = 0.0;
	m_textCase = TEXTCASE_NONE;
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
	std::map<std::wstring, Gdiplus::FontFamily*>::const_iterator iter = c_FontFamilies.find(m_FontFace);
	if (iter != c_FontFamilies.end())
	{
		m_FontFamily = (*iter).second;
	}
	else
	{
		m_FontFamily = new FontFamily(m_FontFace.c_str());
		Status status = m_FontFamily->GetLastStatus();

		// It couldn't find the font family
		// Therefore we look in the privatefontcollection of this meters MeterWindow
		if(Ok != status)
		{
			delete m_FontFamily;
			m_FontFamily = new FontFamily(m_FontFace.c_str(), m_MeterWindow->GetPrivateFontCollection());
			status = m_FontFamily->GetLastStatus();

			// It couldn't find the font family: Log it.
			if(Ok != status)
			{	
				std::wstring error = L"Error: Couldn't load font family: ";
				error += m_FontFace;
				DebugLog(error.c_str());
				
				delete m_FontFamily;
				m_FontFamily = NULL;
			}
			
		}

		if(m_FontFamily)
		{
			c_FontFamilies[m_FontFace] = m_FontFamily;
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
	HDC dc = GetDC(GetDesktopWindow());
	int dpi = GetDeviceCaps(dc, LOGPIXELSX);
	ReleaseDC(GetDesktopWindow(), dc);

	REAL size = (REAL)m_FontSize * (96.0f / (REAL)dpi);

	std::wstring properties = FontPropertiesToString(m_FontFamily, size, style);
	std::map<std::wstring, Gdiplus::Font*>::const_iterator iter2 = c_Fonts.find(properties);
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
			c_Fonts[properties] = m_Font;
		}
		else
		{
			delete m_Font;
			m_Font = NULL;

			if (m_FontSize != 0)
			{
				throw CError(std::wstring(L"Unable to create font: ") + m_FontFace, __LINE__, __FILE__);
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
void CMeterString::ReadConfig(const WCHAR* section)
{
	WCHAR tmpName[64];

	// Store the current font values so we know if the font needs to be updated
	std::wstring oldFontFace = m_FontFace;
	int oldFontSize = m_FontSize;
	TEXTSTYLE oldStyle = m_Style;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_MeasureNames.clear();

	// Check for extra measures
	int i = 2;
	bool loop = true;
	do 
	{
		swprintf(tmpName, L"MeasureName%i", i);
		std::wstring measure = parser.ReadString(section, tmpName, L"");
		if (!measure.empty())
		{
			m_MeasureNames.push_back(measure);
		}
		else
		{
			loop = false;
		}
		++i;
	} while(loop);

	m_Color = parser.ReadColor(section, L"FontColor", Color::Black);
	m_EffectColor = parser.ReadColor(section, L"FontEffectColor", Color::Black);

	m_Prefix = parser.ReadString(section, L"Prefix", L"");
	m_Postfix = parser.ReadString(section, L"Postfix", L"");
	m_Text = parser.ReadString(section, L"Text", L"");

	m_Percentual = 0!=parser.ReadInt(section, L"Percentual", 0);
	m_AutoScale = 0!=parser.ReadInt(section, L"AutoScale", 0);
	m_ClipString = 0!=parser.ReadInt(section, L"ClipString", 0);

	m_FontSize = (int)parser.ReadFormula(section, L"FontSize", 10);

	if (m_FontSize < 0)
	{
		m_FontSize = 10;
	}

	m_NumOfDecimals = parser.ReadInt(section, L"NumOfDecimals", -1);

	m_Angle = (Gdiplus::REAL)parser.ReadFloat(section, L"Angle", 0.0);

	std::wstring scale;
	scale = parser.ReadString(section, L"Scale", L"1");

	if (scale.find(L'.') == std::wstring::npos)
	{
		m_NoDecimals = true;
	}
	else
	{
		m_NoDecimals = false;
	}
	m_Scale = wcstod(scale.c_str(), NULL);

	m_FontFace = parser.ReadString(section, L"FontFace", L"Arial");

	std::wstring align;
	align = parser.ReadString(section, L"StringAlign", L"LEFT");

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
		throw CError(std::wstring(L"StringAlign=") + align + L" is not valid in meter [" + m_Name + L"].", __LINE__, __FILE__);
	}

	std::wstring stringCase;
	stringCase = parser.ReadString(section, L"StringCase", L"NONE");
	
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
		throw CError(std::wstring(L"StringCase=") + stringCase + L" is not valid in meter [" + m_Name + L"].", __LINE__, __FILE__);
	}

	std::wstring style;
	style = parser.ReadString(section, L"StringStyle", L"NORMAL");

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
		throw CError(std::wstring(L"StringStyle=") + style + L" is not valid in meter [" + m_Name + L"].", __LINE__, __FILE__);
	}

	std::wstring effect;
	effect = parser.ReadString(section, L"StringEffect", L"NONE");

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
		throw CError(std::wstring(L"StringEffect=") + effect + L" is not valid in meter [" + m_Name + L"].", __LINE__, __FILE__);
	}

	if (-1 != (int)parser.ReadFormula(section, L"W", -1) && -1 != (int)parser.ReadFormula(section, L"H", -1))
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

		int decimals = (m_NumOfDecimals != -1) ? m_NumOfDecimals : (m_NoDecimals && (m_Percentual || !m_AutoScale)) ? 0 : 1;

		if (m_Measure) stringValues.push_back(m_Measure->GetStringValue(m_AutoScale, m_Scale, decimals, m_Percentual));

		// Get the values for the other measures
		for (size_t i = 0; i < m_Measures.size(); ++i)
		{
			stringValues.push_back(m_Measures[i]->GetStringValue(m_AutoScale, m_Scale, decimals, m_Percentual));
		}

		// Create the text
		m_String = m_Prefix;
		if (m_Text.empty())
		{
			if (stringValues.size() > 0)
			{
				m_String += stringValues[0]; 
			}
		}
		else
		{
			WCHAR buffer[64];
			// Create the actual text (i.e. replace %1, %2, .. with the measure texts)
			std::wstring tmpText = m_Text;

			for (size_t i = 0; i < stringValues.size(); ++i)
			{
				wsprintf(buffer, L"%%%i", i + 1);

				size_t start = 0;
				size_t pos = std::wstring::npos;

				do 
				{
					pos = tmpText.find(buffer, start);
					if (pos != std::wstring::npos)
					{
						tmpText.replace(tmpText.begin() + pos, tmpText.begin() + pos + wcslen(buffer), stringValues[i]);
						start = pos + stringValues[i].length();
					}
				} while(pos != std::wstring::npos);
			}

			m_String += tmpText;
		}
		m_String += m_Postfix;

		switch(m_textCase)
		{
		case TEXTCASE_UPPER:
			m_String = StringToUpper(m_String);
			break;
		case TEXTCASE_LOWER:
			m_String = StringToLower(m_String);
			break;
		case TEXTCASE_PROPER:
			m_String = StringToProper(m_String);
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

	REAL x = (REAL)GetX();
	REAL y = (REAL)GetY();

	if (rect)
	{
		PointF pos(x, y);
		graphics.MeasureString(m_String.c_str(), -1, m_Font, pos, rect);
	}
	else
	{
		RectF rc((REAL)x, (REAL)y, (REAL)m_W, (REAL)m_H);

		if (m_ClipString)
		{
			stringFormat.SetTrimming(StringTrimmingEllipsisCharacter);
		}
		else
		{
			stringFormat.SetTrimming(StringTrimmingNone);
			stringFormat.SetFormatFlags(StringFormatFlagsNoClip | StringFormatFlagsNoWrap);
		}

		REAL angle = m_Angle * 180.0f / 3.14159265f;		// Convert to degrees
		graphics.TranslateTransform((Gdiplus::REAL)CMeter::GetX(), y);
		graphics.RotateTransform(angle);
		graphics.TranslateTransform(-(Gdiplus::REAL)CMeter::GetX(), -y);

		if (m_Effect == EFFECT_SHADOW)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(rc);
			rcEffect.Offset(1, 1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
		}
		else if (m_Effect == EFFECT_BORDER)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(rc);
			rcEffect.Offset(0, 1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(1, -1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(-1, -1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(-1, 1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
		}
		
		SolidBrush solidBrush(m_Color);
		graphics.DrawString(m_String.c_str(), -1, m_Font, rc, &stringFormat, &solidBrush);

		graphics.ResetTransform();
	}

	return true;
}

/*
** BindMeasure
**
** Overridden method. The string meters need not to be bound on anything
**
*/
void CMeterString::BindMeasure(std::list<CMeasure*>& measures)
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
			throw CError(std::wstring(L"The meter [") + m_Name + L"] cannot be bound with [" + (*j) + L"]!", __LINE__, __FILE__);
		}
	}
	CMeter::SetAllMeasures(m_Measures);
}

/*
** FreeFontCache
**
** Static function which frees the font cache.
**
*/
void CMeterString::FreeFontCache()
{
	std::map<std::wstring, Gdiplus::FontFamily*>::iterator iter = c_FontFamilies.begin();
	for ( ; iter != c_FontFamilies.end(); ++iter)
	{
		delete (*iter).second;
	}
	c_FontFamilies.clear();

	std::map<std::wstring, Gdiplus::Font*>::iterator iter2 = c_Fonts.begin();
	for ( ; iter2 != c_Fonts.end(); ++iter2)
	{
		delete (*iter2).second;
	}
	c_Fonts.clear();
}

/*
** FontPropertiesToString
**
** Static helper to convert font properties to a string so it can be used as a key for the cache map.
**
*/
std::wstring CMeterString::FontPropertiesToString(FontFamily* fontFamily, REAL size, FontStyle style)
{
	std::wstringstream stream;
	stream << size << L"-" << (int)style;

	if (fontFamily)
	{
		WCHAR familyName[LF_FACESIZE];
		if (Ok == fontFamily->GetFamilyName(familyName))
		{
			return std::wstring(familyName) + L"-" + stream.str();
		}
	}
	return stream.str();
}

/*
** FontPropertiesToString
**
** Static helper to convert font properties to a string so it can be used as a key for the cache map.
**
*/
std::wstring StringToUpper(std::wstring str)
{
	//change each element of the string to upper case
	for(unsigned int i = 0; i < str.length(); ++i)
	{
		str[i] = toupper( str[i] );
	}

	return str; //return the converted string
}

std::wstring StringToLower(std::wstring str)
{
	//change each element of the string to lower case
	for(unsigned int i = 0; i < str.length(); ++i)
	{
		str[i] = tolower(str[i]);
	}

	return str;//return the converted string
}

std::wstring StringToProper(std::wstring str)
{
	//change each element of the string to lower case
	for(unsigned int i = 0; i < str.length(); ++i)
	{
		if(i == 0)
		{
			str[i] = toupper( str[i] );
		}
		else
		{
			if(str[i-1] == ' ')
			{
				str[i] = toupper(str[i]);
			}
			else
			{
				str[i] = tolower(str[i]);
			}
		}
	}

	return str;//return the converted string
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
				LSLog(LOG_DEBUG, L"Rainmeter", fonts.c_str());
			}
			else
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"Failed to enumerate installed font families: GetFamilies() failed.");
			}

			delete [] fontFamilies;
		}
		else
		{
			LSLog(LOG_DEBUG, L"Rainmeter", L"There are no installed font families!");
		}
	}
	else
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Failed to enumerate installed font families: InstalledFontCollection() failed.");
	}
}

// EOF
