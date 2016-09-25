/* Copyright (C) 2016 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Shape.h"

namespace Gfx {

Shape::Shape() :
	m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
	m_StrokeWidth(1),
	m_StrokeColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_Rotation(),
	m_RotationCenter(),
	m_Skew(),
	m_Scale(D2D1::SizeF(1, 1)),
	m_Offset(),
	m_Antialias(true)
{
}

Shape::~Shape()
{

}

D2D1_MATRIX_3X2_F Shape::GetShapeMatrix() const
{
	D2D1_RECT_F bounds;
	m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, nullptr, &bounds);

	//TODO: make rotation and scale center optional
	return D2D1::Matrix3x2F(
		D2D1::Matrix3x2F::Rotation(m_Rotation, D2D1::Point2F((bounds.right - bounds.left) / 2 + bounds.left, (bounds.bottom - bounds.top) / 2 + bounds.top)) *
		D2D1::Matrix3x2F::Translation(D2D1::SizeF(m_StrokeWidth / 2, m_StrokeWidth / 2)) *
		D2D1::Matrix3x2F::Skew(m_Skew.x, m_Skew.y, D2D1::Point2F(0, 0)) *
		D2D1::Matrix3x2F::Translation(m_Offset) *
		D2D1::Matrix3x2F::Scale(m_Scale, D2D1::Point2F(bounds.right + bounds.left / 2, bounds.bottom + bounds.top / 2))
	);
}

D2D1_RECT_F Shape::GetBounds()
{
	D2D1_RECT_F bounds;
	if (m_Shape) {
		HRESULT result = m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, GetShapeMatrix(), &bounds);
		if (SUCCEEDED(result))
		{
			return bounds;
		}
	}
	return D2D1::RectF();
}

bool Shape::IsShapeDefined()
{
	return m_Shape;
}

bool Shape::ContainsPoint(int x, int y)
{
	auto test = GetBounds();
	if (m_Shape) {
		BOOL outlineContains = false;
		BOOL fillContains = false;
		HRESULT result = m_Shape->StrokeContainsPoint(D2D1::Point2F(x, y), m_StrokeWidth, nullptr, GetShapeMatrix(), &outlineContains);
		if (SUCCEEDED(result))
			if (outlineContains)
				return true;

		HRESULT result2 = m_Shape->FillContainsPoint(D2D1::Point2F(x, y), GetShapeMatrix(), &fillContains);
		if (SUCCEEDED(result2))
			if (fillContains)
				return true;
	}
	return false;
}

void Shape::SetFillColor(D2D1_COLOR_F fillColor)
{
	m_FillColor = fillColor;
}

void Shape::SetStrokeWidth(float strokeWidth)
{
	m_StrokeWidth = strokeWidth;
}

void Shape::SetStrokeColor(D2D1_COLOR_F strokeColor)
{
	m_StrokeColor = strokeColor;
}

void Shape::SetRotation(float rotation)
{
	m_Rotation = rotation;
}

void Shape::SetRotationCenter(D2D1_POINT_2F rotationCenter)
{
	m_RotationCenter = rotationCenter;
}

void Shape::SetSkew(D2D1_POINT_2F skew)
{
	m_Skew = skew;
}

void Shape::SetScale(D2D1_SIZE_F scale)
{
	m_Scale = scale;
}

void Shape::SetOffset(float offsetX, float offsetY)
{
	m_Offset = D2D1::SizeF(offsetX, offsetY);
}

void Shape::SetAntialias(bool antialias)
{
	m_Antialias = antialias;
}

}  // namespace Gfx