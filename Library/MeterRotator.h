// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "GeneralImage.h"

class MeterRotator : public Meter
{
public:
	MeterRotator(Skin* skin, const WCHAR* name);
	virtual ~MeterRotator();

	MeterRotator(const MeterRotator& other) = delete;
	MeterRotator& operator=(MeterRotator other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterRotator>(); }

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

private:
	GeneralImage m_Image;
	std::wstring m_ImageName;

	double m_OffsetX;
	double m_OffsetY;
	double m_StartAngle;
	double m_RotationAngle;
	UINT m_ValueRemainder;
	double m_Value;
};
