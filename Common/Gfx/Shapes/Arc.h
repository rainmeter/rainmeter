/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_ARC_H_
#define RM_GFX_SHAPE_ARC_H_

#include "../Shape.h"

namespace Gfx {

class Arc final : public Shape
{
public:
	Arc(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT xRadius, FLOAT yRadius, FLOAT angle,
		D2D1_SWEEP_DIRECTION sweep, D2D1_ARC_SIZE size, D2D1_FIGURE_END ending);
	~Arc();

	virtual Shape* Clone() override;
	
private:
	Arc(const Arc& other) = delete;
	Arc& operator=(Arc other) = delete;

	D2D1_POINT_2F m_StartPoint;
	D2D1_ARC_SEGMENT m_ArcSegment;
	D2D1_FIGURE_END m_ShapeEnding;
};

} // Gfx

#endif
