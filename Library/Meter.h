/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METER_H__
#define __METER_H__

#include <windows.h>
#include <vector>
#include <string>
#include "ConfigParser.h"
#include "Skin.h"
#include "Section.h"
#include "Measure.h"
#include "../Common/Gfx/RenderTexture.h"

class Measure;

class __declspec(novtable) Meter : public Section
{
public:
	virtual ~Meter();

	Meter(const Meter& other) = delete;

	void ReadOptions(ConfigParser& parser) { ReadOptions(parser, GetName()); parser.ClearStyleTemplate(); }
	void ReadContainerOptions(ConfigParser& parser) { ReadContainerOptions(parser, GetName()); parser.ClearStyleTemplate(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);
	virtual bool HasActiveTransition() { return false; }

	virtual int GetW() { return m_Hidden ? 0 : m_W; }
	virtual int GetH() { return m_Hidden ? 0 : m_H; }
	virtual int GetX(bool abs = false);
	virtual int GetY(bool abs = false);

	RECT GetMeterRect();
	D2D1_RECT_F GetMeterRectPadding();
	int GetWidthPadding() { return (int)m_Padding.right; }
	int GetHeightPadding() { return (int)m_Padding.bottom; }

	bool GetMeterVisibleRect(RECT& rect);

	Gfx::RenderTexture* GetContainerContentTexture() { return m_ContainerContentTexture; }
	Gfx::RenderTexture* GetContainerTexture() { return m_ContainerTexture; }
	void AddContainerItem(Meter* item);
	void RemoveContainerItem(Meter* item);
	const std::vector<Meter*>& GetContainerItems() { return m_ContainerItems; }
	bool IsContained() { return m_ContainerMeter != nullptr; }
	bool IsContainer() { return m_ContainerItems.size() > 0; }
	Meter* GetContainerMeter() { return m_ContainerMeter; }
	void UpdateContainer();
	bool HitTestContainer(int& x, int& y) { return m_ContainerMeter ? m_ContainerMeter->HitTest(x, y) : true; }

	void SetW(int w) { m_W = w; }
	void SetH(int h) { m_H = h; }
	void SetX(int x);
	void SetY(int y);

	void SetRelativeMeter(Meter* meter) { m_RelativeMeter = meter; }
	Meter* GetRelativeMeter() { return m_RelativeMeter; }

	const Mouse& GetMouse() { return m_Mouse; }
	bool HasMouseAction() { return m_Mouse.HasButtonAction() || m_Mouse.HasScrollAction(); }
	void DisableMouseAction(const std::wstring& options) { m_Mouse.DisableMouseAction(options); }
	void ClearMouseAction(const std::wstring& options) { m_Mouse.ClearMouseAction(options); }
	void EnableMouseAction(const std::wstring& options) { m_Mouse.EnableMouseAction(options); }
	void ToggleMouseAction(const std::wstring& options) { m_Mouse.ToggleMouseAction(options); }

	const std::wstring& GetToolTipText() { return m_ToolTipText; }
	bool HasToolTip() { return m_ToolTipHandle != nullptr; }

	void CreateToolTip(Skin* skin);
	void UpdateToolTip();
	void DisableToolTip() { m_ToolTipDisabled = true; UpdateToolTip(); }
	void ResetToolTip() { m_ToolTipDisabled = false; UpdateToolTip(); }

	void Hide();
	void Show();
	bool IsHidden() { return m_Hidden; }

	const D2D1_MATRIX_3X2_F& GetTransformationMatrix() { return m_Transformation; }

	virtual bool HitTest(int x, int y);

	void SetMouseOver(bool over) { m_MouseOver = over; }
	bool IsMouseOver() { return m_MouseOver; }

	static Meter* Create(const WCHAR* meter, Skin* skin, const WCHAR* name);
	
	static void DrawBevel(Gfx::Canvas& canvas, const D2D1_RECT_F& rect, const D2D1_COLOR_F& light, const D2D1_COLOR_F& dark, const bool offsetMode);

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

	Meter(Skin* skin, const WCHAR* name);

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return true; }

	void ReadContainerOptions(ConfigParser& parser, const WCHAR* section);

	bool BindPrimaryMeasure(ConfigParser& parser, const WCHAR* section, bool optional);
	void BindSecondaryMeasures(ConfigParser& parser, const WCHAR* section);

	bool ReplaceMeasures(std::wstring& str, AUTOSCALE autoScale = AUTOSCALE_ON, double scale = 1.0, int decimals = 0, bool percentual = false);

	std::vector<Measure*> m_Measures;
	int m_X;
	int m_Y;
	int m_W;
	int m_H;
	bool m_Hidden;
	bool m_WDefined;
	bool m_HDefined;
	Meter* m_RelativeMeter;

	D2D1_MATRIX_3X2_F m_Transformation;

	std::wstring m_ToolTipText;
	std::wstring m_ToolTipTitle;
	std::wstring m_ToolTipIcon;
	unsigned int m_ToolTipWidth;
	bool m_ToolTipType;
	bool m_ToolTipHidden;
	bool m_ToolTipDisabled;  // Selected skins disable all tooltips
	HWND m_ToolTipHandle;

	Mouse m_Mouse;
	bool m_MouseOver;

	METER_POSITION m_RelativeX;
	METER_POSITION m_RelativeY;

	BEVELTYPE m_SolidBevel;
	D2D1_COLOR_F m_BevelColor;
	D2D1_COLOR_F m_BevelColor2;

	D2D1_COLOR_F m_SolidColor;
	D2D1_COLOR_F m_SolidColor2;
	FLOAT m_SolidAngle;
	D2D1_RECT_F m_Padding;
	bool m_AntiAlias;
	bool m_Initialized;

	Meter* m_ContainerMeter;
	std::vector<Meter*> m_ContainerItems;
	Gfx::RenderTexture* m_ContainerContentTexture;
	Gfx::RenderTexture* m_ContainerTexture;
};

#endif
