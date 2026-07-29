/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Ellipse.h"
#include "Gfx/Canvas.h"

namespace Gfx {

Ellipse::Ellipse(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius) : Shape(ShapeType::Ellipse),
	m_CenterPoint(D2D1::Point2F(x, y)),
	m_RadiusX(xRadius),
	m_RadiusY(yRadius)
{
	const D2D1_ELLIPSE ellipse = D2D1::Ellipse(m_CenterPoint, m_RadiusX, m_RadiusY);
	Canvas::c_D2DFactory->CreateEllipseGeometry(ellipse, (ID2D1EllipseGeometry**)m_Shape.GetAddressOf());
}

Ellipse::~Ellipse()
{
}


}  // namespace Gfx
