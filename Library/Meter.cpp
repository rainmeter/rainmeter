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

extern CRainmeter* Rainmeter;

/*
** The constructor
**
*/
CMeter::CMeter(CMeterWindow* meterWindow, const WCHAR* name) : m_MeterWindow(meterWindow), m_Name(name),
	m_X(),
	m_Y(),
	m_W(0),
	m_H(0),
	m_Hidden(false),
	m_WDefined(false),
	m_HDefined(false),
	m_RelativeMeter(),
	m_DynamicVariables(false),
	m_Transformation(),
	m_ToolTipWidth(),
	m_ToolTipType(false),
	m_ToolTipHidden(meterWindow->GetMeterToolTipHidden()),
	m_ToolTipHandle(),
	m_HasMouseAction(false),
	m_MouseOver(false),
	m_RelativeX(POSITION_ABSOLUTE),
	m_RelativeY(POSITION_ABSOLUTE),
	m_UpdateDivider(1),
	m_UpdateCounter(1),
	m_SolidBevel(BEVELTYPE_NONE),
	m_SolidAngle(),
	m_AntiAlias(false),
	m_Initialized(false)
{
}

/*
** The destructor
**
*/
CMeter::~CMeter()
{
	delete m_Transformation;

	if (m_ToolTipHandle != NULL)
	{
		DestroyWindow(m_ToolTipHandle);
	}
}

/*
** Initializes the meter. Usually this method is overwritten by the inherited
** classes, which load bitmaps and such things during initialization.
**
*/
void CMeter::Initialize()
{
	m_Initialized = true;
}

/*
** Returns the X-position of the meter.
**
*/
int CMeter::GetX(bool abs)
{
	if (m_RelativeX != POSITION_ABSOLUTE && m_RelativeMeter)
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
	return m_X;
}

/*
** Returns the Y-position of the meter.
**
*/
int CMeter::GetY(bool abs)
{
	if (m_RelativeY != POSITION_ABSOLUTE && m_RelativeMeter)
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
	return m_Y;
}

void CMeter::SetX(int x)
{
	m_X = x;
	m_RelativeX = POSITION_ABSOLUTE;

	// Change the option as well to avoid reset in ReadOptions().
	WCHAR buffer[32];
	_itow_s(x, buffer, 10);
	m_MeterWindow->GetParser().SetValue(m_Name, L"X", buffer);
}

void CMeter::SetY(int y)
{
	m_Y = y;
	m_RelativeY = POSITION_ABSOLUTE;

	// Change the option as well to avoid reset in ReadOptions().
	WCHAR buffer[32];
	_itow_s(y, buffer, 10);
	m_MeterWindow->GetParser().SetValue(m_Name, L"Y", buffer);
}

/*
** Returns a RECT containing the dimensions of the meter within the MeterWindow
**
*/
RECT CMeter::GetMeterRect()
{
	RECT meterRect;

	meterRect.left = GetX();
	meterRect.top = GetY();
	meterRect.right = meterRect.left + m_W;
	meterRect.bottom = meterRect.top + m_H;

	return meterRect;
}

/*
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
** Shows the meter and tooltip.
**
*/
void CMeter::Show()
{
	m_Hidden = false;

	// Change the option as well to avoid reset in ReadOptions().
	m_MeterWindow->GetParser().SetValue(m_Name, L"Hidden", L"0");

	if (m_ToolTipHandle != NULL)
	{
		if (!m_ToolTipHidden)
		{
			SendMessage(m_ToolTipHandle, TTM_ACTIVATE, TRUE, NULL);
		}
	}
}

/*
** Hides the meter and tooltip.
**
*/
void CMeter::Hide()
{
	m_Hidden = true;

	// Change the option as well to avoid reset in ReadOptions().
	m_MeterWindow->GetParser().SetValue(m_Name, L"Hidden", L"1");

	if (m_ToolTipHandle != NULL)
	{
		SendMessage(m_ToolTipHandle, TTM_ACTIVATE, FALSE, NULL);
	}
}

/*
** Read the common options specified in the ini file. The inherited classes must
** call this base implementation if they overwrite this method.
**
*/
void CMeter::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	// The MeterStyle defines a template where the values are read if the meter doesn't have it itself
	const std::wstring& style = parser.ReadString(section, L"MeterStyle", L"");
	if (!style.empty())
	{
		parser.SetStyleTemplate(style);
	}

	BindMeasures(parser, section);

	int oldX = m_X;
	std::wstring& x = (std::wstring&)parser.ReadString(section, L"X", L"0");
	if (!x.empty())
	{
		WCHAR lastChar = x[x.size() - 1];
		if (lastChar == L'r')
		{
			m_RelativeX = POSITION_RELATIVE_TL;
			x.pop_back();
		}
		else if (lastChar == L'R')
		{
			m_RelativeX = POSITION_RELATIVE_BR;
			x.pop_back();
		}
		else
		{
			m_RelativeX = POSITION_ABSOLUTE;
		}

		m_X = parser.ParseInt(x.c_str(), 0);
	}
	else
	{
		m_X = 0;
		m_RelativeX = POSITION_ABSOLUTE;
	}

	int oldY = m_Y;
	std::wstring& y = (std::wstring&)parser.ReadString(section, L"Y", L"0");
	if (!y.empty())
	{
		WCHAR lastChar = y[y.size() - 1];
		if (lastChar == L'r')
		{
			m_RelativeY = POSITION_RELATIVE_TL;
			y.pop_back();
		}
		else if (lastChar == L'R')
		{
			m_RelativeY = POSITION_RELATIVE_BR;
			y.pop_back();
		}
		else
		{
			m_RelativeY = POSITION_ABSOLUTE;
		}

		m_Y = parser.ParseInt(y.c_str(), 0);
	}
	else
	{
		m_Y = 0;
		m_RelativeY = POSITION_ABSOLUTE;
	}

	bool oldWDefined = m_WDefined;
	bool oldHDefined = m_HDefined;

	m_W = parser.ReadInt(section, L"W", m_W);
	m_WDefined = parser.GetLastValueDefined();
	if (!m_WDefined && oldWDefined)
	{
		m_W = 0;
		parser.SetValue(section, L"W", L"0");
	}
	
	m_H = parser.ReadInt(section, L"H", m_H);
	m_HDefined = parser.GetLastValueDefined();
	if (!m_HDefined && oldHDefined)
	{
		m_H = 0;
		parser.SetValue(section, L"H", L"0");
	}

	bool oldHidden = m_Hidden;
	m_Hidden = 0!=parser.ReadInt(section, L"Hidden", 0);

	if (oldX != m_X || oldY != m_Y || oldHidden != m_Hidden)
	{
		m_MeterWindow->SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
	}

	m_SolidBevel = (BEVELTYPE)parser.ReadInt(section, L"BevelType", BEVELTYPE_NONE);

	m_SolidColor = parser.ReadColor(section, L"SolidColor", Color::MakeARGB(0, 0, 0, 0));
	m_SolidColor2 = parser.ReadColor(section, L"SolidColor2", m_SolidColor.GetValue());
	m_SolidAngle = (Gdiplus::REAL)parser.ReadFloat(section, L"GradientAngle", 0.0);

	m_Mouse.ReadOptions(parser, section, m_MeterWindow);

	m_HasMouseAction =
		!(m_Mouse.GetLeftUpAction().empty() && m_Mouse.GetLeftDownAction().empty() &&
		m_Mouse.GetLeftDoubleClickAction().empty() && m_Mouse.GetMiddleUpAction().empty() &&
		m_Mouse.GetMiddleDownAction().empty() && m_Mouse.GetMiddleDoubleClickAction().empty() &&
		m_Mouse.GetRightUpAction().empty() && m_Mouse.GetRightDownAction().empty() &&
		m_Mouse.GetRightDoubleClickAction().empty());

	m_ToolTipText = parser.ReadString(section, L"ToolTipText", L"");
	m_ToolTipTitle = parser.ReadString(section, L"ToolTipTitle", L"");
	m_ToolTipIcon = parser.ReadString(section, L"ToolTipIcon", L"");
	m_ToolTipWidth = (int)parser.ReadFloat(section, L"ToolTipWidth", 1000);
	m_ToolTipType = 0!=parser.ReadInt(section, L"ToolTipType", 0);
	m_ToolTipHidden = 0!=parser.ReadInt(section, L"ToolTipHidden", m_MeterWindow->GetMeterToolTipHidden());

	int updateDivider = parser.ReadInt(section, L"UpdateDivider", 1);
	if (updateDivider != m_UpdateDivider)
	{
		m_UpdateCounter = m_UpdateDivider = updateDivider;
	}

	m_AntiAlias = 0!=parser.ReadInt(section, L"AntiAlias", 0);
	m_DynamicVariables = 0!=parser.ReadInt(section, L"DynamicVariables", 0);

	std::vector<Gdiplus::REAL> matrix = parser.ReadFloats(section, L"TransformationMatrix");
	if (matrix.size() == 6)
	{
		if (m_Transformation)
		{
			m_Transformation->SetElements(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
		}
		else
		{
			m_Transformation = new Matrix(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
		}
	}
	else if (!matrix.empty())
	{
		delete m_Transformation;
		m_Transformation = NULL;

		LogWithArgs(LOG_ERROR, L"Meter: Incorrect number of values in TransformationMatrix=%s", parser.ReadString(section, L"TransformationMatrix", L"").c_str());
	}

	const std::wstring& group = parser.ReadString(section, L"Group", L"");
	InitializeGroup(group);
}

/*
** Binds this meter to the given measure. The same measure can be bound to
** several meters but one meter and only be bound to one measure.
**
*/
void CMeter::BindMeasures(CConfigParser& parser, const WCHAR* section)
{
	BindPrimaryMeasure(parser, section, false);
}

/*
** Creates the given meter. This is the factory method for the meters.
** If new meters are implemented this method needs to be updated.
**
*/
CMeter* CMeter::Create(const WCHAR* meter, CMeterWindow* meterWindow, const WCHAR* name)
{
	if (_wcsicmp(L"STRING", meter) == 0)
	{
		return new CMeterString(meterWindow, name);
	}
	else if (_wcsicmp(L"IMAGE", meter) == 0)
	{
		return new CMeterImage(meterWindow, name);
	}
	else if (_wcsicmp(L"HISTOGRAM", meter) == 0)
	{
		return new CMeterHistogram(meterWindow, name);
	}
	else if (_wcsicmp(L"BAR", meter) == 0)
	{
		return new CMeterBar(meterWindow, name);
	}
	else if (_wcsicmp(L"BITMAP", meter) == 0)
	{
		return new CMeterBitmap(meterWindow, name);
	}
	else if (_wcsicmp(L"LINE", meter) == 0)
	{
		return new CMeterLine(meterWindow, name);
	}
	else if (_wcsicmp(L"ROUNDLINE", meter) == 0)
	{
		return new CMeterRoundLine(meterWindow, name);
	}
	else if (_wcsicmp(L"ROTATOR", meter) == 0)
	{
		return new CMeterRotator(meterWindow, name);
	}
	else if (_wcsicmp(L"BUTTON", meter) == 0)
	{
		return new CMeterButton(meterWindow, name);
	}

	LogWithArgs(LOG_ERROR, L"Meter=%s is not valid in [%s]", meter, name);

	return NULL;
}

/*
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
** Reads and binds the primary MeasureName. This must always be called in overridden
** BindMeasures() implementations.
**
*/
bool CMeter::BindPrimaryMeasure(CConfigParser& parser, const WCHAR* section, bool optional)
{
	m_Measures.clear();

	const std::wstring& measureName = parser.ReadString(section, L"MeasureName", L"");

	CMeasure* measure = parser.GetMeasure(measureName);
	if (measure)
	{
		m_Measures.push_back(measure);
		return true;
	}
	else if (!optional)
	{
		LogWithArgs(LOG_ERROR, L"MeasureName=%s is not valid in [%s]", measureName.c_str(), section);
	}

	return false;
}

/*
** Reads and binds secondary measures (MeasureName2 - MeasureNameN).
**
*/
void CMeter::BindSecondaryMeasures(CConfigParser& parser, const WCHAR* section)
{
	if (!m_Measures.empty())
	{
		WCHAR tmpName[64];

		int i = 2;
		do
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"MeasureName%i", i);
			const std::wstring& measureName = parser.ReadString(section, tmpName, L"");
			CMeasure* measure = parser.GetMeasure(measureName);
			if (measure)
			{
				m_Measures.push_back(measure);
			}
			else
			{
				if (!measureName.empty())
				{
					LogWithArgs(LOG_ERROR, L"MeasureName%i=%s is not valid in [%s]", i, measureName.c_str(), section);
				}

				break;
			}
			++i;
		}
		while (true);
	}
}

/*
** Replaces %1, %2, ... with the corresponding measure value.
**
*/
bool CMeter::ReplaceMeasures(std::wstring& str, AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	bool replaced = false;

	if (str.find(L'%') != std::wstring::npos)
	{
		WCHAR buffer[64];

		for (size_t i = m_Measures.size(); i > 0; --i)
		{
			size_t len = _snwprintf_s(buffer, _TRUNCATE, L"%%%i", (int)i);
			size_t start = 0, pos;

			const WCHAR* measureValue = m_Measures[i - 1]->GetStringValue(autoScale, scale, decimals, percentual);
			int measureValueLen = wcslen(measureValue);

			do
			{
				pos = str.find(buffer, start, len);
				if (pos != std::wstring::npos)
				{
					str.replace(pos, len, measureValue, measureValueLen);
					start = pos + measureValueLen;
					replaced = true;
				}
			}
			while (pos != std::wstring::npos);
		}
	}

	return replaced;
}

/*
** Does the initial construction of the ToolTip for the meter
*/
void CMeter::CreateToolTip(CMeterWindow* meterWindow)
{
	HWND hMeterWindow = m_MeterWindow->GetWindow();
	HINSTANCE hInstance = Rainmeter->GetInstance();
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
		hMeterWindow,
		NULL,
		hInstance,
		NULL);

	if (hwndTT)
	{
		SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		TOOLINFO ti = {sizeof(TOOLINFO), TTF_SUBCLASS, hMeterWindow, 0, GetMeterRect(), hInstance};

		SendMessage(hwndTT, TTM_ADDTOOL, NULL, (LPARAM) (LPTOOLINFO) &ti);

		m_ToolTipHandle = hwndTT;
		UpdateToolTip();
	}
}

/*
** Updates the ToolTip to match new values
*/
void CMeter::UpdateToolTip()
{
	HWND hwndTT = m_ToolTipHandle;

	TOOLINFO ti = {sizeof(TOOLINFO)};
	ti.hwnd = m_MeterWindow->GetWindow();

	SendMessage(hwndTT, TTM_GETTOOLINFO, NULL, (LPARAM)&ti);

	std::wstring text = m_ToolTipText;
	ReplaceMeasures(text);
	ti.lpszText = (LPTSTR)text.c_str();
	ti.rect = GetMeterRect();

	SendMessage(hwndTT, TTM_SETTOOLINFO, NULL, (LPARAM)&ti);
	SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, NULL, m_ToolTipWidth);

	if (!m_ToolTipTitle.empty())
	{
		HICON hIcon = NULL;
		bool destroy = false;

		if (!m_ToolTipIcon.empty())
		{
			const WCHAR* tipIcon = m_ToolTipIcon.c_str();
			if (_wcsicmp(tipIcon, L"INFO") == 0)
			{
				hIcon = (HICON)TTI_INFO;
			}
			else if (_wcsicmp(tipIcon, L"WARNING") == 0)
			{
				hIcon = (HICON)TTI_WARNING;
			}
			else if (_wcsicmp(tipIcon, L"ERROR") == 0)
			{
				hIcon = (HICON)TTI_ERROR;
			}
			else if (_wcsicmp(tipIcon, L"QUESTION") == 0)
			{
				hIcon = LoadIcon(NULL, IDI_QUESTION);
			}
			else if (_wcsicmp(tipIcon, L"SHIELD") == 0)
			{
				hIcon = LoadIcon(NULL, IDI_SHIELD);
			}
			else
			{
				hIcon = (HICON)LoadImage(NULL, tipIcon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
				destroy = true;
			}
		}

		text = m_ToolTipTitle;
		ReplaceMeasures(text);
		SendMessage(hwndTT, TTM_SETTITLE, (WPARAM) hIcon, (LPARAM)text.c_str());

		if (destroy)
		{
			DestroyIcon(hIcon);
		}
	}

	if (m_ToolTipHidden)
	{
		SendMessage(hwndTT, TTM_ACTIVATE, FALSE, NULL);
	}
	else
	{
		SendMessage(hwndTT, TTM_ACTIVATE, !IsHidden(), NULL);
	}
}

/*
** Draws the solid background & bevel if such are defined
*/
bool CMeter::Draw(Graphics& graphics)
{
	if (IsHidden()) return false;

	graphics.SetInterpolationMode(InterpolationModeDefault);
	graphics.SetCompositingQuality(CompositingQualityDefault);

	if (m_AntiAlias)
	{
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

		Rect r(x, y, m_W, m_H);

		if (m_SolidColor.GetValue() == m_SolidColor2.GetValue())
		{
			SolidBrush solid(m_SolidColor);
			graphics.FillRectangle(&solid, r);
		}
		else
		{
			if (!m_AntiAlias)
			{
				// Fix the tiling issue in some GradientAngle values
				graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
			}

			LinearGradientBrush gradient(r, m_SolidColor, m_SolidColor2, m_SolidAngle, TRUE);
			graphics.FillRectangle(&gradient, r);

			if (!m_AntiAlias)
			{
				graphics.SetPixelOffsetMode(PixelOffsetModeDefault);
			}
		}
	}

	if (m_SolidBevel != BEVELTYPE_NONE)
	{
		int x = GetX();
		int y = GetY();

		Color lightColor(255, 255, 255, 255);
		Color darkColor(255, 0, 0, 0);

		if (m_SolidBevel == BEVELTYPE_DOWN)
		{
			lightColor.SetValue(Color::MakeARGB(255, 0, 0, 0));
			darkColor.SetValue(Color::MakeARGB(255, 255, 255, 255));
		}

		Pen light(lightColor);
		Pen dark(darkColor);

		// The bevel is drawn outside the meter
		Rect rect(x - 2, y - 2, m_W + 4, m_H + 4);
		DrawBevel(graphics, rect, light, dark);
	}

	return true;
}

/*
** Draws a bevel inside the given area
*/
void CMeter::DrawBevel(Graphics& graphics, const Rect& rect, const Pen& light, const Pen& dark)
{
	int l = rect.GetLeft();
	int r = rect.GetRight() - 1;
	int t = rect.GetTop();
	int b = rect.GetBottom() - 1;

	graphics.DrawLine(&light, l,     t,     l,     b);
	graphics.DrawLine(&light, l,     t,     r,     t);
	graphics.DrawLine(&light, l + 1, t + 1, l + 1, b - 1);
	graphics.DrawLine(&light, l + 1, t + 1, r - 1, t + 1);
	graphics.DrawLine(&dark,  l,     b,     r,     b);
	graphics.DrawLine(&dark,  r,     t,     r,     b);
	graphics.DrawLine(&dark,  l + 1, b - 1, r - 1, b - 1);
	graphics.DrawLine(&dark,  r - 1, t + 1, r - 1, b - 1);
}
