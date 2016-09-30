/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Shape.h"

namespace Gfx {

Shape::Shape(ShapeType type) :
	m_ShapeType(type),
	m_Offset(D2D1::SizeF(0.0f, 0.0f)),
	m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
	m_StrokeColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_StrokeWidth(1.0f),
	m_Rotation(0.0f)
{
}

Shape::~Shape()
{
}

D2D1_MATRIX_3X2_F Shape::GetShapeMatrix()
{
	D2D1_RECT_F bounds;
	m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, nullptr, &bounds);

	//TODO: make rotation and scale center optional
	return D2D1::Matrix3x2F(
		D2D1::Matrix3x2F::Rotation(m_Rotation,
			D2D1::Point2F((bounds.right - bounds.left) / 2.0f + bounds.left, (bounds.bottom - bounds.top) / 2.0f + bounds.top)) *
		D2D1::Matrix3x2F::Translation(D2D1::SizeF(m_StrokeWidth / 2.0f, m_StrokeWidth / 2.0f)) *
		D2D1::Matrix3x2F::Translation(m_Offset)
	);
}

D2D1_RECT_F Shape::GetBounds()
{
	D2D1_RECT_F bounds;
	if (m_Shape)
	{
		HRESULT result = m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, GetShapeMatrix(), &bounds);
		if (SUCCEEDED(result)) return bounds;
	}

	return D2D1::RectF();
}

bool Shape::IsShapeDefined()
{
	return m_Shape;
}

bool Shape::ContainsPoint(int x, int y)
{
	if (m_Shape)
	{
		BOOL outlineContains = false;
		BOOL fillContains = false;
		HRESULT result = m_Shape->StrokeContainsPoint(D2D1::Point2F((FLOAT)x, (FLOAT)y),
			m_StrokeWidth, nullptr, GetShapeMatrix(), &outlineContains);
		if (SUCCEEDED(result) && outlineContains) return true;

		result = m_Shape->FillContainsPoint(D2D1::Point2F((FLOAT)x, (FLOAT)y), GetShapeMatrix(), &fillContains);
		if (SUCCEEDED(result) && fillContains) return true;
	}

	return false;
}

}  // namespace Gfx
