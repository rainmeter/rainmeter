/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_LINE_H_
#define RM_GFX_SHAPE_LINE_H_

#include "../Shape.h"

namespace Gfx {

class Line final : public Shape
{
public:
	Line(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2);
	~Line();

	virtual Shape* Clone() override;
	
private:
	Line(const Line& other) = delete;
	Line& operator=(Line other) = delete;

	D2D1_POINT_2F m_StartPoint;
	D2D1_POINT_2F m_EndPoint;
};

} // Gfx

#endif
