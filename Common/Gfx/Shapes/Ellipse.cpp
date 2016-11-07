/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Ellipse.h"
#include "Gfx/Canvas.h"
#include "../Library/Logger.h"

namespace Gfx {

Ellipse::Ellipse(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius) : Shape(ShapeType::Ellipse),
	m_CenterPoint(D2D1::Point2F(x, y)),
	m_RadiusX(xRadius),
	m_RadiusY(yRadius)
{
	HRESULT hr = E_FAIL;
	const D2D1_ELLIPSE ellipse = D2D1::Ellipse(m_CenterPoint, m_RadiusX, m_RadiusY);

	Microsoft::WRL::ComPtr<ID2D1EllipseGeometry> geometry;
	hr = Canvas::c_D2DFactory->CreateEllipseGeometry(ellipse, geometry.GetAddressOf());
	if (FAILED(hr))
	{
		LogErrorF(
			L"Could not create ellipse object. X=%i, Y=%i, RadiusX=%i, RadiusY=%i",
			(int)x, (int)y, (int)xRadius, (int)yRadius);
		return;
	}

	hr = geometry.CopyTo(m_Shape.GetAddressOf());
	if (FAILED(hr)) LogErrorF(
		L"Could not copy ellipse object to shape object. X=%i, Y=%i, RadiusX=%i, RadiusY=%i",
		(int)x, (int)y, (int)xRadius, (int)yRadius);
}

Ellipse::~Ellipse()
{
}

Shape* Ellipse::Clone()
{
	Shape* newShape = new Ellipse(m_CenterPoint.x, m_CenterPoint.y, m_RadiusX, m_RadiusY);
	CloneModifiers(newShape);
	return newShape;
}

}  // namespace Gfx
