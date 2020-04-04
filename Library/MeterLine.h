/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERLINE_H__
#define __METERLINE_H__

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

	bool m_AutoScale;
	int m_AutoScaleIndex;
	bool m_HorizontalLines;
	bool m_Flip;
	double m_LineWidth;
	D2D1_COLOR_F m_HorizontalColor;
	D2D1_STROKE_TRANSFORM_TYPE m_StrokeType;

	std::vector< std::vector<double> > m_AllValues;
	int m_CurrentPos;

	bool m_GraphStartLeft;
	bool m_GraphHorizontalOrientation;
};

#endif
