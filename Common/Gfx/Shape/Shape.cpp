/* Copyright (C) 2016 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Shape.h"
#include "Gfx\Canvas.h"

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
	m_Antialias(true),
	m_ShoudDraw(true)
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

void Shape::UpdateShape(std::vector<Gdiplus::REAL> parameters)
{
	//Do nothing
}

bool Shape::IsShapeDefined()
{
	return m_Shape;
}

bool Shape::ShouldDraw()
{
	return m_ShoudDraw;
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

bool Shape::CombineWith(Shape* shape, D2D1_COMBINE_MODE mode, bool combineWithEmpty)
{
	if (combineWithEmpty)
	{
		ID2D1GeometrySink* geometrySink = NULL;
		auto pathGeometry = Canvas::CreatePathGeometry();
		auto emptyGeometry = Canvas::CreateRectangle(D2D1::RectF(0, 0, 0, 0));
		HRESULT result = pathGeometry->Open(&geometrySink);
		if (SUCCEEDED(result)) 
		{
			result = emptyGeometry->CombineWithGeometry(m_Shape.Get(), D2D1_COMBINE_MODE_UNION, GetShapeMatrix(), geometrySink);
			if (SUCCEEDED(result)) {
				geometrySink->Close();
				geometrySink->Release();
				geometrySink = nullptr;
				m_Shape = pathGeometry;
				m_Rotation = 0;
				m_Scale = D2D1::SizeF(1,1);
				m_Skew = D2D1::Point2F();
				m_Offset = D2D1::SizeF();
				return true;
			}
			else
			{
				geometrySink->Close();
				geometrySink->Release();
				geometrySink = nullptr;
				return false;
			}
		}
	}
	else
	{
		if (!m_Shape || !shape->m_Shape)
			return false;
		ID2D1GeometrySink* geometrySink = NULL;
		auto pathGeometry = Canvas::CreatePathGeometry();
		HRESULT result = pathGeometry->Open(&geometrySink);
		if (SUCCEEDED(result)) {
			result = m_Shape->CombineWithGeometry(shape->m_Shape.Get(), mode, shape->GetShapeMatrix(), geometrySink);
			if (SUCCEEDED(result)) {
				geometrySink->Close();
				geometrySink->Release();
				geometrySink = nullptr;
				m_Shape = pathGeometry;
				return true;
			}
			else
			{
				geometrySink->Close();
				geometrySink->Release();
				geometrySink = nullptr;
				return false;
			}
		}
	}

	return false;
}

Shape * Shape::Clone()
{
	Shape* clone = new Shape();
	clone->m_Antialias = m_Antialias;
	clone->m_FillColor = m_FillColor;
	clone->m_Offset = m_Offset;
	clone->m_Rotation = m_Rotation;
	clone->m_RotationCenter = m_RotationCenter;
	clone->m_Scale = m_Scale;
	clone->m_Skew = m_Skew;
	clone->m_StrokeColor = m_StrokeColor;
	clone->m_StrokeWidth = m_StrokeWidth;
	clone->m_Shape = m_Shape;
	return clone;
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

void Shape::SetDraw(bool shouldDraw)
{
	m_ShoudDraw = shouldDraw;
}

}  // namespace Gfx