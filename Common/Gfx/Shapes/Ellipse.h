/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_ELLIPSE_H_
#define RM_GFX_SHAPE_ELLIPSE_H_

#include "../Shape.h"

namespace Gfx {

class Ellipse final : public Shape
{
public:
	Ellipse(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius);
	~Ellipse ();

	virtual Shape* Clone() override;
	
private:
	Ellipse(const Ellipse& other) = delete;
	Ellipse& operator=(Ellipse other) = delete;

	D2D1_POINT_2F m_CenterPoint;
	FLOAT m_RadiusX;
	FLOAT m_RadiusY;
};

} // Gfx

#endif
