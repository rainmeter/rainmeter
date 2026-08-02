// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "GeneralImage.h"

#define BUTTON_FRAMES 3

class MeterButton : public Meter
{
public:
	MeterButton(Skin* skin, const WCHAR* name);
	virtual ~MeterButton();

	MeterButton(const MeterButton& other) = delete;
	MeterButton& operator=(MeterButton other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterButton>(); }

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

	bool MouseMove(POINT pos);
	bool MouseUp(POINT pos, bool execute);
	bool MouseDown(POINT pos);

	void SetFocus(bool f) { m_Focus = f; }

	bool HitTest2(int px, int py);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return overwrite; }

private:
	GeneralImage m_Image;
	std::wstring m_ImageName;

	D2D1_RECT_F m_BitmapsRects[BUTTON_FRAMES];	// Cached bitmaps
	std::wstring m_Command;
	int m_State;
	bool m_Clicked;
	bool m_Focus;
};
