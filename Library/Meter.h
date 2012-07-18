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

#ifndef __METER_H__
#define __METER_H__

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include "Litestep.h"
#include "ConfigParser.h"
#include "MeterWindow.h"
#include "Measure.h"
#include "Group.h"

class CMeasure;

class CMeter : public CGroup
{
public:
	CMeter(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeter();

	virtual UINT GetTypeID() = 0;

	void ReadOptions(CConfigParser& parser) { ReadOptions(parser, GetName()); parser.ClearStyleTemplate(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual bool HasActiveTransition() { return false; }
	
	bool HasDynamicVariables() { return m_DynamicVariables; }
	void SetDynamicVariables(bool b) { m_DynamicVariables = b; }

	virtual int GetW() { return m_Hidden ? 0 : m_W; }
	virtual int GetH() { return m_Hidden ? 0 : m_H; }
	virtual int GetX(bool abs = false);
	virtual int GetY(bool abs = false);
	RECT GetMeterRect();

	void SetW(int w) { m_W = w; }
	void SetH(int h) { m_H = h; }
	void SetX(int x);
	void SetY(int y);

	void SetRelativeMeter(CMeter* meter) { m_RelativeMeter = meter; }

	const CMouse& GetMouse() { return m_Mouse; }
	bool HasMouseAction() { return m_HasMouseAction; }

	const std::wstring& GetToolTipText() { return m_ToolTipText; }
	bool HasToolTip() { return m_ToolTipHandle != NULL; }

	void CreateToolTip(CMeterWindow* meterWindow);
	void UpdateToolTip();

	virtual void Hide();
	virtual void Show();
	bool IsHidden() { return m_Hidden; }

	const Gdiplus::Matrix* GetTransformationMatrix() { return m_Transformation; }

	virtual bool HitTest(int x, int y);

	void SetMouseOver(bool over) { m_MouseOver = over; }
	bool IsMouseOver() { return m_MouseOver; }

	const WCHAR* GetName() { return m_Name.c_str(); }
	const std::wstring& GetOriginalName() { return m_Name; }

	void ResetUpdateCounter() { m_UpdateCounter = m_UpdateDivider; }
	int GetUpdateCounter() { return m_UpdateCounter; }
	int GetUpdateDivider() { return m_UpdateDivider; }

	CMeterWindow* GetMeterWindow() { return m_MeterWindow; }

	static CMeter* Create(const WCHAR* meter, CMeterWindow* meterWindow, const WCHAR* name);
	
	static void DrawBevel(Gdiplus::Graphics& graphics, const Gdiplus::Rect& rect, const Gdiplus::Pen& light, const Gdiplus::Pen& dark);

protected:

	enum METER_ALIGNMENT
	{
		ALIGN_LEFT,				// Same as LeftTop
		ALIGN_RIGHT,			// Same as RightTop
		ALIGN_CENTER,			// Same as CenterTop
		ALIGN_LEFTBOTTOM,
		ALIGN_RIGHTBOTTOM,
		ALIGN_CENTERBOTTOM,
		ALIGN_LEFTCENTER,
		ALIGN_RIGHTCENTER,
		ALIGN_CENTERCENTER
	};
	
	enum METER_POSITION
	{
		POSITION_ABSOLUTE,
		POSITION_RELATIVE_TL,
		POSITION_RELATIVE_BR
	};

	virtual void ReadOptions(CConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(CConfigParser& parser, const WCHAR* section);

	bool BindPrimaryMeasure(CConfigParser& parser, const WCHAR* section, bool optional);
	void BindSecondaryMeasures(CConfigParser& parser, const WCHAR* section);

	bool ReplaceMeasures(std::wstring& str, AUTOSCALE autoScale = AUTOSCALE_ON, double scale = 1.0, int decimals = 0, bool percentual = false);

	const std::wstring m_Name;
	std::vector<CMeasure*> m_Measures;
	int m_X;
	int m_Y;
	int m_W;
	int m_H;
	bool m_Hidden;
	bool m_WDefined;
	bool m_HDefined;
	CMeter*	m_RelativeMeter;
	bool m_DynamicVariables;

	Gdiplus::Matrix* m_Transformation;

	std::wstring m_ToolTipText;
	std::wstring m_ToolTipTitle;
	std::wstring m_ToolTipIcon;
	unsigned int m_ToolTipWidth;
	bool m_ToolTipType;
	bool m_ToolTipHidden;

	HWND m_ToolTipHandle;

	CMouse m_Mouse;
	bool m_HasMouseAction;
	bool m_MouseOver;

	METER_POSITION m_RelativeX;
	METER_POSITION m_RelativeY;

	int m_UpdateDivider;
	int m_UpdateCounter;

	BEVELTYPE m_SolidBevel;
	Gdiplus::Color m_SolidColor;
	Gdiplus::Color m_SolidColor2;
	Gdiplus::REAL m_SolidAngle;
	bool m_AntiAlias;
	bool m_Initialized;

	CMeterWindow* m_MeterWindow;
};

#endif
