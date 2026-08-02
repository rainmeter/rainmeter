// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"

class MeterRoundLine : public Meter
{
public:
	MeterRoundLine(Skin* skin, const WCHAR* name);
	virtual ~MeterRoundLine();

	MeterRoundLine(const MeterRoundLine& other) = delete;
	MeterRoundLine& operator=(MeterRoundLine other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterRoundLine>(); }

	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	bool m_Solid;
	double m_LineWidth;
	double m_LineLength;
	double m_LineStart;
	double m_StartAngle;
	double m_RotationAngle;
	bool m_CntrlAngle;
	bool m_CntrlLineStart;
	bool m_CntrlLineLength;
	double m_LineStartShift;
	double m_LineLengthShift;
	UINT m_ValueRemainder;
	D2D1_COLOR_F m_LineColor;
	double m_Value;
};
