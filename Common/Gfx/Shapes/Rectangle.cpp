/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Rectangle.h"
#include "Gfx/Canvas.h"
#include "../Library/Logger.h"

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
	hr = Canvas::c_D2DFactory->CreateRectangleGeometry(rect, rectangle.GetAddressOf());
	if (FAILED(hr))
	{
		LogErrorF(
			L"Could not create rectangle object. X=%i, Y=%i, W=%i, H=%i",
			(int)m_X, (int)m_Y, (int)m_Width, (int)m_Height);
		return;
	}

	hr = rectangle.CopyTo(m_Shape.GetAddressOf());
	if (FAILED(hr)) LogErrorF(
		L"Could not copy rectangle object to shape object. X=%i, Y=%i, W=%i, H=%i",
		(int)m_X, (int)m_Y, (int)m_Width, (int)m_Height);
}

Rectangle::~Rectangle()
{
}

Shape* Rectangle::Clone()
{
	Shape* newShape = new Rectangle(m_X, m_Y, m_Width - m_X, m_Height - m_Y);
	CloneModifiers(newShape);
	return newShape;
}

}  // namespace Gfx
