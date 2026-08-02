// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "GeneralImage.h"

class MeterBar : public Meter
{
public:
	MeterBar(Skin* skin, const WCHAR* name);
	virtual ~MeterBar();

	MeterBar(const MeterBar& other) = delete;
	MeterBar& operator=(MeterBar other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterBar>(); }

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return !m_Image.IsLoaded(); }

private:
	enum ORIENTATION
	{
		HORIZONTAL,
		VERTICAL
	};

	GeneralImage m_Image;
	std::wstring m_ImageName;

	D2D1_COLOR_F m_Color;
	ORIENTATION m_Orientation;	// Growth direction
	double m_Value;
	int m_Border;
	bool m_Flip;
};
