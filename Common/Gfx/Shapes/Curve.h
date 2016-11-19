/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_CURVE_H_
#define RM_GFX_SHAPE_CURVE_H_

#include "../Shape.h"

namespace Gfx {

class Curve final : public Shape
{
public:
	Curve(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2,
		FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2, D2D1_FIGURE_END ending);
	~Curve();

	virtual Shape* Clone() override;
	
private:
	Curve(const Curve& other) = delete;
	Curve& operator=(Curve other) = delete;

	D2D1_POINT_2F m_StartPoint;
	D2D1_BEZIER_SEGMENT m_BezierSegment;
	D2D1_FIGURE_END m_ShapeEnding;
};

} // Gfx

#endif
