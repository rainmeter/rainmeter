/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "RoundedRectangle.h"
#include "Gfx/Canvas.h"

namespace Gfx {

RoundedRectangle::RoundedRectangle(FLOAT x, FLOAT y, FLOAT width, FLOAT height,
	FLOAT xRadius, FLOAT yRadius, bool isCloned) : Shape(ShapeType::RoundedRectangle),
		m_X(x),
		m_Y(y),
		m_Width(width + x),
		m_Height(height + y),
		m_XRadius(xRadius),
		m_YRadius(yRadius)
{
	// Cloned shapes do not need to re-create any resources
	if (isCloned) return;

	const D2D1_ROUNDED_RECT rect = { m_X, m_Y, m_Width, m_Height, m_XRadius, m_YRadius };
	Canvas::c_D2DFactory->CreateRoundedRectangleGeometry(rect, (ID2D1RoundedRectangleGeometry**)m_Shape.GetAddressOf());
}

RoundedRectangle::~RoundedRectangle()
{
}

Shape* RoundedRectangle::Clone()
{
	RoundedRectangle* newShape = new RoundedRectangle(m_X, m_Y, m_Width - m_X, m_Height - m_Y, m_XRadius, m_YRadius, true);
	m_Shape.CopyTo(newShape->m_Shape.GetAddressOf());
	CloneModifiers(newShape);
	return newShape;
}

}  // namespace Gfx
