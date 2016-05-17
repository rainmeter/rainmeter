/* Copyright (C) 2002 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERGEOMETRY_H__
#define __METERGEOMETRY_H__

#include "Meter.h"
#include <vector>
#include <ole2.h>  // For Gdiplus.h.
class MeterGeometry :
	public Meter
{
public:
	MeterGeometry(Skin* skin, const WCHAR* name);
	virtual ~MeterGeometry();

	MeterGeometry(const MeterGeometry& other) = delete;
	MeterGeometry& operator=(MeterGeometry other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterGeometry>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	enum VectorType {
		Rectangle,
		Circle,
		Arc,
		Curve,
		Custom
	};
	
private:
	std::vector<Gfx::Canvas::GeometryPoint> m_points;
	Gdiplus::Color m_fillColor;
	Gdiplus::Color m_outlineColor;
	bool m_Solid;
	bool m_connectedEdges;
	float m_lineWidth;

};
#endif