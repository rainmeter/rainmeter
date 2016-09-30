/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Rectangle.h"
#include "Gfx\Canvas.h"

namespace Gfx {

Rectangle::Rectangle(FLOAT x, FLOAT y, FLOAT width, FLOAT height) : Shape(ShapeType::Rectangle),
	m_X(x),
	m_Y(y),
	m_Width(width + x),
	m_Height(height + y)
{
	HRESULT hr = E_FAIL;
	const D2D1_RECT_F rect = { m_X, m_Y, m_Width, m_Height };

	Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> rectangle;
	hr = Canvas::c_D2DFactory->CreateRectangleGeometry(rect, &rectangle);

	if (!FAILED(hr)) hr = rectangle.CopyTo(m_Shape.GetAddressOf());

	if (FAILED(hr))
	{
		//LogErrorF();
	}
}

Rectangle::~Rectangle()
{
}

}  // namespace Gfx
