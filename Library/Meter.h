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

#ifndef __METER_H__
#define __METER_H__

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include "Litestep.h"
#include "ConfigParser.h"
#include "MeterWindow.h"
#include "Group.h"

class CMeasure;

class CMeter : public CGroup
{
public:
	CMeter(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeter();

	void ReadConfig(CConfigParser& parser) { ReadConfig(parser, GetName()); parser.ClearStyleTemplate(); }

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual void BindMeasure(const std::list<CMeasure*>& measures);
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
	void SetX(int x) { m_X = x; m_RelativeX = POSITION_ABSOLUTE; }
	void SetY(int y) { m_Y = y; m_RelativeY = POSITION_ABSOLUTE; }

	const std::wstring& GetRightMouseDownAction() { return m_RightMouseDownAction; }
	const std::wstring& GetRightMouseUpAction() { return m_RightMouseUpAction; }
	const std::wstring& GetRightMouseDoubleClickAction() { return m_RightMouseDoubleClickAction; }
	const std::wstring& GetLeftMouseDownAction() { return m_LeftMouseDownAction; }
	const std::wstring& GetLeftMouseUpAction() { return m_LeftMouseUpAction; }
	const std::wstring& GetLeftMouseDoubleClickAction() { return m_LeftMouseDoubleClickAction; }
	const std::wstring& GetMiddleMouseDownAction() { return m_MiddleMouseDownAction; }
	const std::wstring& GetMiddleMouseUpAction() { return m_MiddleMouseUpAction; }
	const std::wstring& GetMiddleMouseDoubleClickAction() { return m_MiddleMouseDoubleClickAction; }
	const std::wstring& GetMouseOverAction() { return m_MouseOverAction; }
	const std::wstring& GetMouseLeaveAction() { return m_MouseLeaveAction; }

	const std::wstring& GetToolTipText() { return m_ToolTipText; }
	bool HasToolTip() { return m_ToolTipHandle != NULL; }

	void CreateToolTip(CMeterWindow* meterWindow);
	void UpdateToolTip();

	bool HasMouseAction() { return m_HasMouseAction; }
	bool HasMouseActionCursor() { return m_MouseActionCursor; }

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
		ALIGN_LEFT,
		ALIGN_RIGHT,
		ALIGN_CENTER
	};

	enum METER_POSITION
	{
		POSITION_ABSOLUTE,
		POSITION_RELATIVE_TL,
		POSITION_RELATIVE_BR
	};

	void SetAllMeasures(CMeasure* measure);
	void SetAllMeasures(const std::vector<CMeasure*>& measures);
	void ReplaceToolTipMeasures(std::wstring& str);

	static void ReadMeasureNames(CConfigParser& parser, const WCHAR* section, std::vector<std::wstring>& measureNames);
	static bool ReplaceMeasures(const std::vector<std::wstring>& stringValues, std::wstring& str);

	const std::wstring m_Name;			// Name of the meter
	std::wstring m_MeasureName;	// Name of the measure this is bound to
	CMeasure* m_Measure;		// Pointer to the measure this meter is bound to
	std::vector<CMeasure*> m_AllMeasures;
	int	m_X;					// X-position of the meter
	int	m_Y;					// Y-position of the meter
	int	m_W;					// Width of the meter
	int	m_H;					// Height of the meter
	bool m_Hidden;				// Status of the meter
	bool m_WDefined;
	bool m_HDefined;
	CMeter*	m_RelativeMeter;
	bool m_DynamicVariables;		// If true, the measure contains dynamic variables

	Gdiplus::Matrix* m_Transformation;	// The transformation matrix

	std::wstring m_StyleX;
	std::wstring m_StyleY;
	std::wstring m_StyleHidden;

	std::wstring m_ToolTipText;
	std::wstring m_ToolTipTitle;
	std::wstring m_ToolTipIcon;
	unsigned int m_ToolTipWidth;
	unsigned int m_ToolTipDelay;
	bool m_ToolTipType;
	bool m_ToolTipHidden;

	HWND m_ToolTipHandle;

	std::wstring m_LeftMouseDownAction;		// Actions for left/right/middle mouse buttons
	std::wstring m_RightMouseDownAction;
	std::wstring m_MiddleMouseDownAction;
	std::wstring m_LeftMouseUpAction;
	std::wstring m_RightMouseUpAction;
	std::wstring m_MiddleMouseUpAction;
	std::wstring m_LeftMouseDoubleClickAction;
	std::wstring m_RightMouseDoubleClickAction;
	std::wstring m_MiddleMouseDoubleClickAction;
	std::wstring m_MouseOverAction;
	std::wstring m_MouseLeaveAction;
	
	bool m_HasMouseAction;
	bool m_MouseActionCursor;

	bool m_MouseOver;
	METER_POSITION m_RelativeX;
	METER_POSITION m_RelativeY;

	int m_UpdateDivider;			// Divider for the update
	int m_UpdateCounter;			// Current update counter

	BEVELTYPE m_SolidBevel;
	Gdiplus::Color m_SolidColor;
	Gdiplus::Color m_SolidColor2;
	Gdiplus::REAL m_SolidAngle;
	bool m_AntiAlias;				// If true, the line is antialiased
	bool m_Initialized;

	CMeterWindow* m_MeterWindow;
};

#endif
