// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"

class MeterLine : public Meter
{
public:
	MeterLine(Skin* skin, const WCHAR* name);
	virtual ~MeterLine();

	MeterLine(const MeterLine& other) = delete;
	MeterLine& operator=(MeterLine other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterLine>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	std::vector<D2D1_COLOR_F> m_Colors;
	std::vector<double> m_ScaleValues;

	bool m_Autoscale;
	bool m_HorizontalLines;
	bool m_Flip;
	double m_LineWidth;
	D2D1_COLOR_F m_HorizontalColor;
	D2D1_STROKE_TRANSFORM_TYPE m_StrokeType;
	D2D1_LINE_JOIN m_LineJoin;
	FLOAT m_MiterLimit;

	std::vector<std::vector<double>> m_AllValues;
	int m_CurrentPos;

	bool m_GraphStartLeft;
	bool m_GraphHorizontalOrientation;
};
