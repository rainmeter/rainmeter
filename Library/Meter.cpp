/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
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
#include "MeterShape.h"
#include "Measure.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

Meter::Meter(Skin* skin, const WCHAR* name) : Section(skin, name),
	m_X(),
	m_Y(),
	m_W(0),
	m_H(0),
	m_Hidden(false),
	m_WDefined(false),
	m_HDefined(false),
	m_RelativeMeter(),
	m_Transformation(),
	m_ToolTipWidth(),
	m_ToolTipType(false),
	m_ToolTipHidden(skin->GetMeterToolTipHidden()),
	m_ToolTipDisabled(false),
	m_ToolTipHandle(),
	m_Mouse(skin, this),
	m_HasMouseAction(false),
	m_MouseOver(false),
	m_RelativeX(POSITION_ABSOLUTE),
	m_RelativeY(POSITION_ABSOLUTE),
	m_SolidBevel(BEVELTYPE_NONE),
	m_SolidAngle(),
	m_Padding(),
	m_AntiAlias(false),
	m_Initialized(false)
{
}

Meter::~Meter()
{
	delete m_Transformation;

	if (m_ToolTipHandle != nullptr)
	{
		DestroyWindow(m_ToolTipHandle);
	}
}

/*
** Initializes the meter. Usually this method is overwritten by the inherited
** classes, which load bitmaps and such things during initialization.
**
*/
void Meter::Initialize()
{
	m_Initialized = true;
}

/*
** Returns the X-position of the meter.
**
*/
int Meter::GetX(bool abs)
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
int Meter::GetY(bool abs)
{
	if (m_RelativeY != POSITION_ABSOLUTE && m_RelativeMeter)
	{
		if (m_RelativeY == POSITION_RELATIVE_TL)
		{
			return m_RelativeMeter->GetY(true) + m_Y;
		}
		else
		{
			return m_RelativeMeter->GetY(true) + m_RelativeMeter->GetH() + m_Y;
		}
	}
	return m_Y;
}

void Meter::SetX(int x)
{
	m_X = x;
	m_RelativeX = POSITION_ABSOLUTE;

	// Change the option as well to avoid reset in ReadOptions().
	WCHAR buffer[32];
	_itow_s(x, buffer, 10);
	m_Skin->GetParser().SetValue(m_Name, L"X", buffer);
}

void Meter::SetY(int y)
{
	m_Y = y;
	m_RelativeY = POSITION_ABSOLUTE;

	// Change the option as well to avoid reset in ReadOptions().
	WCHAR buffer[32];
	_itow_s(y, buffer, 10);
	m_Skin->GetParser().SetValue(m_Name, L"Y", buffer);
}

/*
** Returns a RECT containing the dimensions of the meter within the Skin
**
*/
RECT Meter::GetMeterRect()
{
	RECT meterRect;

	meterRect.left = GetX();
	meterRect.top = GetY();
	meterRect.right = meterRect.left + m_W;
	meterRect.bottom = meterRect.top + m_H;

	return meterRect;
}

/*
** Returns a Rect containing the adjusted meter location with "Padding" option
**
*/
Gdiplus::Rect Meter::GetMeterRectPadding()
{
	Gdiplus::Rect meterRect;

	meterRect.X = GetX() + m_Padding.X;
	meterRect.Y = GetY() + m_Padding.Y;
	meterRect.Width = m_W - m_Padding.X - m_Padding.Width;
	meterRect.Height = m_H - m_Padding.Y - m_Padding.Height;

	return meterRect;
}

/*
** Checks if the given point is inside the meter.
** This function doesn't check Hidden state, so check it before calling this function if needed.
**
*/
bool Meter::HitTest(int x, int y)
{
	int p;
	return (x >= (p = GetX()) && x < p + m_W && y >= (p = GetY()) && y < p + m_H);
}

/*
** Shows the meter and tooltip.
**
*/
void Meter::Show()
{
	m_Hidden = false;

	// Change the option as well to avoid reset in ReadOptions().
	m_Skin->GetParser().SetValue(m_Name, L"Hidden", L"0");

	if (m_ToolTipHandle != nullptr)
	{
		if (!m_ToolTipHidden)
		{
			SendMessage(m_ToolTipHandle, TTM_ACTIVATE, TRUE, 0);
		}
	}
}

/*
** Hides the meter and tooltip.
**
*/
void Meter::Hide()
{
	m_Hidden = true;

	// Change the option as well to avoid reset in ReadOptions().
	m_Skin->GetParser().SetValue(m_Name, L"Hidden", L"1");

	if (m_ToolTipHandle != nullptr)
	{
		SendMessage(m_ToolTipHandle, TTM_ACTIVATE, FALSE, 0);
	}
}

/*
** Read the common options specified in the ini file. The inherited classes must
** call this base implementation if they overwrite this method.
**
*/
void Meter::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	// The MeterStyle defines a template where the values are read if the meter doesn't have it itself
	const std::wstring& style = parser.ReadString(section, L"MeterStyle", L"");
	if (!style.empty())
	{
		parser.SetStyleTemplate(style);
	}

	Section::ReadOptions(parser, section);

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

	static const Gdiplus::Rect defPadding;
	m_Padding = parser.ReadRect(section, L"Padding", defPadding);

	const int oldW = m_W;
	const bool oldWDefined = m_WDefined;
	const int widthPadding = GetWidthPadding();

	const int w = parser.ReadInt(section, L"W", m_W);
	m_WDefined = parser.GetLastValueDefined();

	if (IsFixedSize(true)) m_W = w;
	if (oldW != (m_W - widthPadding)) m_W += widthPadding;
	if (!m_WDefined && oldWDefined && IsFixedSize())
	{
		m_W = 0;
	}
	
	const int oldH = m_H;
	const bool oldHDefined = m_HDefined;
	const int heightPadding = GetHeightPadding();

	const int h = parser.ReadInt(section, L"H", m_H);
	m_HDefined = parser.GetLastValueDefined();
	
	if (IsFixedSize(true)) m_H = h;
	if (oldH != (m_H - heightPadding)) m_H += heightPadding;
	if (!m_HDefined && oldHDefined && IsFixedSize())
	{
		m_H = 0;
	}

	bool oldHidden = m_Hidden;
	m_Hidden = parser.ReadBool(section, L"Hidden", false);

	if (oldX != m_X || oldY != m_Y || oldHidden != m_Hidden)
	{
		m_Skin->SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
	}

	m_SolidBevel = (BEVELTYPE)parser.ReadInt(section, L"BevelType", BEVELTYPE_NONE);

	m_SolidColor = parser.ReadColor(section, L"SolidColor", Color::MakeARGB(0, 0, 0, 0));
	m_SolidColor2 = parser.ReadColor(section, L"SolidColor2", m_SolidColor.GetValue());
	m_SolidAngle = (Gdiplus::REAL)parser.ReadFloat(section, L"GradientAngle", 0.0);

	m_Mouse.ReadOptions(parser, section);
	m_HasMouseAction = m_Mouse.HasButtonAction() || m_Mouse.HasScrollAction();

	m_ToolTipText = parser.ReadString(section, L"ToolTipText", L"");
	m_ToolTipTitle = parser.ReadString(section, L"ToolTipTitle", L"");
	m_ToolTipIcon = parser.ReadString(section, L"ToolTipIcon", L"");
	m_ToolTipWidth = parser.ReadInt(section, L"ToolTipWidth", 1000);
	m_ToolTipType = parser.ReadBool(section, L"ToolTipType", false);
	m_ToolTipHidden = parser.ReadBool(section, L"ToolTipHidden", m_Skin->GetMeterToolTipHidden());

	m_AntiAlias = parser.ReadBool(section, L"AntiAlias", false);

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
		m_Transformation = nullptr;

		LogErrorF(this, L"Meter: Incorrect number of values in TransformationMatrix=%s", parser.ReadString(section, L"TransformationMatrix", L"").c_str());
	}
}

/*
** Binds this meter to the given measure. The same measure can be bound to
** several meters but one meter and only be bound to one measure.
**
*/
void Meter::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	BindPrimaryMeasure(parser, section, false);
}

/*
** Creates the given meter. This is the factory method for the meters.
** If new meters are implemented this method needs to be updated.
**
*/
Meter* Meter::Create(const WCHAR* meter, Skin* skin, const WCHAR* name)
{
	if (_wcsicmp(L"STRING", meter) == 0)
	{
		return new MeterString(skin, name);
	}
	else if (_wcsicmp(L"IMAGE", meter) == 0)
	{
		return new MeterImage(skin, name);
	}
	else if (_wcsicmp(L"HISTOGRAM", meter) == 0)
	{
		return new MeterHistogram(skin, name);
	}
	else if (_wcsicmp(L"BAR", meter) == 0)
	{
		return new MeterBar(skin, name);
	}
	else if (_wcsicmp(L"BITMAP", meter) == 0)
	{
		return new MeterBitmap(skin, name);
	}
	else if (_wcsicmp(L"LINE", meter) == 0)
	{
		return new MeterLine(skin, name);
	}
	else if (_wcsicmp(L"ROUNDLINE", meter) == 0)
	{
		return new MeterRoundLine(skin, name);
	}
	else if (_wcsicmp(L"ROTATOR", meter) == 0)
	{
		return new MeterRotator(skin, name);
	}
	else if (_wcsicmp(L"BUTTON", meter) == 0)
	{
		return new MeterButton(skin, name);
	}
	else if (_wcsicmp(L"SHAPE", meter) == 0)
	{
		return new MeterShape(skin, name);
	}

	LogErrorF(skin, L"Meter=%s is not valid in [%s]", meter, name);

	return nullptr;
}

/*
** Updates the value(s) from the measures. Derived classes should
** only update if this returns true;
*/
bool Meter::Update()
{
	// Only update the meter's value when the divider is equal to the counter
	return UpdateCounter();
}

/*
** Reads and binds the primary MeasureName. This must always be called in overridden
** BindMeasures() implementations.
**
*/
bool Meter::BindPrimaryMeasure(ConfigParser& parser, const WCHAR* section, bool optional)
{
	m_Measures.clear();

	const std::wstring& measureName = parser.ReadString(section, L"MeasureName", L"");

	Measure* measure = parser.GetMeasure(measureName);
	if (measure)
	{
		m_Measures.push_back(measure);
		return true;
	}
	else if (!optional)
	{
		LogErrorF(this, L"MeasureName=%s is not valid", measureName.c_str());
	}

	return false;
}

/*
** Reads and binds secondary measures (MeasureName2 - MeasureNameN).
**
*/
void Meter::BindSecondaryMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (!m_Measures.empty())
	{
		WCHAR tmpName[64];

		int i = 2;
		do
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"MeasureName%i", i);
			const std::wstring& measureName = parser.ReadString(section, tmpName, L"");
			Measure* measure = parser.GetMeasure(measureName);
			if (measure)
			{
				m_Measures.push_back(measure);
			}
			else
			{
				if (!measureName.empty())
				{
					LogErrorF(this, L"MeasureName%i=%s is not valid", i, measureName.c_str());
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
bool Meter::ReplaceMeasures(std::wstring& str, AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	bool replaced = false;

	if (str.find(L'%') != std::wstring::npos)
	{
		WCHAR buffer[64];

		for (size_t i = m_Measures.size(); i > 0; --i)
		{
			size_t len = _snwprintf_s(buffer, _TRUNCATE, L"%%%i", (int)i);
			size_t start = 0, pos;

			const WCHAR* measureValue = m_Measures[i - 1]->GetStringOrFormattedValue(
				autoScale, scale, decimals, percentual);
			const size_t measureValueLen = wcslen(measureValue);

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
void Meter::CreateToolTip(Skin* skin)
{
	HWND hSkin = m_Skin->GetWindow();
	HINSTANCE hInstance = GetRainmeter().GetModuleInstance();
	DWORD style = WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP;

	if (m_ToolTipType)
	{
		style |= TTS_BALLOON;
	}

	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		nullptr,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hSkin,
		nullptr,
		hInstance,
		nullptr);

	if (hwndTT)
	{
		SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		TOOLINFO ti = {sizeof(TOOLINFO), TTF_SUBCLASS, hSkin, 0, GetMeterRect(), hInstance};

		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);

		m_ToolTipHandle = hwndTT;
		UpdateToolTip();
	}
}

/*
** Updates the ToolTip to match new values
*/
void Meter::UpdateToolTip()
{
	HWND hwndTT = m_ToolTipHandle;

	TOOLINFO ti = {sizeof(TOOLINFO)};
	ti.hwnd = m_Skin->GetWindow();

	SendMessage(hwndTT, TTM_GETTOOLINFO, 0, (LPARAM)&ti);

	std::wstring text = m_ToolTipTitle;
	if (!text.empty())
	{
		HICON hIcon = nullptr;
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
				hIcon = LoadIcon(nullptr, IDI_QUESTION);
			}
			else if (_wcsicmp(tipIcon, L"SHIELD") == 0)
			{
				hIcon = LoadIcon(nullptr, IDI_SHIELD);
			}
			else
			{
				std::wstring iconPath = m_ToolTipIcon;
				m_Skin->MakePathAbsolute(iconPath);
				hIcon = (HICON)LoadImage(nullptr, iconPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
				destroy = true;
			}
		}

		ReplaceMeasures(text);
		SendMessage(hwndTT, TTM_SETTITLE, (WPARAM)hIcon, (LPARAM)text.c_str());

		if (destroy)
		{
			DestroyIcon(hIcon);
		}
	}
	else
	{
		SendMessage(hwndTT, TTM_SETTITLE, (WPARAM)nullptr, (LPARAM)L"");
	}

	text = m_ToolTipText;
	ReplaceMeasures(text);
	ti.lpszText = (LPTSTR)text.c_str();
	ti.rect = GetMeterRect();

	SendMessage(hwndTT, TTM_SETTOOLINFO, 0, (LPARAM)&ti);
	SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, m_ToolTipWidth);

	if (m_ToolTipHidden || m_ToolTipDisabled)
	{
		SendMessage(hwndTT, TTM_ACTIVATE, FALSE, 0);
	}
	else
	{
		SendMessage(hwndTT, TTM_ACTIVATE, !IsHidden(), 0);
	}
}

/*
** Draws the solid background & bevel if such are defined
*/
bool Meter::Draw(Gfx::Canvas& canvas)
{
	if (IsHidden()) return false;

	canvas.SetAntiAliasing(m_AntiAlias);

	if (m_SolidColor.GetA() != 0 || m_SolidColor2.GetA() != 0)
	{
		int x = GetX();
		int y = GetY();

		Rect r(x, y, m_W, m_H);

		if (m_SolidColor.GetValue() == m_SolidColor2.GetValue())
		{
			SolidBrush solid(m_SolidColor);
			canvas.FillRectangle(r, solid);
		}
		else
		{
			Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

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

			canvas.EndGdiplusContext();
		}
	}

	if (m_SolidBevel != BEVELTYPE_NONE)
	{
		Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

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

		canvas.EndGdiplusContext();
	}

	return true;
}

/*
** Draws a bevel inside the given area
*/
void Meter::DrawBevel(Graphics& graphics, const Rect& rect, const Pen& light, const Pen& dark)
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
