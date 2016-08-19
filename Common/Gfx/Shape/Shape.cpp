/* Copyright (C) 2016 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Shape.h"

namespace Gfx {

D2D1_MATRIX_3X2_F Shape::GetShapeMatrix() const
{
	D2D1_POINT_2F center = D2D1::Point2F((m_UntransformedBounds.right - m_UntransformedBounds.left) / 2 + m_UntransformedBounds.left, (m_UntransformedBounds.bottom - m_UntransformedBounds.top) / 2 + m_UntransformedBounds.top);
	return D2D1::Matrix3x2F(
		//D2D1::Matrix3x2F::Translation(D2D1::SizeF(m_X, m_Y)) *
		D2D1::Matrix3x2F::Rotation(m_Rotation, center) *
		D2D1::Matrix3x2F::Skew(m_Skew.x, m_Skew.y, D2D1::Point2F(m_UntransformedBounds.left, m_UntransformedBounds.top)) *
		D2D1::Matrix3x2F::Translation(m_Offset) *
		D2D1::Matrix3x2F::Scale(m_Scale, center)
	);
}

bool Shape::IsShapeDefined()
{
	return m_Shape;
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

void Shape::SetOffset(D2D1_SIZE_F offset)
{
	m_Offset = offset;
}

void Shape::SetAntialias(bool antialias)
{
	m_Antialias = antialias;
}

}  // namespace Gfx