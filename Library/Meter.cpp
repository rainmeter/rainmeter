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

Meter::Meter(Skin* skin, const WCHAR* name) : Section(skin, name),
	m_X(),
	m_Y(),
	m_W(0),
	m_H(0),
	m_Hidden(false),
	m_WDefined(false),
	m_HDefined(false),
	m_RelativeMeter(),
	m_Transformation(D2D1::Matrix3x2F::Identity()),
	m_ToolTipWidth(),
	m_ToolTipType(false),
	m_ToolTipHidden(skin->GetMeterToolTipHidden()),
	m_ToolTipDisabled(false),
	m_ToolTipHandle(),
	m_Mouse(skin, this),
	m_MouseOver(false),
	m_RelativeX(POSITION_ABSOLUTE),
	m_RelativeY(POSITION_ABSOLUTE),
	m_SolidBevel(BEVELTYPE_NONE),
	m_BevelColor(Gfx::Util::c_Transparent_Color_F),
	m_BevelColor2(Gfx::Util::c_Transparent_Color_F),
	m_SolidAngle(),
	m_Padding(),
	m_AntiAlias(false),
	m_Initialized(false),
	m_ContainerMeter(nullptr),
	m_ContainerContentTexture(nullptr),
	m_ContainerTexture(nullptr),
	m_ContainerItems(),
	m_SolidColor(Gfx::Util::c_Transparent_Color_F),
	m_SolidColor2(Gfx::Util::c_Transparent_Color_F)
{
}

Meter::~Meter()
{
	if (m_ToolTipHandle)
	{
		DestroyWindow(m_ToolTipHandle);
	}

	if (m_ContainerContentTexture)
	{
		delete m_ContainerContentTexture;
		m_ContainerContentTexture = nullptr;
	}

	if (m_ContainerTexture)
	{
		delete m_ContainerTexture;
		m_ContainerTexture = nullptr;
	}

	m_ContainerMeter = nullptr;
	m_ContainerItems.clear();
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
	int containerOffset = 0;
	if (m_ContainerMeter)
	{
		containerOffset = m_ContainerMeter->GetX(true);
	}

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

	return containerOffset + m_X;
}

/*
** Returns the Y-position of the meter.
**
*/
int Meter::GetY(bool abs)
{
	int containerOffset = 0;
	if (m_ContainerMeter)
	{
		containerOffset = m_ContainerMeter->GetY(true);
	}

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

	return containerOffset + m_Y;
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
	int x = GetX();
	int y = GetY();
	return { x, y, x + m_W, y + m_H };
}

/*
** Returns a Rect containing the adjusted meter location with "Padding" option
**
*/
D2D1_RECT_F Meter::GetMeterRectPadding()
{
	RECT rect = GetMeterRect();
	return D2D1::RectF(
		rect.left + m_Padding.left,
		rect.top + m_Padding.top,
		rect.right + m_Padding.left - m_Padding.right,
		rect.bottom + m_Padding.top - m_Padding.bottom);
}

/*
** Returns the visible portion of the meter or the meter's bounds
**
*/
bool Meter::GetMeterVisibleRect(RECT& rect)
{
	rect = GetMeterRect();
	if (!m_ContainerMeter) return true;

	const RECT cRect = m_ContainerMeter->GetMeterRect();
	RECT dest = { 0 };
	if (IntersectRect(&dest, &rect, &cRect))
	{
		rect = dest;
		return true;
	}
	return false;
}

/*
** Checks if the given point is inside the meter.
** This function doesn't check Hidden state, so check it before calling this function if needed.
**
*/
bool Meter::HitTest(int x, int y)
{
	if (!HitTestContainer(x, y))
	{
		return false;
	}

	int p;
	return (x >= (p = GetX()) && x < p + m_W && y >= (p = GetY()) && y < p + m_H);
}

void Meter::AddContainerItem(Meter* item)
{
	m_ContainerItems.push_back(item);
	m_Skin->ResetRelativeMeters();
	
	if (m_ContainerItems.size() == 1)
	{
		UINT width = (UINT)GetW();
		UINT height = (UINT)GetH();

		delete m_ContainerTexture;
		m_ContainerTexture = nullptr;
		m_ContainerTexture = new Gfx::RenderTexture(m_Skin->GetCanvas(), width, height);

		delete m_ContainerContentTexture;
		m_ContainerContentTexture = nullptr;
		m_ContainerContentTexture = new Gfx::RenderTexture(m_Skin->GetCanvas(), width, height);
	}
}

void Meter::RemoveContainerItem(Meter* item)
{
	m_ContainerItems.erase(std::remove(m_ContainerItems.begin(), m_ContainerItems.end(), item));
	m_Skin->ResetRelativeMeters();

	if (m_ContainerItems.size() == 0)
	{
		if (m_ContainerContentTexture != nullptr)
		{
			delete m_ContainerContentTexture;
			m_ContainerContentTexture = nullptr;
		}

		if (m_ContainerTexture != nullptr)
		{
			delete m_ContainerTexture;
			m_ContainerTexture = nullptr;
		}
	}
}

void Meter::UpdateContainer()
{
	UINT width = (UINT)GetW();
	UINT height = (UINT)GetH();

	if (m_ContainerTexture) m_ContainerTexture->Resize(m_Skin->GetCanvas(), width, height);

	if (m_ContainerContentTexture) m_ContainerContentTexture->Resize(m_Skin->GetCanvas(), width, height);
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

	static const D2D1_RECT_F defPadding = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
	m_Padding = parser.ReadRect(section, L"Padding", defPadding);

	const int oldW = m_W;
	const bool oldWDefined = m_WDefined;
	const int widthPadding = GetWidthPadding();

	const int w = parser.ReadInt(section, L"W", m_W);
	m_WDefined = parser.GetLastValueDefined();

	if (IsFixedSize(true)) m_W = w;
	if (!m_Initialized || oldW != (m_W - widthPadding)) m_W += widthPadding;
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
	if (!m_Initialized || oldH != (m_H - heightPadding)) m_H += heightPadding;
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
	m_BevelColor = parser.ReadColor(section, L"BevelColor", D2D1::ColorF(D2D1::ColorF::White));
	m_BevelColor2 = parser.ReadColor(section, L"BevelColor2", D2D1::ColorF(D2D1::ColorF::Black));

	m_SolidColor = parser.ReadColor(section, L"SolidColor", Gfx::Util::c_Transparent_Color_F);
	m_SolidColor2 = parser.ReadColor(section, L"SolidColor2", m_SolidColor);
	m_SolidAngle = (FLOAT)parser.ReadFloat(section, L"GradientAngle", 0.0);

	m_Mouse.ReadOptions(parser, section);

	m_ToolTipText = parser.ReadString(section, L"ToolTipText", L"");
	m_ToolTipTitle = parser.ReadString(section, L"ToolTipTitle", L"");
	m_ToolTipIcon = parser.ReadString(section, L"ToolTipIcon", L"");
	m_ToolTipWidth = parser.ReadInt(section, L"ToolTipWidth", 1000);
	m_ToolTipType = parser.ReadBool(section, L"ToolTipType", false);
	m_ToolTipHidden = parser.ReadBool(section, L"ToolTipHidden", m_Skin->GetMeterToolTipHidden());

	m_AntiAlias = parser.ReadBool(section, L"AntiAlias", false);

	std::vector<FLOAT> matrix = parser.ReadFloats(section, L"TransformationMatrix");
	if (matrix.size() == 6)
	{
		m_Transformation = D2D1::Matrix3x2F(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
	}
	else if (!matrix.empty())
	{
		m_Transformation = D2D1::Matrix3x2F::Identity();
		LogErrorF(this, L"Meter: Incorrect number of values in TransformationMatrix=%s", parser.ReadString(section, L"TransformationMatrix", L"").c_str());
	}

	ReadContainerOptions(parser, section);
}

void Meter::ReadContainerOptions(ConfigParser& parser, const WCHAR* section)
{
	const std::wstring& style = parser.ReadString(section, L"MeterStyle", L"");
	if (!style.empty())
	{
		parser.SetStyleTemplate(style);
	}

	const std::wstring& container = parser.ReadString(section, L"Container", L"");
	if (_wcsicmp(section, container.c_str()) == 0)
	{
		LogErrorF(this, L"Container cannot self-reference: %s", container.c_str());
		return;
	}

	if (!m_ContainerMeter || _wcsicmp(m_ContainerMeter->GetName(), container.c_str()) != 0)
	{
		if (m_ContainerMeter)
		{
			m_ContainerMeter->RemoveContainerItem(this);
			m_ContainerMeter = nullptr;
		}

		auto meter = m_Skin->GetMeter(container);
		if (meter)
		{
			meter->AddContainerItem(this);
			m_ContainerMeter = meter;
		}
		else if (!container.empty())
		{
			LogErrorF(this, L"Invalid container: %s", container.c_str());
		}
	}

	// The first contained meter in a container is required to be relative to
	// the top/left of the container meter
	if (m_ContainerMeter)
	{
		const auto& items = m_ContainerMeter->GetContainerItems();
		if (items.size() > 0 && items[0] == this)
		{
			m_RelativeX = m_RelativeY = POSITION_RELATIVE_TL;
		}
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

		RECT rc = { 0 };
		GetMeterVisibleRect(rc);

		TOOLINFO ti = { sizeof(TOOLINFO), TTF_SUBCLASS, hSkin, 0ULL, rc, hInstance };

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

	const bool isVisible = GetMeterVisibleRect(ti.rect);

	SendMessage(hwndTT, TTM_SETTOOLINFO, 0, (LPARAM)&ti);
	SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, m_ToolTipWidth);

	if (m_ToolTipHidden || m_ToolTipDisabled || !isVisible)
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

	if (m_SolidColor.a != 0.0f || m_SolidColor2.a != 0.0f)
	{
		const FLOAT x = (FLOAT)GetX();
		const FLOAT y = (FLOAT)GetY();

		const D2D1_RECT_F r = D2D1::RectF(x, y, x + (FLOAT)m_W, y + (FLOAT)m_H);

		if (m_SolidColor.r == m_SolidColor2.r && m_SolidColor.g == m_SolidColor2.g && 
			m_SolidColor.b == m_SolidColor2.b && m_SolidColor.a == m_SolidColor2.a)
		{
			canvas.FillRectangle(r, m_SolidColor);
		}
		else
		{
			canvas.FillGradientRectangle(r, m_SolidColor, m_SolidColor2, (FLOAT)m_SolidAngle);
		}
	}

	if (m_SolidBevel != BEVELTYPE_NONE)
	{
		D2D1_COLOR_F lightColor = m_BevelColor;
		D2D1_COLOR_F darkColor = m_BevelColor2;
		
		if (m_SolidBevel == BEVELTYPE_DOWN)
		{
			std::swap(lightColor, darkColor);
		}

		// The bevel is drawn outside the meter
		const FLOAT x = (FLOAT)GetX();
		const FLOAT y = (FLOAT)GetY();
		const D2D1_RECT_F rect = D2D1::RectF(x - 2.0f, y - 2.0f, x + (FLOAT)m_W + 2.0f, y + (FLOAT)m_H + 2.0f);
		DrawBevel(canvas, rect, lightColor, darkColor);
	}

	return true;
}

/*
** Draws a bevel inside the given area
*/
void Meter::DrawBevel(Gfx::Canvas& canvas, const D2D1_RECT_F& rect, const D2D1_COLOR_F& light, const D2D1_COLOR_F& dark)
{
	const FLOAT w = 1.0f;
	const FLOAT l = rect.left   + w;
	const FLOAT r = rect.right  - w;
	const FLOAT t = rect.top    + w;
	const FLOAT b = rect.bottom - w;

	// GDI+ offset for innermost lines
	const FLOAT o = 0.155f;

	canvas.DrawLine(light, l,         t,         l,         b,         w);
	canvas.DrawLine(light, l,         t,         r,         t,         w);
	canvas.DrawLine(light, l + w - o, t + w,     l + w - o, b - w,     w - (2 * o));
	canvas.DrawLine(light, l + w,     t + w - o, r - w,     t + w - o, w - (2 * o));
	canvas.DrawLine(dark,  l,         b,         r,         b,         w);
	canvas.DrawLine(dark,  r,         t,         r,         b,         w);
	canvas.DrawLine(dark,  l + w,     b - w,     r - w,     b - w,     w);
	canvas.DrawLine(dark,  r - w,     t + w,     r - w,     b - w,     w);
}
