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
#include "Error.h"
#include "Meter.h"
#include "MeterBitmap.h"
#include "MeterBar.h"
#include "MeterHistogram.h"
#include "MeterString.h"
#include "MeterImage.h"
#include "MeterLine.h"
#include "MeterRoundLine.h"
#include "MeterRotator.h"
#include "MeterButton.h"
#include "Measure.h"
#include "Rainmeter.h"

using namespace Gdiplus;

/*
** CMeter
**
** The constructor
**
*/
CMeter::CMeter(CMeterWindow* meterWindow)
{
	m_Measure = NULL;

	m_X = 0;
	m_Y = 0;
	m_W = 0;
	m_H = 0;
	m_RelativeMeter = NULL;
	m_Hidden = false;
	m_SolidBevel = BEVELTYPE_NONE;
	m_MouseOver = false;
	m_UpdateDivider = 1;
	m_UpdateCounter = 0;
	m_RelativeX = POSITION_ABSOLUTE;
	m_RelativeY = POSITION_ABSOLUTE;
	m_SolidAngle = 0.0f;
	m_AntiAlias = false;
	m_DynamicVariables = false;
	m_Initialized = false;
	m_HasMouseAction = false;
	m_MouseActionCursor = true;

	m_ToolTipHandle = NULL;

	m_MeterWindow = meterWindow;
}

/*
** ~CMeter
**
** The destructor
**
*/
CMeter::~CMeter()
{
}

/*
** Initialize
**
** Initializes the meter. The base implementation just stores the pointer.
** Usually this method is overwritten by the inherited classes, which load
** bitmaps and such things during initialization.
**
*/
void CMeter::Initialize()
{
	m_Initialized = true;
}

/*
** GetX
**
** Returns the X-position of the meter.
**
*/
int CMeter::GetX(bool abs)
{
	if (m_RelativeX != POSITION_ABSOLUTE && m_MeterWindow)
	{
		if (m_RelativeMeter == NULL)
		{
			std::list<CMeter*>& meters = m_MeterWindow->GetMeters();
			std::list<CMeter*>::const_iterator iter = meters.begin();

			// Find this meter
			for ( ; iter != meters.end(); ++iter)
			{
				if (*iter == this && iter != meters.begin())
				{
					--iter;
					m_RelativeMeter = (*iter);
					if (m_RelativeX == POSITION_RELATIVE_TL)
					{
						return m_RelativeMeter->GetX(true) + m_X;
					}
					else
					{
						return m_RelativeMeter->GetX(true) + m_RelativeMeter->GetW() + m_X;
					}
				}
			}
		}
		else
		{
			if (m_RelativeX == POSITION_RELATIVE_TL)
			{
				return m_RelativeMeter->GetX(true) + m_X;
			}
			else
			{
				return m_RelativeMeter->GetX(true) + m_RelativeMeter->GetW() + m_X;
			}
		}
	}
	return m_X; 
}

/*
** GetY
**
** Returns the Y-position of the meter.
**
*/
int CMeter::GetY(bool abs)
{
	if (m_RelativeY != POSITION_ABSOLUTE && m_MeterWindow)
	{
		if (m_RelativeMeter == NULL)
		{
			std::list<CMeter*>& meters = m_MeterWindow->GetMeters();
			std::list<CMeter*>::const_iterator iter = meters.begin();

			// Find this meter
			for ( ; iter != meters.end(); ++iter)
			{
				if (*iter == this && iter != meters.begin())
				{
					--iter;
					m_RelativeMeter = (*iter);
					if (m_RelativeY == POSITION_RELATIVE_TL)
					{
						return m_RelativeMeter->GetY() + m_Y;
					}
					else
					{
						return m_RelativeMeter->GetY() + m_RelativeMeter->GetH() + m_Y;
					}
				}
			}
		}
		else
		{
			if (m_RelativeY == POSITION_RELATIVE_TL)
			{
				return m_RelativeMeter->GetY() + m_Y;
			}
			else
			{
				return m_RelativeMeter->GetY() + m_RelativeMeter->GetH() + m_Y;
			}
		}
	}
	return m_Y; 
}

/*
** GetMeterRect
**
** Returns a RECT containing the dimensions of the meter within the MeterWindow
**
*/
RECT CMeter::GetMeterRect()
{
	RECT meterRect;

	meterRect.left = GetX();
	meterRect.top = GetY();
	meterRect.right = GetX() + m_W;
	meterRect.bottom = GetY() + m_H;

	return meterRect;
}

/*
** HitTest
**
** Checks if the given point is inside the meter.
**
*/
bool CMeter::HitTest(int x, int y)
{
	if (x >= GetX() && x < GetX() + GetW() && y >= GetY() && y < GetY() + GetH())
	{
		return true;
	}
	return false;
}

/*
** ReadConfig
**
** Reads the meter-specific configs from the ini-file. The base implementation
** reads the common settings for all meters. The inherited classes must call 
** the base implementation if they overwrite this method.
**
*/
void CMeter::ReadConfig(const WCHAR* section)
{
	bool replaced;

	CConfigParser& parser = m_MeterWindow->GetParser();

	// The MeterStyle defines a template where the values are read if the meter doesn't have it itself
	const std::wstring& style = parser.ReadString(section, L"MeterStyle", L"");
	if (!style.empty())
	{
		parser.SetStyleTemplate(style);
	}

	replaced = false;
	std::wstring coord = parser.ReadString(section, L"X", L"0", true, &replaced);
	if (!m_Initialized || replaced)
	{
		if (!coord.empty())
		{
			size_t len = coord.size();
			if (coord[len - 1] == L'r')
			{
				m_RelativeX = POSITION_RELATIVE_TL;
				coord.erase(--len);
			}
			else if (coord[len - 1] == L'R')
			{
				m_RelativeX = POSITION_RELATIVE_BR;
				coord.erase(--len);
			}
			else
			{
				m_RelativeX = POSITION_ABSOLUTE;
			}

			double val;
			if (len >= 2 && coord[0] == L'(' && coord[len - 1] == L')' && -1 != parser.ReadFormula(coord, &val))
			{
				m_X = (int)val;
			}
			else
			{
				m_X = (int)parser.ParseDouble(coord, 0.0);
			}
		}
		else
		{
			m_X = 0;
			m_RelativeX = POSITION_ABSOLUTE;
		}
	}

	replaced = false;
	coord = parser.ReadString(section, L"Y", L"0", true, &replaced);
	if (!m_Initialized || replaced)
	{
		if (!coord.empty())
		{
			size_t len = coord.size();
			if (coord[len - 1] == L'r')
			{
				m_RelativeY = POSITION_RELATIVE_TL;
				coord.erase(--len);
			}
			else if (coord[len - 1] == L'R')
			{
				m_RelativeY = POSITION_RELATIVE_BR;
				coord.erase(--len);
			}
			else
			{
				m_RelativeY = POSITION_ABSOLUTE;
			}

			double val;
			if (len >= 2 && coord[0] == L'(' && coord[len - 1] == L')' && -1 != parser.ReadFormula(coord, &val))
			{
				m_Y = (int)val;
			}
			else
			{
				m_Y = (int)parser.ParseDouble(coord, 0.0);
			}
		}
		else
		{
			m_Y = 0;
			m_RelativeY = POSITION_ABSOLUTE;
		}
	}

	m_W = (int)parser.ReadFormula(section, L"W", 1.0);
	m_H = (int)parser.ReadFormula(section, L"H", 1.0);

	if (!m_Initialized)
	{
		m_Hidden = 0!=parser.ReadInt(section, L"Hidden", 0);
	}
	else
	{
		replaced = false;
		const std::wstring& result = parser.ReadString(section, L"Hidden", L"0", true, &replaced);
		if (replaced)
		{
			m_Hidden = 0!=(int)parser.ParseDouble(result, 0.0, true);
		}
	}

	m_SolidBevel = (BEVELTYPE)parser.ReadInt(section, L"BevelType", BEVELTYPE_NONE);

	m_SolidColor = parser.ReadColor(section, L"SolidColor", Color(0, 0, 0, 0));
	m_SolidColor2 = parser.ReadColor(section, L"SolidColor2", m_SolidColor);
	m_SolidAngle = (Gdiplus::REAL)parser.ReadFloat(section, L"GradientAngle", 0.0);

	m_RightMouseDownAction = parser.ReadString(section, L"RightMouseDownAction", L"", false);
	m_LeftMouseDownAction = parser.ReadString(section, L"LeftMouseDownAction", L"", false);
	m_MiddleMouseDownAction = parser.ReadString(section, L"MiddleMouseDownAction", L"", false);
	m_RightMouseUpAction = parser.ReadString(section, L"RightMouseUpAction", L"", false);
	m_LeftMouseUpAction = parser.ReadString(section, L"LeftMouseUpAction", L"", false);
	m_MiddleMouseUpAction = parser.ReadString(section, L"MiddleMouseUpAction", L"", false);
	m_RightMouseDoubleClickAction = parser.ReadString(section, L"RightMouseDoubleClickAction", L"", false);
	m_LeftMouseDoubleClickAction = parser.ReadString(section, L"LeftMouseDoubleClickAction", L"", false);
	m_MiddleMouseDoubleClickAction = parser.ReadString(section, L"MiddleMouseDoubleClickAction", L"", false);
	m_MouseOverAction = parser.ReadString(section, L"MouseOverAction", L"", false);
	m_MouseLeaveAction = parser.ReadString(section, L"MouseLeaveAction", L"", false);

	m_MouseActionCursor = 0!=parser.ReadInt(section, L"MouseActionCursor", m_MouseActionCursor);

	m_HasMouseAction =
		( !m_LeftMouseUpAction.empty() || !m_LeftMouseDownAction.empty() || !m_LeftMouseDoubleClickAction.empty()
		|| !m_MiddleMouseUpAction.empty() || !m_MiddleMouseDownAction.empty() || !m_MiddleMouseDoubleClickAction.empty()
		|| !m_RightMouseUpAction.empty() || !m_RightMouseDownAction.empty() || !m_RightMouseDoubleClickAction.empty() );

	m_ToolTipText = parser.ReadString(section, L"ToolTipText", L"", true);
	m_ToolTipTitle = parser.ReadString(section, L"ToolTipTitle", L"", true);
	m_ToolTipIcon = parser.ReadString(section, L"ToolTipIcon", L"", true);
	m_ToolTipWidth = (int)parser.ReadFormula(section, L"ToolTipWidth", 1000);
	m_ToolTipType = 0!=parser.ReadInt(section, L"ToolTipType", 0);

	m_MeasureName = parser.ReadString(section, L"MeasureName", L"");

	m_UpdateDivider = parser.ReadInt(section, L"UpdateDivider", 1);
	m_UpdateCounter = m_UpdateDivider;
	m_AntiAlias = 0!=parser.ReadInt(section, L"AntiAlias", 0);
	m_DynamicVariables = 0!=parser.ReadInt(section, L"DynamicVariables", 0);

	std::vector<Gdiplus::REAL> matrix = parser.ReadFloats(section, L"TransformationMatrix");
	if (matrix.size() == 6)
	{
		m_Transformation.SetElements(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
	}
	else if (!matrix.empty())
	{
		DebugLog(L"The transformation matrix has incorrect number of values:", parser.ReadString(section, L"TransformationMatrix", L"").c_str());
	}

	std::wstring group = parser.ReadString(section, L"Group", L"");
	InitializeGroup(group);

/* Are these necessary?
	if (m_W == 0 || m_H == 0)
	{
        throw CError(std::wstring(L"The meter ") + section + L" has zero dimensions.", __LINE__, __FILE__);
	}
*/
}

/*
** BindMeasure
**
** Binds this meter to the given measure. The same measure can be bound to
** several meters but one meter and only be bound to one measure.
**
*/
void CMeter::BindMeasure(std::list<CMeasure*>& measures)
{
	// The meter is not bound to anything
	if (m_MeasureName.empty())
	{
		throw CError(std::wstring(L"The meter [") + m_Name + L"] is not bound to anything!", __LINE__, __FILE__);
	}

	// Go through the list and check it there is a measure for us
	std::list<CMeasure*>::const_iterator i = measures.begin();
	for( ; i != measures.end(); ++i)
	{
		if(_wcsicmp((*i)->GetName(), m_MeasureName.c_str()) == 0)
		{
			m_Measure = (*i);
			return;
		}
	}

	// Error :)
	throw CError(std::wstring(L"The meter [") + m_Name + L"] cannot be bound with [" + m_MeasureName + L"]!", __LINE__, __FILE__);
}

/*
** Create
**
** Creates the given meter. This is the factory method for the meters.
** If new meters are implemented this method needs to be updated.
**
*/
CMeter* CMeter::Create(const WCHAR* meter, CMeterWindow* meterWindow)
{
	if(_wcsicmp(L"HISTOGRAM", meter) == 0)
	{
		return new CMeterHistogram(meterWindow);
	} 
	else if(_wcsicmp(L"STRING", meter) == 0)
	{
		return new CMeterString(meterWindow);
	} 
	else if(_wcsicmp(L"BAR", meter) == 0)
	{
		return new CMeterBar(meterWindow);
	} 
	else if(_wcsicmp(L"BITMAP", meter) == 0)
	{
		return new CMeterBitmap(meterWindow);
	} 
	else if(_wcsicmp(L"IMAGE", meter) == 0)
	{
		return new CMeterImage(meterWindow);
	} 
	else if(_wcsicmp(L"LINE", meter) == 0)
	{
		return new CMeterLine(meterWindow);
	} 
	else if(_wcsicmp(L"ROUNDLINE", meter) == 0)
	{
		return new CMeterRoundLine(meterWindow);
	} 
	else if(_wcsicmp(L"ROTATOR", meter) == 0)
	{
		return new CMeterRotator(meterWindow);
	} 
	else if(_wcsicmp(L"BUTTON", meter) == 0)
	{
		return new CMeterButton(meterWindow);
	} 

	// Error
	throw CError(std::wstring(L"Meter=") + meter + L" is not valid.", __LINE__, __FILE__);

	return NULL;
}

/*
** Update
**
** Updates the value(s) from the measures. Derived classes should
** only update if this returns true;
*/
bool CMeter::Update()
{
	// Only update the meter's value when the divider is equal to the counter
	++m_UpdateCounter;
	if (m_UpdateCounter < m_UpdateDivider) return false;
	m_UpdateCounter = 0;

	return true;
}

/*
** SetAllMeasures
**
** Creates a vector containing all the defined measures (for Histogram)
*/
void CMeter::SetAllMeasures(CMeasure* measure)
{
	m_AllMeasures.clear();
	m_AllMeasures.push_back(m_Measure);
	m_AllMeasures.push_back(measure);
}

/*
** SetAllMeasures
**
** Creates a vector containing all the defined measures (for Line/String)
*/
void CMeter::SetAllMeasures(std::vector<CMeasure*> measures)
{
	m_AllMeasures.clear();
	m_AllMeasures.push_back(m_Measure);

	std::vector<CMeasure*>::const_iterator i = measures.begin();
	for( ; i != measures.end(); ++i)
	{
		m_AllMeasures.push_back(*i);
	}
}

/*
** ReplaceMeasures
**
** Replaces %1, %2 etc with the corresponding measure value
*/
std::wstring CMeter::ReplaceMeasures(std::wstring source)
{
	std::vector<std::wstring> stringValues;

	if (!m_AllMeasures.empty()) 
	{	
		// Get the values for the measures
		for (size_t i = 0; i < m_AllMeasures.size(); ++i)
		{
			stringValues.push_back(m_AllMeasures[i]->GetStringValue(true, 1, 0, false));
		}
	}
	else if (m_Measure != NULL)
	{
		stringValues.push_back(m_Measure->GetStringValue(true, 1, 0, false));
	}
	else
	{
		return source;
	}

	WCHAR buffer[64];
	// Create the actual text (i.e. replace %1, %2, .. with the measure texts)

	for (size_t i = 0; i < stringValues.size(); ++i)
	{
		wsprintf(buffer, L"%%%i", i + 1);

		size_t start = 0;
		size_t pos = std::wstring::npos;

		do 
		{
			pos = source.find(buffer, start);
			if (pos != std::wstring::npos)
			{
				source.replace(source.begin() + pos, source.begin() + pos + wcslen(buffer), stringValues[i]);
				start = pos + stringValues[i].length();
			}
		} while(pos != std::wstring::npos);
	}
	return source;
}

/*
** CreateToolTip
**
** Does the initial construction of the ToolTip for the meter
*/
void CMeter::CreateToolTip(CMeterWindow* meterWindow)
{
	DWORD style = WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP;

	if (m_ToolTipType)
	{
		style |= TTS_BALLOON;
	}

	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS, 
		NULL,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		m_MeterWindow->GetWindow(),
		NULL,
		m_MeterWindow->GetMainObject()->GetInstance(),
		NULL);

	if (hwndTT)
	{
		SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		TOOLINFO ti = {sizeof(TOOLINFO)};
		ti.uFlags = TTF_SUBCLASS;
		ti.hwnd = m_MeterWindow->GetWindow();
		ti.hinst = m_MeterWindow->GetMainObject()->GetInstance();

		ti.rect = GetMeterRect();

		SendMessage(hwndTT, TTM_ADDTOOL, NULL, (LPARAM) (LPTOOLINFO) &ti);

		m_ToolTipHandle = hwndTT;
		UpdateToolTip();
	}
}

/*
** UpdateToolTip
**
** Updates the ToolTip to match new values
*/
void CMeter::UpdateToolTip()
{
	HWND hwndTT = m_ToolTipHandle;

	TOOLINFO ti = {sizeof(TOOLINFO)};
	ti.hwnd = m_MeterWindow->GetWindow();

	SendMessage(hwndTT, TTM_GETTOOLINFO, NULL, (LPARAM) (LPTOOLINFO) &ti);

	std::wstring text = ReplaceMeasures(m_ToolTipText);
	ti.lpszText = (PTSTR) text.c_str();
	ti.rect = GetMeterRect();

	SendMessage(hwndTT, TTM_SETTOOLINFO, NULL, (LPARAM) (LPTOOLINFO) &ti); 
	SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, NULL, m_ToolTipWidth);

	if (!m_ToolTipTitle.empty())
	{
		HICON hIcon = NULL;
		bool destroy = false;

		if (!m_ToolTipIcon.empty())
		{
			if (!_wcsicmp(m_ToolTipIcon.c_str(), L"INFO"))
			{
				hIcon = (HICON) TTI_INFO;
			}
			else if (!_wcsicmp(m_ToolTipIcon.c_str(), L"WARNING"))
			{
				hIcon = (HICON) TTI_WARNING;
			}
			else if (!_wcsicmp(m_ToolTipIcon.c_str(), L"ERROR"))
			{
				hIcon = (HICON) TTI_ERROR;
			}
			else if (!_wcsicmp(m_ToolTipIcon.c_str(), L"QUESTION"))
			{
				hIcon = LoadIcon(NULL, IDI_QUESTION);
			}
			else if (!_wcsicmp(m_ToolTipIcon.c_str(), L"SHIELD"))
			{
				hIcon = LoadIcon(NULL, IDI_SHIELD);
			}
			else
			{
				hIcon = (HICON) LoadImage(NULL, m_ToolTipIcon.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
				destroy = true;
			}
		}

		text = ReplaceMeasures(m_ToolTipTitle);
		SendMessage(hwndTT, TTM_SETTITLE, (WPARAM) hIcon, (LPARAM) text.c_str());

		if (destroy)
		{
			DestroyIcon(hIcon);
		}
	}
	SendMessage(hwndTT, TTM_ACTIVATE, !IsHidden(), NULL);
}

/*
** Draw
**
** Draws the solid background & bevel if such are defined
*/
bool CMeter::Draw(Graphics& graphics)
{
	if (IsHidden()) return false;

	graphics.SetInterpolationMode(InterpolationModeDefault);
	graphics.SetCompositingQuality(CompositingQualityDefault);

	if (m_AntiAlias)
	{
		//graphics.SetInterpolationMode(InterpolationModeBicubic);  // Bicubic is not suitable for shrinking an image.
		graphics.SetSmoothingMode(SmoothingModeHighQuality);
		graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
	}
	else
	{
		graphics.SetSmoothingMode(SmoothingModeNone);
		graphics.SetPixelOffsetMode(PixelOffsetModeDefault);
	}

	if (m_SolidColor.GetA() != 0 || m_SolidColor2.GetA() != 0)
	{
		int x = GetX();
		int y = GetY();

		if (m_SolidColor.GetValue() == m_SolidColor2.GetValue())
		{
			SolidBrush solid(m_SolidColor);
			graphics.FillRectangle(&solid, x, y, m_W, m_H);
		}
		else
		{
			Rect r(x, y, m_W, m_H);
			LinearGradientBrush gradient(r, m_SolidColor, m_SolidColor2, m_SolidAngle, TRUE);
			graphics.FillRectangle(&gradient, r);
		}
	}

	if (m_SolidBevel != BEVELTYPE_NONE)
	{
		int x = GetX();
		int y = GetY();

		Pen light(Color(255, 255, 255, 255));
		Pen dark(Color(255, 0, 0, 0));

		if (m_SolidBevel == BEVELTYPE_DOWN)
		{
			light.SetColor(Color(255, 0, 0, 0));
			dark.SetColor(Color(255, 255, 255, 255));
		}

		// The bevel is drawn outside the meter
		Rect rect(x - 2, y - 2, m_W + 4, m_H + 4);	
		DrawBevel(graphics, rect, light, dark);
	}

	return true;
}

/*
** DrawBevel
**
** Draws a bevel inside the given area
*/
void CMeter::DrawBevel(Graphics& graphics, Rect& rect, Pen& light, Pen& dark)
{
	int l = rect.GetLeft();
	int r = rect.GetRight() - 1;
	int t = rect.GetTop();
	int b = rect.GetBottom() - 1;

	graphics.DrawLine(&light, l,     t,     l,     b);
	graphics.DrawLine(&light, l,     t,     r,     t);
	graphics.DrawLine(&light, l + 1, t + 1, l + 1, b - 1);
	graphics.DrawLine(&light, l + 1, t + 1, r - 1, l + 1);
	graphics.DrawLine(&dark,  l,     b,     r,     b);
	graphics.DrawLine(&dark,  r,     t,     r,     b);
	graphics.DrawLine(&dark,  l + 1, b - 1, r - 1, b - 1);
	graphics.DrawLine(&dark,  r - 1, t + 1, r - 1, b - 1);
}
